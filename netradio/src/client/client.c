#include<stdio.h>
#include<stdlib.h>


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
    printf("-P --port       指定接收端口\n");
    printf("-M  --mgrop     指定多播组\n");
    printf("-p  --player    指定播放器 \n");
    printf("-H  --help      显示帮助 \n");
}

int main(int argc, char **argv)
{
    int index = 0;
    int c;
    struct option argarr[] = {{"port", 1, NULL, 'P'},{"mgroup", 1, NULL, 'M'},\
                                {"player", 1, NULL, 'p'},{"help", 0, NULL, 'H'},\
                                {NULL, 0， NULL, 0}};
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
    socket();
    
    pipe();
    
    fork();

    //子进程：调用解码器
    //父进程：从网络收包，发送给子进程
    

    exit(0);
}
