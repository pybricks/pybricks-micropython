#ifndef __NXOS_MYTYPES_H__
#define __NXOS_MYTYPES_H__

typedef unsigned char U8;
typedef signed char S8;
typedef unsigned short U16;
typedef signed short S16;
typedef unsigned long U32;
typedef signed long S32;

typedef U32 size_t;
typedef U8 bool;

#define FALSE 0
#define TRUE (!FALSE)

#ifndef NULL
#define NULL 0
#endif

typedef void (*callback_t)(void);

#endif
