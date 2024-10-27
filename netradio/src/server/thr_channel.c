#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <syslog.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <string.h>

#include "thr_channel.h"
#include "proto.h"
#include "server_conf.h"

//将chnid与对应的线程关联，方便根据chnid取消该线程
struct thr_channel_ent_st
{
	chnid_t chnid;
	pthread_t tid;
};

struct thr_channel_ent_st thr_channel[CHNNR];		//最多
static int tid_nextpos = 0;


//组织当前待发送的数据，通过socket发送出去
static void *thr_channel_snder(void *ptr)
{
//	int datasize;								//当前数据量的最大值
	struct msg_channel_st *sbufp;
	int len;
	struct msg_listentry_st *ent = ptr;
	sbufp = malloc(MSG_CHANNEL_MAX);
	if(sbufp == NULL){
		syslog(LOG_ERR, "[thr_channel][thr_channel_snder] malloc():%s", strerror(errno));
		exit(1);
	}
	
	sbufp->chnid = ent->chnid;
	
	while(1){
		len = mlib_readchn(ent->chnid, sbufp->data, MAX_DATA);

		if(sendto(serversd, sbufp, len+sizeof(chnid_t), 0, (void *)&sndaddr, sizeof(sndaddr)) < 0){
			syslog(LOG_ERR, "[thr_channel][thr_channel_snder] chnid %d, sendto():%s", ent->chnid, strerror(errno));

		}
        else{
            syslog(LOG_DEBUG, "[thr_channel][thr_channel_snder] thr_channel(%d): sendto() successed.", ent->chnid);
        }
		//while(1)死循环， 线程调度策略，主动出让调度器
		//它用于主动让出处理器，允许其他线程或进程运行。这个调用告诉调度器，当前线程愿意放弃 CPU，使得调度器可以选择另一个线程来运行。
		sched_yield();
	}
	pthread_exit(NULL);
}

int thr_channel_create(struct mlib_listentry_st *ptr)
{
	int err;
	err = pthread_create(&thr_channel[tid_nextpos].tid, NULL, thr_channel_snder, ptr);
	if(err < 0){
		syslog(LOG_WARNING, "[thr_channel][thr_pthread_create] pthread_create():%s", strerror(err));
		return -err;
	}
	thr_channel[tid_nextpos].chnid = ptr->chnid;
	tid_nextpos++;
	
	return 0;
}
int thr_channel_destroy(struct mlib_listentry_st *ptr)
{
	for(int i = 0; i < CHNNR; i++){
		if(thr_channel[i].chnid == ptr->chnid){
			if(pthread_cancel(thr_channel[i].tid) < 0){
				syslog(LOG_ERR, "[thr_channel][thr_channel_destroy] pthread_cancel():the thread of channel %d",ptr->chnid);
				return -ESRCH;
			}
		}
		pthread_join(thr_channel[i].tid, NULL);
		thr_channel[i].chnid = -1;
		return 0;
	}
}
int thr_channel_destroyall(void)
{
	for(int i = 0; i < CHNNR; i++){
		if(thr_channel[i].chnid > 0){
			if(pthread_cancel(thr_channel[i].tid) < 0){
				syslog(LOG_ERR, "[thr_channel][thr_channel_destroyall] pthread_cancel():the thread of channel %d", thr_channel[i].chnid);
				return -ESRCH;
			}
			pthread_join(thr_channel[i].tid, NULL);
			thr_channel[i].chnid = -1;
		}
	}
	return 0;
}
