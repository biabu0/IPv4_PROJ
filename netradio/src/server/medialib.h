#ifndef MEDIALIB_H__
#define MEDIALIB_H__
#include<stdio.h>
#include"proto.h"

struct mlib_listentry_st
{
	chnid_t chnid;
	char *desc;					//有待发送的内容肯定不会用char*类型传递，这里只在本地使用
};


//频道信息要求结构体类型的数组，给起始位置； 数组有多长
extern int mlib_getchnlist(struct mlib_listentry_st **, int *);

extern //内存空间申请，这里释放
int mlib_freechnlist(struct mlib_listentry_st *);

//读频道，读频道，读到哪，读多少字节；
extern ssize_t mlib_readchn(chnid_t, void *, size_t);

#endif
