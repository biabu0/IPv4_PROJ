#ifndef MYTNF_H__
#define MYTBF_H__

#define MYTBF_MAX	1024			//最大1024个令牌桶
typedef void mytbf_t;


mytbf_t *mytbf_init(int cps, int burst);
int mytbf_fetchtoken(mytbf_t *, int );
int mytbf_returntoken(mytbf_t *, int);
int mytbf_destroy(mytbf_t *);






#endif
