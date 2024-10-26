#ifndef PROTO_H__
#define PROTO_H__

#include"site_type.h"

#define DEFAULT_MGROUP      "224.2.2.2"                       //组播地址
#define DEFAULT_RCVPORT     "2000"                            //默认端口

#define CHNNR               200                           //总的频道个数
#define LISTCHNID           0                                //特殊的频道号，发送节目单
#define MINCHNID            1                                //频道ID
#define MAXCHNID            (MINCHNID+CHNNR-1)

#define MSG_CHANNEL_MAX     (65536-20-8)                            //推荐包的长度-IP头-UDP头的推荐长度
#define MAX_DATA            (MSG_CHANNEL_MAX - sizeof(chnid_t))             //数据包上限

#define MSG_LIST_MAX        (65536-20-8)
#define MSG_ENTRY           (MSG_LIST_MAX-sizeof(chnid_t))

struct msg_channel_st
{
    chnid_t chnid;                          //频道号, chnid_t 方便拓展id；uint8_t 解决类型长度问题
    uint8_t data[1];                        //网络传输中多用变长的，不浪费资源；uint8_t 数据以单字节单位传输，包的形式，流媒体
}__attribute__((packed));                   //不需要对齐

/*
描述
1 music:xxxxxxxxxxxxxxxxxxxxx
2 sport:xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
3 ad:xxxxxxxxxxxxx
*/
struct msg_listentry_st
{
    chnid_t chnid;
    uint16_t len;                   //当前结构体多大
    uint8_t desc[1];                //描述
}__attribute__((packed));


struct msg_list_st
{
    chnid_t chnid;                          //一定是LISTCH:NID，即0
    struct msg_listentry_st entry[1];
}__attribute__((packed));



#endif
