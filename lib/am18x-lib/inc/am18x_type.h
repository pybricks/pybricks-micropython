// tary, 23:14 2012/12/16

#ifndef __AM18X_TYPE_H__
#define __AM18X_TYPE_H__

#define __USE_STDINT_H

#ifdef __USE_STDINT_H
#include <stdint.h>

#else
typedef signed char			int8_t;
typedef short				int16_t;
typedef int				int32_t;
typedef long long			int64_t;

typedef unsigned char			uint8_t;
typedef unsigned short			uint16_t;
typedef unsigned int			uint32_t;
typedef unsigned long long		uint64_t;

#define INT8_MIN			(-128)
#define INT16_MIN			(-32767-1)
#define INT32_MIN			(-2147483647-1)
#define INT64_MIN			(-0x7FFFFFFFFFFFFFFFLL - 1)

#define INT8_MAX			(127)
#define INT16_MAX			(32767)
#define INT32_MAX			(2147483647)
#define INT64_MAX			(0x7FFFFFFFFFFFFFFFLL)

#define UINT8_MAX			(255)
#define UINT16_MAX			(65535)
#define UINT32_MAX			(4294967295U)
#define UINT64_MAX			(0xFFFFFFFFFFFFFFFFULL)

#endif //__USE_STDINT_H

typedef signed char const		cint8_t;
typedef short const			cint16_t;
typedef int const			cint32_t;
typedef long long const			cint64_t;

typedef unsigned char const		cuint8_t;
typedef unsigned short const		cuint16_t;
typedef unsigned int const		cuint32_t;
typedef unsigned long long const	cuint64_t;

typedef volatile signed char		vint8_t;
typedef volatile short			vint16_t;
typedef volatile int			vint32_t;
typedef volatile long long		vint64_t;

typedef volatile unsigned char		vuint8_t;
typedef volatile unsigned short		vuint16_t;
typedef volatile unsigned int		vuint32_t;
typedef volatile unsigned long long	vuint64_t;

typedef volatile signed char const	vcint8_t;
typedef volatile short const		vcint16_t;
typedef volatile int const		vcint32_t;
typedef volatile long long const	vcint64_t;

typedef volatile unsigned char const	vcuint8_t;
typedef volatile unsigned short const	vcuint16_t;
typedef volatile unsigned int const	vcuint32_t;
typedef volatile unsigned long long const vcuint64_t;

#define _MASK_OFFSET_8BIT(x)		(((x) & 0x00000001UL)?0:	\
					(((x) & 0x00000002UL)?1:	\
					(((x) & 0x00000004UL)?2:	\
					(((x) & 0x00000008UL)?3:	\
					(((x) & 0x00000010UL)?4:	\
					(((x) & 0x00000020UL)?5:	\
					(((x) & 0x00000040UL)?6:	\
					(((x) & 0x00000080UL)?7:128	\
					))))))))

#define MASK_OFFSET(x)			(((x) & 0x000000FFUL)? _MASK_OFFSET_8BIT(x):			\
					(((x) & 0x0000FF00UL)? (8 + _MASK_OFFSET_8BIT((x)>>8)):		\
					(((x) & 0x00FF0000UL)? (16 + _MASK_OFFSET_8BIT((x)>>16)):	\
					(((x) & 0xFF000000UL)? (24 + _MASK_OFFSET_8BIT((x)>>24)):128	\
					))))

/* unaligned bits operations */
#define FIELD_GET(_reg,_msk)		((_reg) & (_msk))
#define FIELD_SET(_reg,_msk,_v)		(((_reg) & ~(_msk)) | ((_v) & (_msk)))
/* bit 0 aligned bits operations */
#define FIELD_XGET(_reg,_msk)		(((_reg) & (_msk)) >> MASK_OFFSET(_msk))
#define FIELD_XSET(_reg,_msk,_vx)	(((_reg) & ~(_msk)) | (((_vx) << MASK_OFFSET(_msk)) & (_msk)))

#ifndef __ASSEMBLY__
static inline uint32_t __ffs(uint32_t x)
{
	uint32_t r = 0;

	if (!x)	return 0;

	if (!(x & 0xffff)) {
		x >>= 16; r += 16;
	}
	if (!(x & 0xff)) {
		x >>= 8; r += 8;
	}
	if (!(x & 0xf)) {
		x >>= 4; r += 4;
	}
	if (!(x & 3)) {
		x >>= 2; r += 2;
	}
	if (!(x & 1)) {
		x >>= 1; r += 1;
	}
	return r;
}

static inline uint32_t __field_xget(uint32_t reg, uint32_t msk) {
	return ((reg & msk) >> __ffs(msk));
}

static inline uint32_t __field_xset(uint32_t reg, uint32_t msk, uint32_t vx) {
	return (reg & ~msk) | ((vx << __ffs(msk)) & msk);
}

typedef enum { AM18X_ERR = -1, AM18X_OK = 0} am18x_rt;
typedef enum { AM18X_FALSE = 0, AM18X_TRUE = !AM18X_FALSE} am18x_bool;

#ifndef NULL
#define NULL				((void*)0UL)
#endif

#endif

#define countof(x)			(sizeof(x) / sizeof(x[0]))
#define BIT(x)				(0x1UL << (x))
#define BIT_DEF(r,of,b,bdis,ben)		\
	r##_##b##_##MASK = (0x1UL << (of)),	\
	r##_##b##_##bdis = (0x0UL << (of)),	\
	r##_##b##_##ben  = (0x1UL << (of))


#endif//__AM18X_TYPE_H__
