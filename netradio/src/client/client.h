#ifndef CLIENT_H__
#define CLIENT_H__


//给定播放器的地址，命令行上不输出各种提示则将其重定向到空设备上
// - :表示只接受标准输入来的内容
#define DEFAULT_PLAYERCMD   "/usr/bin/mpg123 - > /dev/null"         //player没有必要给到server端


//用户端可以指定的内容
struct client_conf_st
{
    char *rcvport;              //跨网络的数据传输不能出现char *，这里不能直接上传网络
    char *mgroup;
    char *player_cmd;
};

//extern 声明；全局变量可以在包含当前.h文件的其他.c中使用，拓展当前全局变量的作用域

extern struct client_conf_st client_conf;       


#endif
