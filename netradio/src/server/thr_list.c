#include<stdio.h>
#include<stdlib.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netinet/ip.h>

#include "thr_list.h"
#include "proto.h"
#include "server_conf.h"

static pthread_t tid_list;

static int nr_list_ent;
static struct mlib_listentry_st *list_ent;



static void *thr_list(void *p)
{
	int totalsize;		//总字节
	struct msg_list_st *entlistp;			//节目单全部信息
	struct msg_listentry_st *entryp;		//部分信息

	totalsize = sizeof(chnid_t);
	for(int i = 0; i < nr_list_ent; i++){				//总的节目单个数
		syslog(LOG_INFO, "[thr_list][thr_list] chnid = %d, desc = %s", list_ent[i].chnid, list_ent[i].desc);
		totalsize += sizeof(struct msg_listentry_st) + strlen(list_ent[i].desc);			//变长
	}

	entlistp = malloc(totalsize);				//根据具体的大小创建存放节目单结构体的数据
	if(entlistp == NULL){
		syslog(LOG_ERR, "[thr_list][thr_list] malloc fail:%s", strerror(errno));
		exit(1);
	}
	entlistp->chnid = LISTCHNID;
	entryp = entlistp->entry;
	syslog(LOG_DEBUG, "[thr_list][thr_list] nr_list_ent: %d\n", nr_list_ent);				//节目单条数
	int size;
	int ret;

	for(int i = 0; i < nr_list_ent; i++){
		size = sizeof(struct msg_listentry_st) + strlen(list_ent[i].desc);
		//要和申请时的size一样，否则移动时就会出现地址对不齐的情况
        //size不确定，要根据具体内容desc来确定每个频道的size，所以第二项要用strlen
		entryp->chnid = list_ent[i].chnid;			//各个节目的chnid
		entryp->len = htons(size);					//长度
		strcpy(entryp->desc, list_ent[i].desc);
		entryp = (void *)(((char *)entryp) + size);	//使其便宜到下一个节目单的位置，这里存放的节目单结构体数组的起始位置
		syslog(LOG_DEBUG,"[thr_list][thr_list] entryp len :%d", entryp->len);
	}

	while(1){
		ret = sendto(serversd, entlistp, totalsize, 0,(void *)&sndaddr, sizeof(sndaddr));
		if(ret < 0){
			syslog(LOG_WARNING, "[thr_list][thr_list] sendto(serversd, entlistp.....:%s)", strerror(errno));
		}
		else{
			syslog(LOG_DEBUG, "[thr_list][thr_list] sendto(serversd, entlistp....): successd");
		}
		sleep(1);			//每隔一秒发送一次节目单
		
	}

}


int thr_list_create(struct mlib_listentry_st *listp, int nr_ent)
{
	int err;
	list_ent = listp;
	nr_list_ent = nr_ent;
	err = pthread_create(&tid_list, NULL, thr_list, NULL);
	if(err){
		syslog(LOG_ERR, "[thr_list][thr_list_create] pthread_create fail, the reason is %s.", strerror(err));
		return -1;
	}
	return 0;
}


int thr_list_destroy(void)
{
	pthread_cancel(tid_list);
	pthread_join(tid_list, NULL);
	return 0;
}


















