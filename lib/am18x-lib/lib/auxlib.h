// tary 22:22 2011/6/19
#ifndef __AUXLIB_H__
#define __AUXLIB_H__

#if defined(LBP_DBG)
#define DBG_LINE(nr, ...)		do {					\
	if (LOCAL_DBG) debug_line(__FUNCTION__, __LINE__, nr, __VA_ARGS__);	\
					}while(0)
#define DBG_BUF(name, buf, size)	do {					\
	if (LOCAL_DBG) debug_buf(name, buf, size);				\
					}while(0)
#define DBG_PRINT(fmt, ...)		do {					\
	if (LOCAL_DBG){								\
		printf("\r\nDBG:%s() L%d\t", __FUNCTION__, __LINE__);		\
		printf(fmt, __VA_ARGS__);					\
	}				}while(0)
#else
#define DBG_LINE(...)
#define DBG_BUF(...)
#define DBG_PRINT(...)
#endif//LBP_DBG

#include <stddef.h>
#define KV(x)		{ x, #x }
#define KOF(s,x)	{ offsetof(s,x), #x }
typedef struct {
	int key;
	char* val;
} kv_t;

// typedef unsigned long size_t;

#define __HAS_STRING_H__

#include <stdio.h>

int delay(int d);
#ifndef __HAS_STRING_H__
int puts(const char* s);
#endif

#define printk printf

int sscanf(const char* buf, const char* fmt, ...);
int sprintf(char * buf, const char *fmt, ...);
size_t strlen(const char* s);
int strcmp(const char* ss, const char* sd);
int strncmp(const char* ss, const char* sd, size_t size);
#ifndef __HAS_STRING_H__
int memset(void* dst, int pattern, size_t size);
int memcpy(char* dst, const char* src, size_t size);
#endif
int debug_buf(const char* head, char* buf, int len);
int debug_line(const char* file, int lin, int nr, ...);
int dump_regs_word(const char* head, unsigned base, size_t size);
unsigned get_byte_uint(char* buf, int size, int bigend);
int set_byte_uint(char* buf, int size, unsigned value, int bigend);
int make_argv(char* s, int size, char *argv[], int av_max);

unsigned get_exec_base(void);

#endif //__AUXLIB_H__
