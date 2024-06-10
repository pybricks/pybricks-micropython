#ifndef __LINKAGE_H__
#define __LINKAGE_H__

#define __LINUX_ARM_ARCH__	5
#define CONFIG_AEABI		1
#define ENTRY(x)		\
	.GLOBAL	x;		\
	.BALIGN	4;		\
	x:
#define ENDPROC(x)		\
	.type x, %function;	\
	.size x, . - x;

#define WARN_ON(x)
#define EXPORT_SYMBOL(x)
#define EXPORT_SYMBOL_GPL(x)
#define unlikely(x)		(x)

#endif //__LINKAGE_H__
