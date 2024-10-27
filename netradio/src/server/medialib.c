#include<stdlib.h>
#include<stdio.h>
#include<glob.h>
#include<fcntl.h>
#include<syslog.h>
#include<string.h>
#include<errno.h>
#include<unistd.h>


#include"medialib.h"
#include"server_conf.h"
#include"proto.h"
#include"site_type.h"
#include"mytbf.h"

#define PATH_SIZE	1024
#define LINEBUFSIZE	1024
#define MP3_BITRATE	1024*256


struct channel_context_st
{
	chnid_t chnid;
	char *desc;
	glob_t mp3glob;		//解析的mp3目录
	int pos;			//频道中的哪一个mp3文件位置
	int fd;				//打开mp3文件的文件描述符
	off_t offset;		//读取数据的偏移量
	mytbf_t *tbf;		//流量控制；
};

//是否做校验可以分为开环流控和闭环流控：开环不进行对方的确认信息；




static struct channel_context_st channel[MAXCHNID+1];


static struct channel_context_st *path2entry(const char *path)
{
	char pathstr[PATH_SIZE] = {'\0'};		//	初始化为空
	char linebuf[LINEBUFSIZE];
	FILE *fp;
	struct channel_context_st *me;
	static chnid_t curr_id = MINCHNID;

	strcpy(pathstr, path);
	strcat(pathstr, "/desc.text");

	fp = fopen(pathstr, "r");
	if(fp == NULL){
		syslog(LOG_INFO, "[medialib][path2entry] %s is not a channel dir(Cannot fid desc.text)", path);
		return NULL;
	}
	syslog(LOG_INFO, "[medialib][path2entry] fopen(): channel dir :%s.", path);
	
	//将desc内容读到linebuf中
	if(fgets(linebuf, LINEBUFSIZE, fp) == NULL){
		syslog(LOG_INFO, "[medialib][path2entry] %s is not a channel dir(Cannot get desc.text)", path);
		fclose(fp);
		return NULL;
	}

	fclose(fp);
	
	me = malloc(sizeof(*me));
	if(me == NULL){
		syslog(LOG_ERR, "[medialib][path2entry] malloc failed:%s", strerror(errno));
		return NULL;
	}
	me->tbf = mytbf_init(MP3_BITRATE/8, MP3_BITRATE/8*10);
	if(me->tbf == NULL){
		syslog(LOG_ERR,"[medialib][path2entry] mytbf_init() failed.");
		free(me);
		return NULL;
	}
	me->desc = strdup(linebuf);	//获得一份linebuf的副本，防止后序对desc操作会影响原来的linebuf；

	//解析MP3文件
	strncpy(pathstr, path, PATH_SIZE);
	strncat(pathstr, "/*.mp3", PATH_SIZE);
	if(glob(pathstr, 0, NULL, &me->mp3glob) != 0){
		syslog(LOG_ERR, "[medialib][path2entry] glob() failed; %s is not a channel dir.(Cannot find mp3 files)", path);
		free(me);
		return NULL;
	}

	me->pos = 0;
	me->offset = 0;
	me->fd = open(me->mp3glob.gl_pathv[me->pos], O_RDONLY);
	if(me->fd < 0){
		syslog(LOG_WARNING,"[medialib][path2entry] %s open() failed", me->mp3glob.gl_pathv[me->pos]);
		free(me);
		return NULL;
	}
	me->chnid = curr_id;
	curr_id ++;
	return me;


}


//频道信息要求结构体类型的数组，给起始位置； 数组有多长
int mlib_getchnlist(struct mlib_listentry_st **result, int *resnum)
{
	char path[PATH_SIZE];
	glob_t globres;
	int num = 0;				//记录多少目录结构，频道数
	struct mlib_listentry_st *ptr;
	struct channel_context_st *res;
	int i;

	for(i = 0; i < MAXCHNID+1; i++){
		channel[i].chnid = -1;				//当前未启用该频道ID
	}


	snprintf(path, PATH_SIZE, "%s*", server_conf.media_dir);			//   media_dir/*路径
	syslog(LOG_INFO, "[medialib][mlib_getchnlist] path: %s", path);

	if(glob(path, 0, NULL, &globres)){
		syslog(LOG_ERR, "[medialib][mlib_getchnlist] glob() error:%s", strerror(errno));
		return -1;	
	}
	
	ptr = malloc(sizeof(struct mlib_listentry_st) * globres.gl_pathc);
	if(ptr == NULL){
		syslog(LOG_ERR, "[medialib][mlib_getchnlist] malloc() error.");
		exit(1);
	}
	

	//将每个ch的数据放到channel中
	for(int i = 0; i < globres.gl_pathc; i++){			//路径数
		//globres.gl_pathv[i] -> "/var/media/ch1"
		res = path2entry(globres.gl_pathv[i]);		//继续解析目录						
		if(res != NULL){
			syslog(LOG_DEBUG, "[medialib][mlib_getchnlist] path2entry success [chnid:%d  desc: %s]", res->chnid, res->desc);
			memcpy(channel + res->chnid, res, sizeof(*res));
			ptr[num].desc = strdup(res->desc);
			ptr[num].chnid = res->chnid;

			num ++;
		}
	}
	
	*result = realloc(ptr, sizeof(struct mlib_listentry_st) * num);		//节目单信息
	if(result == NULL){
		syslog(LOG_ERR, "[medialib][mlib_getchnlist] realloc() failed.");
	}
	*resnum = num;

	return 0;
}

//内存空间申请，这里释放
int mlib_freechnlist(struct mlib_listentry_st *ptr)
{
	free(ptr);
	return 0;
}

//试打开下一个MP3文件，并在成功时返回 0，如果所有文件都无法打开，则返回 -1。
static int open_next(chnid_t chnid)
{
	for(int i = 0; i < channel[chnid].mp3glob.gl_pathc; i++){
		channel[chnid].pos++;					//下一个mp3文件
		if(channel[chnid].pos == channel[chnid].mp3glob.gl_pathc){			//如果是最后一个mp3文件，返回第一个mp3文件重新检索
			channel[chnid].pos = 0;
			break;								//读取完最后一个mp3文件，结束
		}
		close(channel[chnid].fd);
		channel[chnid].fd = open(channel[chnid].mp3glob.gl_pathv[channel[chnid].pos], O_RDONLY);
		//打不开换下一个
		if(channel[chnid].fd < 0){
			syslog(LOG_WARNING, "[medialib][open_next] open(%s), fail:%s", channel[chnid].mp3glob.gl_pathv[channel[chnid].pos], strerror(errno));
		}
		else{
			channel[chnid].offset = 0;
			return 0;
		}
	}
	syslog(LOG_ERR, "[medialib][open_next] None of mp3s in channel %d id available.", chnid);
	return -1;
	
}



//读频道，读频道，读到哪，读多少字节；
ssize_t mlib_readchn(chnid_t chnid, void *buf, size_t size)
{
	int tbfsize;
	int len;
	
	//流量限制
	tbfsize = mytbf_fetchtoken(channel[chnid].tbf, size);

	while(1){
		//pread从文件的指定偏移量开始读取数据，而不改变文件的当前读写位置（文件指针）。
		len = pread(channel[chnid].fd, buf, tbfsize, channel[chnid].offset);
		//当前chnid频道中的节目1不能读，给出警告，读取下一个节目
		if(len < 0){							
			syslog(LOG_WARNING, "[meidalib][mlib_readchn] meida file %s pread() failed : %s", channel[chnid].mp3glob.gl_pathv[channel[chnid].pos], strerror(errno));
			open_next(chnid);
		}
		//当前chnid频道中的节目1正好读完，读取下一个节目
		else if(len == 0){
			syslog(LOG_DEBUG, "[medialib][mlib_readchn] meida file %s is over.", channel[chnid].mp3glob.gl_pathv[channel[chnid].pos]);
			//channel[chnid].offset = 0;				//下一首歌重新从偏移量0的位置读取
			open_next(chnid);
		}
		else{			//成功读取len个字节
			channel[chnid].offset += len;
			break;
		}
	}
	//返回没有用完的令牌
	if(tbfsize - len > 0){
		mytbf_returntoken(channel[chnid].tbf, tbfsize-len);
	}

	return len;
}





