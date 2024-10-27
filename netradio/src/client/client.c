#include<stdio.h>
#include<stdlib.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<unistd.h>
#include<errno.h>
#include<arpa/inet.h>
#include<getopt.h>
#include<net/if.h>

#include<proto.h>                   //有Makefile中指定地址

#include"client.h"

/*
 *  -M  --mgrop     指定多播组
 *  -P  --port      指定接收端口
 *  -p  --player    指定播放器
 *  -H  --help      显示帮助
 **/

struct client_conf_st client_conf = {\
        .rcvport = DEFAULT_RCVPORT,\
        .mgroup = DEFAULT_MGROUP,\
        .player_cmd = DEFAULT_PLAYERCMD};


static void printhelp(void)
{
    printf("-P  --port      指定接收端口\n");
    printf("-M  --mgrop     指定多播组\n");
    printf("-p  --player    指定播放器 \n");
    printf("-H  --help      显示帮助 \n");
}


static ssize_t writen(int fd, const char *buf, size_t len)
{
    int ret = 0;
    int pos = 0;
    while(len > 0){         //读取了内容就跳出循环
        ret = write(fd, buf+pos, len);
        if(ret < 0){
            if(errno == EINTR)
                continue;
            perror("write");
            return -1;
        }
        len -= ret;
        pos += ret;
    }
    
    return pos;
}

int main(int argc, char **argv)
{
    int result;
    pid_t pid;
    int pd[2];
    int index = 0;
    int c;
    int sd;
    int val;
    int chosenid;
    struct ip_mreqn mreq;
    struct sockaddr_in laddr, serveraddr,raddr;
    socklen_t raddr_len;
    socklen_t serveraddr_len;
    struct option argarr[] = {{"port", 1, NULL, 'P'},{"mgroup", 1, NULL, 'M'},\
                                {"player", 1, NULL, 'p'},{"help", 0, NULL, 'H'},\
                                {NULL, 0, NULL, 0}};
    /*初始化
     *级别：默认值，配置文件，环境变量，命令行参数（高）
     **/
    while(1){ 
        c = getopt_long(argc, argv, "P:M:p:H", argarr, &index);         //分析命令行参数
        if(c < 0)
            break;
        switch(c){
            case 'P':
                client_conf.rcvport = optarg;
                break;
            case 'p':
                client_conf.player_cmd = optarg;
                break;
            case 'M':
                client_conf.mgroup = optarg;
                break;
            case 'H':
                printhelp();
                exit(1);
                break;
            default:
                abort();
                break;

        }
    }
    sd = socket(AF_INET, SOCK_DGRAM, 0);                        //报的形式传输
    if(sd < 0){
        perror("socket()");
        exit(1);
    }

    //多播地址
    inet_pton(AF_INET, client_conf.mgroup, &mreq.imr_multiaddr);
    //本地地址
    inet_pton(AF_INET, "0.0.0.0", &mreq.imr_address);
    //当前网络设备索引号
    mreq.imr_ifindex = if_nametoindex("ens33");        //使用ifconfig命令查看网卡；这里可以使用命令端指定
    
    //设置属性，加入多播组
    if(setsockopt(sd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0){
        perror("setsockopt()");
        exit(1);
    }
    

    //开启多播数据包的回环功能,当设置为1时，开启回环功能，发送者可以接收到自己发送的多播数据包。
    val = 1;
    if(setsockopt(sd, IPPROTO_IP, IP_MULTICAST_LOOP, &val, sizeof(val)) <  0){
        perror("setsockopt()");
        exit(1);
    }


/*
    绑定"0.0.0.0"作为本地地址意味着该套接字将监听所有可用的网络接口上的多播数据包。这允许客户端从网络中的任何位置接收多播数据，而无需指定特定的网络接口或IP地址。
*/
    laddr.sin_family = AF_INET;
    laddr.sin_port = htons(atoi(client_conf.rcvport));
    inet_pton(AF_INET, "0.0.0.0", &laddr.sin_addr);

    //绑定本地地址
    if(bind(sd, (void *)&laddr, sizeof(laddr))<0){
        perror("bind()");
        exit(1);
    }

    if(pipe(pd) < 0){
        perror("pipe()");
        exit(1);
    }

    
    pid = fork();
    if(pid < 0){
        perror("fork()");
        exit(1);
    }

    //子进程调用解码器
    if(pid == 0){

        close(sd);                      //子进程中的sd关闭，不要泄露文件描述符
        close(pd[1]);                   //子进程不需要管道的写端口
        dup2(pd[0], 0);                  //管道的读端作为0号文件描述符，作为子进程的标准输入；播放的时候解码器只能获得标准输入的内容，使用的是"-"
        if(pd[0] > 0)                   //本身不是标准输入
            close(pd[0]);
        
        //子进程播放器选择
        //使用shell程序执行
        execl("/bin/sh", "sh", "-c", client_conf.player_cmd, NULL);     
        perror("execl()");
        exit(1);
        
    }
    else{

    //父进程：从网络收包，发送给子进程
    
    //收节目单
    struct msg_list_st *msg_list;

    msg_list = malloc(MSG_LIST_MAX);        //最大的包
    if(msg_list == NULL){
        perror("malloc()");
        exit(1);
    }

    //如果不加第一次打印地址可能是错误的，但后面应该就正确了
    serveraddr_len = sizeof(serveraddr);
    int len;    
    while(1)
    {
        len = recvfrom(sd, msg_list, MSG_LIST_MAX, 0, (void *)&serveraddr, &serveraddr_len);
        if(len < sizeof(struct msg_list_st)){
            fprintf(stderr, "message is too small.\n");
            continue;
        }
        if(msg_list->chnid != LISTCHNID)   //不是节目单频道号
        {
            fprintf(stderr, "chnid is not match.\n");
            continue;
        }
        break;              //收到节目单
    }

    //打印节目单并选择频道
    struct msg_listentry_st *pos;
    //
    for(pos = msg_list->entry; (char *)pos < (((char *)msg_list) + len) ;pos = (void*)(((char *)pos) + ntohs(pos->len))){
        printf("channel %d : %s\n", pos->chnid, pos->desc);
    }

    free(msg_list);                 //节目单已经打印出来，不在需要该结构体，释放
    
    //scanf放在循环中危险，如果输入‘a’，会放在缓冲区中取不出来，会一直在while中
    
    puts("Please enter: ");
    int ret;
    while(ret < 1){
        ret = scanf("%d", &chosenid);
        if(ret != 1)
            exit(1);                        //做为异常终止的一种
    }


    fprintf(stdout, "chosenid = %d\n", chosenid);        //不是守护进程可以用

    //收频道包，发送给子进程
    
    
    struct msg_channel_st *msg_channel;

    msg_channel = malloc(MSG_CHANNEL_MAX);
    if(msg_channel == NULL){
        perror("malloc()");
        exit(1);
    }
    
    raddr_len = sizeof(raddr);

    while(1){
        len = recvfrom(sd, msg_channel, MSG_CHANNEL_MAX, 0, (void *)&raddr, &raddr_len);
        if(len < 0){
            perror("recvfrom()");
            exit(1);
        }
        //验证当前收到的频道包的发送方是否与收到节目包的发送方地址一样
        if(raddr.sin_addr.s_addr != serveraddr.sin_addr.s_addr || raddr.sin_port != serveraddr.sin_port){
            fprintf(stderr, "Ignore:address not match.\n");
            continue;               //再给一次机会试一下
        }

        if(len < sizeof(struct msg_channel_st)){            //两个字节都不到（边长的结构体）
            fprintf(stderr, "Ignore: message too small.\n");
            continue;
        }
        //

        if(msg_channel->chnid == chosenid)
        {
            fprintf(stdout,"accepted msg:%d recieved.\n", msg_channel->chnid);
            //writen函数，坚持写够多少个字节
            result = writen(pd[1], msg_channel->data, len-sizeof(chnid_t));
            if(result < 0)
                exit(1);
        }
        
    }
    
    //可以使用信号来处理，将这些free操作和close操作放到信号处理函数中
    free(msg_channel);
    close(sd);

    exit(0);
    }

    exit(0);


}



