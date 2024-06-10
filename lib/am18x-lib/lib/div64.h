#ifndef __ASM_ARM_DIV64
#define __ASM_ARM_DIV64

/*
 * The semantics of do_div() are:
 *
 * uint32_t do_div(uint64_t *n, uint32_t base)
 * {
 * 	uint32_t remainder = *n % base;
 * 	*n = *n / base;
 * 	return remainder;
 * }
 *
 * In other words, a 64-bit dividend with a 32-bit divisor producing
 * a 64-bit result and a 32-bit remainder.  To accomplish this optimally
 * we call a special __do_div64 helper with completely non standard
 * calling convention for arguments and results (beware).
 */

#define __xl "r0"
#define __xh "r1"
#define __asmeq(x, y)  ".ifnc " x "," y " ; .err ; .endif\n\t"


#define __do_div_asm(n, base)					\
({								\
	register unsigned int __base      asm("r4") = base;	\
	register unsigned long long __n   asm("r0") = n;	\
	register unsigned long long __res asm("r2");		\
	register unsigned int __rem       asm(__xh);		\
	asm(	__asmeq("%0", __xh)				\
		__asmeq("%1", "r2")				\
		__asmeq("%2", "r0")				\
		__asmeq("%3", "r4")				\
		"bl	__do_div64"				\
		: "=r" (__rem), "=r" (__res)			\
		: "r" (__n), "r" (__base)			\
		: "ip", "lr", "cc");				\
	n = __res;						\
	__rem;							\
})


/*
 * If the divisor happens to be constant, we determine the appropriate
 * inverse at compile time to turn the division into a few inline
 * multiplications instead which is much faster. And yet only if compiling
 * for ARMv4 or higher (we need umull/umlal) and if the gcc version is
 * sufficiently recent to perform proper long long constant propagation.
 * (It is unfortunate that gcc doesn't perform all this internally.)
 */
#define do_div(n, base)							\
({									\
	unsigned int __r, __b = (base);					\
	/* non-constant divisor (or zero): slow path */			\
	__r = __do_div_asm(n, __b);					\
	__r;								\
})

/* our own fls implementation to make sure constant propagation is fine */
#define __div64_fls(bits)						\
({									\
	unsigned int __left = (bits), __nr = 0;				\
	if (__left & 0xffff0000) __nr += 16, __left >>= 16;		\
	if (__left & 0x0000ff00) __nr +=  8, __left >>=  8;		\
	if (__left & 0x000000f0) __nr +=  4, __left >>=  4;		\
	if (__left & 0x0000000c) __nr +=  2, __left >>=  2;		\
	if (__left & 0x00000002) __nr +=  1;				\
	__nr;								\
})


#endif
