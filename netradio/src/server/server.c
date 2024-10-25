#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>
#include<string.h>
#include<errno.h>

#include"server_conf.h"
#include<proto.h>

/*
 *	-M		指定多播组
 *	-P		指定接收端口
 *	-F		前台运行			//守护进程前台运行便于查看变量的输出情况
 *	-I		指定网络设备		
 *	-H		显示帮助
 *	-D		指定媒体库位置
 *
 **/




struct server_conf_st server_conf = {.rcvport = DEFAULT_RCVPORT,\
									.mgroup = DEFAULT_MGROUP,\
									.media_dir = DEFAULT_MEDIA_DIR,\
									.runmode = RUN_DAEMON,\    		//不方便进行单元测试或者模块测试
									.ifname = DEFAULT_IF};


int serversd;				//不能使用static 
struct sockaddr_in sndaddr;

static void printf_help(void)
{

	printf("-M      指定多播组\n");
	printf("-P      指定接收端口\n");
	printf("-F      前台运行\n");
	printf("-I      指定网络设备\n");
	printf("-H      显示帮助\n");
	printf("-D      指定媒体库位置\n");
	
}




//脱离控制终端，没有父进程等待，子进程会成为session的leader，父进程等待则父进程也要脱离终端；直接将父进程结束，子进程变成孤儿进程，会被1号init接管，父进程也就是init进程，直到当前文件系统卸载，或者设备卸载为止
	

static void daemon_exit(int s)
{

	closelog();

	exit(0);//正常终止
}


static int demonize(void)
{
	
	pid_t pid;
	int fd;
	
	//进程ID，PID父进程所用的资源总量会在子进程清0，父进程的信号掩码不被进程，父进程中的未决信号不会直接派生到子进程，文件锁不会被继承
	pid = fork();
	if(pid < 0){
//		perror("fork()");
		syslog(LOG_ERR, "fork():%s", strerror(errno));			//不要写\n
		return -1;
	}
	if(pid > 0)
		exit(0);				//正常结束
	
	fd = open("dev/null", O_RDWR);
	if(fd < 0){
//		perror("open()");			//此时还没有脱离控制终端，可以使用perror输出到stderr
		syslog(LOG_WARNING, "open():%s", strerror(errno));			//失败无法重定向，给出警告
	i	return -2;
	}
	/*守护进程通常不需要从用户那里读取输入，也不应该将输出直接显示在终端上。通过重定向到 /dev/null，可以确保守护进程不会尝试读取标准输入或向标准输出和标准错误写入*/
	/*如果守护进程将错误信息输出到日志文件，而没有任何限制，那么随着时间的推移，日志文件可能会变得非常大。重定向到 /dev/null 可以防止这种情况，因为 /dev/null 是一个黑洞设备，所有写入它的数据都会被丢弃。*/
	/*如果需要记录守护进程的输出或错误，可以将其重定向到日志文件，而不是 /dev/null。*/
	else{
		dup2(fd, 0);
		dup2(fd, 1);
		dup2(fd, 2);
		if(fd > 2)
			close(fd);
	}
	setsid();
	//当前进程的工作路径改变到绝对有的路径；假如改到U盘的话在卸载的时候会不成功
	//这样做是为了确保守护进程不会保持对任何特定挂载点的引用。
	chdir("/");
	//设置文件模式创建掩码为0，以确保守护进程创建文件时不会受到默认掩码的限制。
	umask(0);
	
}

static socket_init(void)
{
	struct ip_mreqn mreq;

	serversd = socket(AF_INET, SOCK_DGRAM, 0);
	if(serversd < 0){
		syslog(LOG_ERR,"socket:%s", strerror(errno));
		exit(1);
	}
	

	//创建多播组
	inet_pton(AF_INET, server_conf.mgroup, &mreq.imr_multiaddr);
	inet_pton(AF_INET, "0.0.0.0", &mreq.imr_addr);
	mreq.imr_ifindex = if_nametoindex(server_conf.ifname);
	if(setsockopt(serversd, IPPROTO_IP, IP_MULTICAST_IF, &mreq, sizeof(mreq)) < 0){
		syslog(LOG_ERR, "setsockopt(IP_MULTICAST_IF):%s",strerror(errno));
		exit(1);
	}

	//bind();			//主动端，不需要bind()

}

int main(int argc, char *argv[])
{
	int c;
	struct sigaction sa;

	sa.sa_handler = daemon_exit;
	sigemptyset(&sa.sa_mask);
	sigaddset(&sa.sa_mask, SIGINT);
	sigaddset(&sa.sa_mask, SIGQUIT);
	sigaddset(&sa.sa_mask, SIGTERM);

	sigaction(SIGTERM, &sa, NULL);		//不使用signal，其有重入的问题；在这里无论哪个信号都会执行同一个处理函数
	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGQUIT, &sa, NULL);

	/*日志文件带哪些信息：PID信息，perror; 来源：DAEMON守护进程*/
	opnelog("netradio", LOG_PID|LOG_PERROR, LOG_DAEMON);
	
	while(1)
	{
		/*命令行分析*/
		c = getopt(argc, argv, "M:P:FD:I:H");			//:表示有参数修饰
		if(c < 0)
			break;
		switch(c){
			case 'M':
				server_conf.mgroup = optarg;		//optarg表示参数内容
				break;
			case 'P':
				server_conf.rcvport = optarg;
				break;
			case 'F':
				server_conf.runmode = RUN_FOREGROUD;
				break;
			case 'I':
				server_conf.ifname = optarg;
				break;
			case 'D':
				server_conf.media_dir = optarg;
				break;
			case 'H':
				printf_help();
				exit(0);
				break;
			default:
				abort();				//在检测到无法恢复的错误或异常情况时，使程序能够安全地停止运行。
				break;

		}
			
	}
	//server放到后台脱离控制终端
	//进程脱离控制终端，将标准文件描述符重定向>>>>>>>>>>>>>>>>>>>>>>>>
	/*	守护进程的实现	*/
	if(server_conf.runmode == RUN_DAEMON){
		/*守护进程只能用信号杀死进程，异常终止，内存来不及释放等问题；当前没有办法关联标准的输入输出和出错，需要写系统日志*/
		if(daemonize() != 0)
			exit(1);
	}
	else if(server_conf.runmode == RUN_FOREGROUND){
		/*do nothing*/
	}
	else{
		syslog(LOG_ERR, "EINVAL server_conf.runmode.");
		exit(1);
	}

	/*	SOCKET初始化	*/
	
	socket_init();

	/*	获取频道信息	*/
	struct mlib_listentry_st *list;
	int list_size;
	int err;
	err = mlib_getchnlist(&list, &list_size);
	if(){
		
	}
	/*	创建节目单线程	*/

	thr_list_create(list, list_size);
	/*if error*/

	/*	创建频道线程	*/			
	//用一个线程发送200个频道信息，不怕延迟（一个包延迟后面的包还是会延迟的，依然连贯），怕延迟抖动（延迟时间或长或短，包的顺序可能改变）；64位系统，栈的大小128T，可以创建的线程非常的多，使用200个线程创建200个频道；
	int i;
	for(i = 0; i < list_size; i++){
		thr_channel_create(list+i);
		/* if error*/
	}


	syslog(LOG_DEBUG,"%d channel threads created.", i);				//提示
	

	while(1)
		pause(1);		//不会结束但也不至于频繁消耗资源

//	closelog();			//异常终止，执行不到，放入daemon_exit

	exit(0);
}






















