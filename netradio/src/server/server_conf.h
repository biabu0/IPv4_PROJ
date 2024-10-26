#ifndef SERVER_CONF_H__
#define SERVER_CONF_H__


#define DEFAULT_MEDIADIR		"/var/media/"
#define DEFAULT_IF 				"ens33"


enum
{
	RUN_DAEMON = 1,				//守护进程运行
	RUN_FOREGROUND				//前台运行，方便调参数
};

struct server_conf_st
{
	char *rcvport;
	char *mgroup;
	char *media_dir;
	char *ifname;
	char runmode;
};
//声明

extern struct server_conf_st server_conf;
extern int serversd;
extern struct sockaddr_in sndaddr;			//对端地址


#endif
