#ifndef SERVER_CONF_H__
#define SERVER_CONF_H__


#define DEFAULT_MEDIADIR		"var/meida/"
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

extern struct server_conf_st server_conf;
extern int serversd;
extern struct sockaddr_in sndaddr;


#endif
