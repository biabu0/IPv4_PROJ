#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>
#include<time.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<string.h>
#include<pthread.h>
#include<errno.h>

#include"mytbf.h"

struct mytbf_st
{
	int cps;
	int burst;
	int token;
	int pos;
	pthread_mutex_t mut;		//token防止竞争和冲突
	pthread_cond_t cond;
	
};

struct mytbf_st *job[MYTBF_MAX];
static pthread_mutex_t mut_job = PTHREAD_MUTEX_INITIALIZER;
static pthread_once_t init_once = PTHREAD_ONCE_INIT;
static pthread_t tid;

static int get_free_pos_unlocked(void)
{
	int i;
	for(i = 0; i < MYTBF_MAX; i++)
		if(job[i] == NULL)
			return i;
	return -1;
}


static int min(int a, int b)
{
	if(a > b)
		return b;
	else
		return a;
}

static void *thr_alrm(void *p)
{
	
	int i;

	while(1){
		pthread_mutex_lock(&mut_job);
		for(i = 0; i < MYTBF_MAX; i++){
		
			if(job[i] != NULL){
				pthread_mutex_lock(&job[i]->mut);
				job[i]->token += job[i]->cps;
				if(job[i]->token > job[i]->burst)
					job[i]->token = job[i]->burst;

				pthread_cond_broadcast(&job[i]->cond);
				pthread_mutex_unlock(&job[i]->mut);
			}
		}
		pthread_mutex_unlock(&mut_job);
		sleep(1);
	}

}

static void module_unload(void)
{
	int i;
	pthread_cancel(tid);
	pthread_join(tid, NULL);

	for(i = 0; i < MYTBF_MAX; i++)
		free(job[i]);
	return ;
}


static void module_load(void)
{
	int err;

	err = pthread_create(&tid, NULL, thr_alrm, NULL);
	if(err < 0){
		fprintf(stderr, "pthread_create():%s\n", strerror(err));
		exit(1);
	}

	atexit(module_unload);

}


mytbf_t *mytbf_init(int cps, int burst)
{
	struct mytbf_st *me;
	int pos;

	pthread_once(&init_once, module_load);			//只执行一次，无论多少个线程调用
	

	me = malloc(sizeof(*me));
	if(me == NULL){
		return NULL;
	}
	me->cps = cps;
	me->burst = burst;
	me->token = 0;
	pthread_mutex_init(&me->mut, NULL);
	pthread_cond_init(&me->cond, NULL);
	
	pthread_mutex_lock(&mut_job);
	pos = get_free_pos_unlocked();		//
	if(pos < 0){
		pthread_mutex_unlock(&mut_job);
		free(me);
		return NULL;
	}

	me->pos = pos;
	job[me->pos] = me;
	pthread_mutex_unlock(&mut_job);

	return me;
}
int mytbf_fetchtoken(mytbf_t *ptr, int size)
{
	struct mytbf_st *me = ptr;
	int n;
	pthread_mutex_lock(&me->mut);
	while(me->token <= 0)
		pthread_cond_wait(&me->cond, &me->mut);

	n = min(me->token,size);

	me->token -= n;

	pthread_mutex_unlock(&me->mut);

	return n;

}
int mytbf_returntoken(mytbf_t *ptr, int size)
{
	struct mytbf_st *me = ptr;

	pthread_mutex_lock(&me->mut);
	me->token += size;
	if(me->token > me->burst)
		me->token = me->burst;
	
	pthread_cond_broadcast(&me->cond);		//token改变，发送通知
	pthread_mutex_unlock(&me->mut);

	return 0;
}
int mytbf_destroy(mytbf_t *ptr)
{
	struct mytbf_st *me = ptr;
	pthread_mutex_lock(&mut_job);
	job[me->pos] = NULL;
	pthread_mutex_unlock(&mut_job);

	pthread_mutex_destroy(&me->mut);
	pthread_cond_destroy(&me->cond);
	free(ptr);

	return 0;
}



