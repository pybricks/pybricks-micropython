/*
 *  linux/lib/vsprintf.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 */

/* vsprintf.c -- Lars Wirzenius & Linus Torvalds. */
/* 
 * Fri Jul 13 2001 Crutcher Dunnavant <crutcher+kernel@datastacks.com>
 * - changed to provide snprintf and vsnprintf functions
 * So Feb  1 16:51:32 CET 2004 Juergen Quade <quade@hsnr.de>
 * - scnprintf and vscnprintf
 */
#include <am18x_lib.h>
#include <stdarg.h>
#include <string.h>
#include <limits.h>
#include "linkage.h"
#include "ctype.h"
#include "div64.h"

#define EINVAL			1
#define PAGE_SIZE		0x20

/* Works only for digits and letters, but small and fast */
#define TOLOWER(x) ((x) | 0x20)

static unsigned int simple_guess_base(const char *cp)
{
	if (cp[0] == '0') {
		if (TOLOWER(cp[1]) == 'x' && isxdigit(cp[2]))
			return 16;
		else
			return 8;
	} else {
		return 10;
	}
}

/**
 * simple_strtoul - convert a string to an unsigned long
 * @cp: The start of the string
 * @endp: A pointer to the end of the parsed string will be placed here
 * @base: The number base to use
 */
unsigned long simple_strtoul(const char *cp, char **endp, unsigned int base)
{
	unsigned long result = 0;

	if (!base)
		base = simple_guess_base(cp);

	if (base == 16 && cp[0] == '0' && TOLOWER(cp[1]) == 'x')
		cp += 2;

	while (isxdigit(*cp)) {
		unsigned int value;

		value = isdigit(*cp) ? *cp - '0' : TOLOWER(*cp) - 'a' + 10;
		if (value >= base)
			break;
		result = result * base + value;
		cp++;
	}

	if (endp)
		*endp = (char *)cp;
	return result;
}
EXPORT_SYMBOL(simple_strtoul);

/**
 * simple_strtol - convert a string to a signed long
 * @cp: The start of the string
 * @endp: A pointer to the end of the parsed string will be placed here
 * @base: The number base to use
 */
long simple_strtol(const char *cp, char **endp, unsigned int base)
{
	if(*cp == '-')
		return -simple_strtoul(cp + 1, endp, base);
	return simple_strtoul(cp, endp, base);
}
EXPORT_SYMBOL(simple_strtol);

/**
 * simple_strtoull - convert a string to an unsigned long long
 * @cp: The start of the string
 * @endp: A pointer to the end of the parsed string will be placed here
 * @base: The number base to use
 */
unsigned long long simple_strtoull(const char *cp, char **endp, unsigned int base)
{
	unsigned long long result = 0;

	if (!base)
		base = simple_guess_base(cp);

	if (base == 16 && cp[0] == '0' && TOLOWER(cp[1]) == 'x')
		cp += 2;

	while (isxdigit(*cp)) {
		unsigned int value;

		value = isdigit(*cp) ? *cp - '0' : TOLOWER(*cp) - 'a' + 10;
		if (value >= base)
			break;
		result = result * base + value;
		cp++;
	}

	if (endp)
		*endp = (char *)cp;
	return result;
}
EXPORT_SYMBOL(simple_strtoull);

/**
 * simple_strtoll - convert a string to a signed long long
 * @cp: The start of the string
 * @endp: A pointer to the end of the parsed string will be placed here
 * @base: The number base to use
 */
long long simple_strtoll(const char *cp, char **endp, unsigned int base)
{
	if(*cp=='-')
		return -simple_strtoull(cp + 1, endp, base);
	return simple_strtoull(cp, endp, base);
}

/**
 * strict_strtoul - convert a string to an unsigned long strictly
 * @cp: The string to be converted
 * @base: The number base to use
 * @res: The converted result value
 *
 * strict_strtoul converts a string to an unsigned long only if the
 * string is really an unsigned long string, any string containing
 * any invalid char at the tail will be rejected and -EINVAL is returned,
 * only a newline char at the tail is acceptible because people generally
 * change a module parameter in the following way:
 *
 * 	echo 1024 > /sys/module/e1000/parameters/copybreak
 *
 * echo will append a newline to the tail.
 *
 * It returns 0 if conversion is successful and *res is set to the converted
 * value, otherwise it returns -EINVAL and *res is set to 0.
 *
 * simple_strtoul just ignores the successive invalid characters and
 * return the converted value of prefix part of the string.
 */
int strict_strtoul(const char *cp, unsigned int base, unsigned long *res)
{
	char *tail;
	unsigned long val;
	size_t len;

	*res = 0;
	len = strlen(cp);
	if (len == 0)
		return -EINVAL;

	val = simple_strtoul(cp, &tail, base);
	if (tail == cp)
		return -EINVAL;
	if ((*tail == '\0') ||
		((len == (size_t)(tail - cp) + 1) && (*tail == '\n'))) {
		*res = val;
		return 0;
	}

	return -EINVAL;
}
EXPORT_SYMBOL(strict_strtoul);

/**
 * strict_strtol - convert a string to a long strictly
 * @cp: The string to be converted
 * @base: The number base to use
 * @res: The converted result value
 *
 * strict_strtol is similiar to strict_strtoul, but it allows the first
 * character of a string is '-'.
 *
 * It returns 0 if conversion is successful and *res is set to the converted
 * value, otherwise it returns -EINVAL and *res is set to 0.
 */
int strict_strtol(const char *cp, unsigned int base, long *res)
{
	int ret;
	if (*cp == '-') {
		ret = strict_strtoul(cp + 1, base, (unsigned long *)res);
		if (!ret)
			*res = -(*res);
	} else {
		ret = strict_strtoul(cp, base, (unsigned long *)res);
	}

	return ret;
}
EXPORT_SYMBOL(strict_strtol);

/**
 * strict_strtoull - convert a string to an unsigned long long strictly
 * @cp: The string to be converted
 * @base: The number base to use
 * @res: The converted result value
 *
 * strict_strtoull converts a string to an unsigned long long only if the
 * string is really an unsigned long long string, any string containing
 * any invalid char at the tail will be rejected and -EINVAL is returned,
 * only a newline char at the tail is acceptible because people generally
 * change a module parameter in the following way:
 *
 * 	echo 1024 > /sys/module/e1000/parameters/copybreak
 *
 * echo will append a newline to the tail of the string.
 *
 * It returns 0 if conversion is successful and *res is set to the converted
 * value, otherwise it returns -EINVAL and *res is set to 0.
 *
 * simple_strtoull just ignores the successive invalid characters and
 * return the converted value of prefix part of the string.
 */
int strict_strtoull(const char *cp, unsigned int base, unsigned long long *res)
{
	char *tail;
	unsigned long long val;
	size_t len;

	*res = 0;
	len = strlen(cp);
	if (len == 0)
		return -EINVAL;

	val = simple_strtoull(cp, &tail, base);
	if (tail == cp)
		return -EINVAL;
	if ((*tail == '\0') ||
		((len == (size_t)(tail - cp) + 1) && (*tail == '\n'))) {
		*res = val;
		return 0;
	}

	return -EINVAL;
}
EXPORT_SYMBOL(strict_strtoull);

/**
 * strict_strtoll - convert a string to a long long strictly
 * @cp: The string to be converted
 * @base: The number base to use
 * @res: The converted result value
 *
 * strict_strtoll is similiar to strict_strtoull, but it allows the first
 * character of a string is '-'.
 *
 * It returns 0 if conversion is successful and *res is set to the converted
 * value, otherwise it returns -EINVAL and *res is set to 0.
 */
int strict_strtoll(const char *cp, unsigned int base, long long *res)
{
	int ret;
	if (*cp == '-') {
		ret = strict_strtoull(cp + 1, base, (unsigned long long *)res);
		if (!ret)
			*res = -(*res);
	} else {
		ret = strict_strtoull(cp, base, (unsigned long long *)res);
	}

	return ret;
}
EXPORT_SYMBOL(strict_strtoll);

static int skip_atoi(const char **s)
{
	int i=0;

	while (isdigit(**s))
		i = i*10 + *((*s)++) - '0';
	return i;
}

/* Decimal conversion is by far the most typical, and is used
 * for /proc and /sys data. This directly impacts e.g. top performance
 * with many processes running. We optimize it for speed
 * using code from
 * http://www.cs.uiowa.edu/~jones/bcd/decimal.html
 * (with permission from the author, Douglas W. Jones). */

/* Formats correctly any integer in [0,99999].
 * Outputs from one to five digits depending on input.
 * On i386 gcc 4.1.2 -O2: ~250 bytes of code. */
static char* put_dec_trunc(char *buf, unsigned q)
{
	unsigned d3, d2, d1, d0;
	d1 = (q>>4) & 0xf;
	d2 = (q>>8) & 0xf;
	d3 = (q>>12);

	d0 = 6*(d3 + d2 + d1) + (q & 0xf);
	q = (d0 * 0xcd) >> 11;
	d0 = d0 - 10*q;
	*buf++ = d0 + '0'; /* least significant digit */
	d1 = q + 9*d3 + 5*d2 + d1;
	if (d1 != 0) {
		q = (d1 * 0xcd) >> 11;
		d1 = d1 - 10*q;
		*buf++ = d1 + '0'; /* next digit */

		d2 = q + 2*d2;
		if ((d2 != 0) || (d3 != 0)) {
			q = (d2 * 0xd) >> 7;
			d2 = d2 - 10*q;
			*buf++ = d2 + '0'; /* next digit */

			d3 = q + 4*d3;
			if (d3 != 0) {
				q = (d3 * 0xcd) >> 11;
				d3 = d3 - 10*q;
				*buf++ = d3 + '0';  /* next digit */
				if (q != 0)
					*buf++ = q + '0';  /* most sign. digit */
			}
		}
	}
	return buf;
}
/* Same with if's removed. Always emits five digits */
static char* put_dec_full(char *buf, unsigned q)
{
	/* BTW, if q is in [0,9999], 8-bit ints will be enough, */
	/* but anyway, gcc produces better code with full-sized ints */
	unsigned d3, d2, d1, d0;
	d1 = (q>>4) & 0xf;
	d2 = (q>>8) & 0xf;
	d3 = (q>>12);

	/* Possible ways to approx. divide by 10 */
	/* gcc -O2 replaces multiply with shifts and adds */
	// (x * 0xcd) >> 11: 11001101 - shorter code than * 0x67 (on i386)
	// (x * 0x67) >> 10:  1100111
	// (x * 0x34) >> 9:    110100 - same
	// (x * 0x1a) >> 8:     11010 - same
	// (x * 0x0d) >> 7:      1101 - same, shortest code (on i386)

	d0 = 6*(d3 + d2 + d1) + (q & 0xf);
	q = (d0 * 0xcd) >> 11;
	d0 = d0 - 10*q;
	*buf++ = d0 + '0';
	d1 = q + 9*d3 + 5*d2 + d1;
		q = (d1 * 0xcd) >> 11;
		d1 = d1 - 10*q;
		*buf++ = d1 + '0';

		d2 = q + 2*d2;
			q = (d2 * 0xd) >> 7;
			d2 = d2 - 10*q;
			*buf++ = d2 + '0';

			d3 = q + 4*d3;
				q = (d3 * 0xcd) >> 11; /* - shorter code */
				/* q = (d3 * 0x67) >> 10; - would also work */
				d3 = d3 - 10*q;
				*buf++ = d3 + '0';
					*buf++ = q + '0';
	return buf;
}

/* No inlining helps gcc to use registers better */
static char* put_dec(char *buf, unsigned long long num)
{
	while (1) {
		unsigned rem;
		if (num < 100000)
			return put_dec_trunc(buf, num);
		rem = do_div(num, 100000);
		buf = put_dec_full(buf, rem);
	}
}

#define ZEROPAD	1		/* pad with zero */
#define SIGN	2		/* unsigned/signed long */
#define PLUS	4		/* show plus */
#define SPACE	8		/* space if plus */
#define LEFT	16		/* left justified */
#define SMALL	32		/* Must be 32 == 0x20 */
#define SPECIAL	64		/* 0x */

enum format_type {
	FORMAT_TYPE_NONE, /* Just a string part */
	FORMAT_TYPE_WIDTH,
	FORMAT_TYPE_PRECISION,
	FORMAT_TYPE_CHAR,
	FORMAT_TYPE_STR,
	FORMAT_TYPE_PTR,
	FORMAT_TYPE_PERCENT_CHAR,
	FORMAT_TYPE_INVALID,
	FORMAT_TYPE_LONG_LONG,
	FORMAT_TYPE_ULONG,
	FORMAT_TYPE_LONG,
	FORMAT_TYPE_UBYTE,
	FORMAT_TYPE_BYTE,
	FORMAT_TYPE_USHORT,
	FORMAT_TYPE_SHORT,
	FORMAT_TYPE_UINT,
	FORMAT_TYPE_INT,
	FORMAT_TYPE_NRCHARS,
	FORMAT_TYPE_SIZE_T,
	FORMAT_TYPE_PTRDIFF
};

struct printf_spec {
	enum format_type	type;
	int			flags;		/* flags to number() */
	int			field_width;	/* width of output field */
	int			base;
	int			precision;	/* # of digits/chars */
	int			qualifier;
};

static char *number(char *buf, char *end, unsigned long long num,
			struct printf_spec spec)
{
	/* we are called with base 8, 10 or 16, only, thus don't need "G..."  */
	static const char digits[16] = "0123456789ABCDEF"; /* "GHIJKLMNOPQRSTUVWXYZ"; */

	char tmp[66];
	char sign;
	char locase;
	int need_pfx = ((spec.flags & SPECIAL) && spec.base != 10);
	int i;

	/* locase = 0 or 0x20. ORing digits or letters with 'locase'
	 * produces same digits or (maybe lowercased) letters */
	locase = (spec.flags & SMALL);
	if (spec.flags & LEFT)
		spec.flags &= ~ZEROPAD;
	sign = 0;
	if (spec.flags & SIGN) {
		if ((signed long long) num < 0) {
			sign = '-';
			num = - (signed long long) num;
			spec.field_width--;
		} else if (spec.flags & PLUS) {
			sign = '+';
			spec.field_width--;
		} else if (spec.flags & SPACE) {
			sign = ' ';
			spec.field_width--;
		}
	}
	if (need_pfx) {
		spec.field_width--;
		if (spec.base == 16)
			spec.field_width--;
	}

	/* generate full string in tmp[], in reverse order */
	i = 0;
	if (num == 0)
		tmp[i++] = '0';
	/* Generic code, for any base:
	else do {
		tmp[i++] = (digits[do_div(num,base)] | locase);
	} while (num != 0);
	*/
	else if (spec.base != 10) { /* 8 or 16 */
		int mask = spec.base - 1;
		int shift = 3;
		if (spec.base == 16) shift = 4;
		do {
			tmp[i++] = (digits[((unsigned char)num) & mask] | locase);
			num >>= shift;
		} while (num);
	} else { /* base 10 */
		i = put_dec(tmp, num) - tmp;
	}

	/* printing 100 using %2d gives "100", not "00" */
	if (i > spec.precision)
		spec.precision = i;
	/* leading space padding */
	spec.field_width -= spec.precision;
	if (!(spec.flags & (ZEROPAD+LEFT))) {
		while(--spec.field_width >= 0) {
			if (buf < end)
				*buf = ' ';
			++buf;
		}
	}
	/* sign */
	if (sign) {
		if (buf < end)
			*buf = sign;
		++buf;
	}
	/* "0x" / "0" prefix */
	if (need_pfx) {
		if (buf < end)
			*buf = '0';
		++buf;
		if (spec.base == 16) {
			if (buf < end)
				*buf = ('X' | locase);
			++buf;
		}
	}
	/* zero or space padding */
	if (!(spec.flags & LEFT)) {
		char c = (spec.flags & ZEROPAD) ? '0' : ' ';
		while (--spec.field_width >= 0) {
			if (buf < end)
				*buf = c;
			++buf;
		}
	}
	/* hmm even more zero padding? */
	while (i <= --spec.precision) {
		if (buf < end)
			*buf = '0';
		++buf;
	}
	/* actual digits of result */
	while (--i >= 0) {
		if (buf < end)
			*buf = tmp[i];
		++buf;
	}
	/* trailing space padding */
	while (--spec.field_width >= 0) {
		if (buf < end)
			*buf = ' ';
		++buf;
	}
	return buf;
}

static char *string(char *buf, char *end, char *s, struct printf_spec spec)
{
	int len, i;

	if ((unsigned long)s < PAGE_SIZE)
		s = "<NULL>";

	len = strnlen(s, spec.precision);

	if (!(spec.flags & LEFT)) {
		while (len < spec.field_width--) {
			if (buf < end)
				*buf = ' ';
			++buf;
		}
	}
	for (i = 0; i < len; ++i) {
		if (buf < end)
			*buf = *s;
		++buf; ++s;
	}
	while (len < spec.field_width--) {
		if (buf < end)
			*buf = ' ';
		++buf;
	}
	return buf;
}

static char *symbol_string(char *buf, char *end, void *ptr,
				struct printf_spec spec, char ext)
{
	unsigned long value = (unsigned long) ptr;
#ifdef CONFIG_KALLSYMS
	char sym[KSYM_SYMBOL_LEN];
	if (ext != 'f')
		sprint_symbol(sym, value);
	else
		kallsyms_lookup(value, NULL, NULL, NULL, sym);
	return string(buf, end, sym, spec);
#else
	spec.field_width = 2*sizeof(void *);
	spec.flags |= SPECIAL | SMALL | ZEROPAD;
	spec.base = 16;
	return number(buf, end, value, spec);
#endif
}



static char *ip4_addr_string(char *buf, char *end, uint8_t *addr,
				struct printf_spec spec)
{
	char ip4_addr[4 * 4]; /* (4 * 3 decimal digits), 3 dots and trailing zero */
	char temp[3];	/* hold each IP quad in reverse order */
	char *p = ip4_addr;
	int i, digits;

	for (i = 0; i < 4; i++) {
		digits = put_dec_trunc(temp, addr[i]) - temp;
		/* reverse the digits in the quad */
		while (digits--)
			*p++ = temp[digits];
		if (i != 3)
			*p++ = '.';
	}
	*p = '\0';
	spec.flags &= ~SPECIAL;

	return string(buf, end, ip4_addr, spec);
}

/*
 * Show a '%p' thing.  A kernel extension is that the '%p' is followed
 * by an extra set of alphanumeric characters that are extended format
 * specifiers.
 *
 * Right now we handle:
 *
 * - 'F' For symbolic function descriptor pointers with offset
 * - 'f' For simple symbolic function names without offset
 * - 'S' For symbolic direct pointers
 * - 'R' For a struct resource pointer, it prints the range of
 *       addresses (not the name nor the flags)
 * - 'M' For a 6-byte MAC address, it prints the address in the
 *       usual colon-separated hex notation
 * - 'I' [46] for IPv4/IPv6 addresses printed in the usual way (dot-separated
 *       decimal for v4 and colon separated network-order 16 bit hex for v6)
 * - 'i' [46] for 'raw' IPv4/IPv6 addresses, IPv6 omits the colons, IPv4 is
 *       currently the same
 *
 * Note: The difference between 'S' and 'F' is that on ia64 and ppc64
 * function pointers are really function descriptors, which contain a
 * pointer to the real address.
 */
static char *pointer(const char *fmt, char *buf, char *end, void *ptr,
			struct printf_spec spec)
{
	if (!ptr)
		return string(buf, end, "(null)", spec);

	switch (*fmt) {
	case 'S':
		return symbol_string(buf, end, ptr, spec, *fmt);
	case 'm':
		spec.flags |= SPECIAL;
		/* Fallthrough */
	case 'i':
		spec.flags |= SPECIAL;
		/* Fallthrough */
	case 'I':
		if (fmt[1] == '4')
			return ip4_addr_string(buf, end, ptr, spec);
		spec.flags &= ~SPECIAL;
		break;
	}
	spec.flags |= SMALL;
	if (spec.field_width == -1) {
		spec.field_width = 2*sizeof(void *);
		spec.flags |= ZEROPAD;
	}
	spec.base = 16;

	return number(buf, end, (unsigned long) ptr, spec);
}

/*
 * Helper function to decode printf style format.
 * Each call decode a token from the format and return the
 * number of characters read (or likely the delta where it wants
 * to go on the next call).
 * The decoded token is returned through the parameters
 *
 * 'h', 'l', or 'L' for integer fields
 * 'z' support added 23/7/1999 S.H.
 * 'z' changed to 'Z' --davidm 1/25/99
 * 't' added for ptrdiff_t
 *
 * @fmt: the format string
 * @type of the token returned
 * @flags: various flags such as +, -, # tokens..
 * @field_width: overwritten width
 * @base: base of the number (octal, hex, ...)
 * @precision: precision of a number
 * @qualifier: qualifier of a number (long, size_t, ...)
 */
static int format_decode(const char *fmt, struct printf_spec *spec)
{
	const char *start = fmt;

	/* we finished early by reading the field width */
	if (spec->type == FORMAT_TYPE_WIDTH) {
		if (spec->field_width < 0) {
			spec->field_width = -spec->field_width;
			spec->flags |= LEFT;
		}
		spec->type = FORMAT_TYPE_NONE;
		goto precision;
	}

	/* we finished early by reading the precision */
	if (spec->type == FORMAT_TYPE_PRECISION) {
		if (spec->precision < 0)
			spec->precision = 0;

		spec->type = FORMAT_TYPE_NONE;
		goto qualifier;
	}

	/* By default */
	spec->type = FORMAT_TYPE_NONE;

	for (; *fmt ; ++fmt) {
		if (*fmt == '%')
			break;
	}

	/* Return the current non-format string */
	if (fmt != start || !*fmt)
		return fmt - start;

	/* Process flags */
	spec->flags = 0;

	while (1) { /* this also skips first '%' */
		int found = 1;

		++fmt;

		switch (*fmt) {
		case '-': spec->flags |= LEFT;    break;
		case '+': spec->flags |= PLUS;    break;
		case ' ': spec->flags |= SPACE;   break;
		case '#': spec->flags |= SPECIAL; break;
		case '0': spec->flags |= ZEROPAD; break;
		default:  found = 0;
		}

		if (!found)
			break;
	}

	/* get field width */
	spec->field_width = -1;

	if (isdigit(*fmt))
		spec->field_width = skip_atoi(&fmt);
	else if (*fmt == '*') {
		/* it's the next argument */
		spec->type = FORMAT_TYPE_WIDTH;
		return ++fmt - start;
	}

precision:
	/* get the precision */
	spec->precision = -1;
	if (*fmt == '.') {
		++fmt;
		if (isdigit(*fmt)) {
			spec->precision = skip_atoi(&fmt);
			if (spec->precision < 0)
				spec->precision = 0;
		} else if (*fmt == '*') {
			/* it's the next argument */
			spec->type = FORMAT_TYPE_PRECISION;
			return ++fmt - start;
		}
	}

qualifier:
	/* get the conversion qualifier */
	spec->qualifier = -1;
	if (*fmt == 'h' || *fmt == 'l' || *fmt == 'L' ||
	    *fmt == 'Z' || *fmt == 'z' || *fmt == 't') {
		spec->qualifier = *fmt++;
		if (unlikely(spec->qualifier == *fmt)) {
			if (spec->qualifier == 'l') {
				spec->qualifier = 'L';
				++fmt;
			} else if (spec->qualifier == 'h') {
				spec->qualifier = 'H';
				++fmt;
			}
		}
	}

	/* default base */
	spec->base = 10;
	switch (*fmt) {
	case 'c':
		spec->type = FORMAT_TYPE_CHAR;
		return ++fmt - start;

	case 's':
		spec->type = FORMAT_TYPE_STR;
		return ++fmt - start;

	case 'p':
		spec->type = FORMAT_TYPE_PTR;
		return fmt - start;
		/* skip alnum */

	case 'n':
		spec->type = FORMAT_TYPE_NRCHARS;
		return ++fmt - start;

	case '%':
		spec->type = FORMAT_TYPE_PERCENT_CHAR;
		return ++fmt - start;

	/* integer number formats - set up the flags and "break" */
	case 'o':
		spec->base = 8;
		break;

	case 'x':
		spec->flags |= SMALL;

	case 'X':
		spec->base = 16;
		break;

	case 'd':
	case 'i':
		spec->flags |= SIGN;
	case 'u':
		break;

	default:
		spec->type = FORMAT_TYPE_INVALID;
		return fmt - start;
	}

	if (spec->qualifier == 'L')
		spec->type = FORMAT_TYPE_LONG_LONG;
	else if (spec->qualifier == 'l') {
		if (spec->flags & SIGN)
			spec->type = FORMAT_TYPE_LONG;
		else
			spec->type = FORMAT_TYPE_ULONG;
	} else if (spec->qualifier == 'Z' || spec->qualifier == 'z') {
		spec->type = FORMAT_TYPE_SIZE_T;
	} else if (spec->qualifier == 't') {
		spec->type = FORMAT_TYPE_PTRDIFF;
	} else if (spec->qualifier == 'H') {
		if (spec->flags & SIGN)
			spec->type = FORMAT_TYPE_BYTE;
		else
			spec->type = FORMAT_TYPE_UBYTE;
	} else if (spec->qualifier == 'h') {
		if (spec->flags & SIGN)
			spec->type = FORMAT_TYPE_SHORT;
		else
			spec->type = FORMAT_TYPE_USHORT;
	} else {
		if (spec->flags & SIGN)
			spec->type = FORMAT_TYPE_INT;
		else
			spec->type = FORMAT_TYPE_UINT;
	}

	return ++fmt - start;
}

/**
 * vsnprintf - Format a string and place it in a buffer
 * @buf: The buffer to place the result into
 * @size: The size of the buffer, including the trailing null space
 * @fmt: The format string to use
 * @args: Arguments for the format string
 *
 * This function follows C99 vsnprintf, but has some extensions:
 * %pS output the name of a text symbol
 * %pF output the name of a function pointer with its offset
 * %pf output the name of a function pointer without its offset
 * %pR output the address range in a struct resource
 *
 * The return value is the number of characters which would
 * be generated for the given input, excluding the trailing
 * '\0', as per ISO C99. If you want to have the exact
 * number of characters written into @buf as return value
 * (not including the trailing '\0'), use vscnprintf(). If the
 * return is greater than or equal to @size, the resulting
 * string is truncated.
 *
 * Call this function if you are already dealing with a va_list.
 * You probably want snprintf() instead.
 */
int vsnprintf(char *buf, size_t size, const char *fmt, va_list args)
{
	unsigned long long num;
	char *str, *end, c;
	int read;
	struct printf_spec spec = {0};

	/* Reject out-of-range values early.  Large positive sizes are
	   used for unknown buffer sizes. */
	if (unlikely((int) size < 0)) {
		/* There can be only one.. */
		static char warn = 1;
		WARN_ON(warn);
		warn = 0;
		return 0;
	}

	str = buf;
	end = buf + size;

	/* Make sure end is always >= buf */
	if (end < buf) {
		end = ((void *)-1);
		size = end - buf;
	}

	while (*fmt) {
		const char *old_fmt = fmt;

		read = format_decode(fmt, &spec);

		fmt += read;

		switch (spec.type) {
		case FORMAT_TYPE_NONE: {
			int copy = read;
			if (str < end) {
				if (copy > end - str)
					copy = end - str;
				memcpy(str, old_fmt, copy);
			}
			str += read;
			break;
		}

		case FORMAT_TYPE_WIDTH:
			spec.field_width = va_arg(args, int);
			break;

		case FORMAT_TYPE_PRECISION:
			spec.precision = va_arg(args, int);
			break;

		case FORMAT_TYPE_CHAR:
			if (!(spec.flags & LEFT)) {
				while (--spec.field_width > 0) {
					if (str < end)
						*str = ' ';
					++str;

				}
			}
			c = (unsigned char) va_arg(args, int);
			if (str < end)
				*str = c;
			++str;
			while (--spec.field_width > 0) {
				if (str < end)
					*str = ' ';
				++str;
			}
			break;

		case FORMAT_TYPE_STR:
			str = string(str, end, va_arg(args, char *), spec);
			break;

		case FORMAT_TYPE_PTR:
			str = pointer(fmt+1, str, end, va_arg(args, void *),
				      spec);
			while (isalnum(*fmt))
				fmt++;
			break;

		case FORMAT_TYPE_PERCENT_CHAR:
			if (str < end)
				*str = '%';
			++str;
			break;

		case FORMAT_TYPE_INVALID:
			if (str < end)
				*str = '%';
			++str;
			break;

		case FORMAT_TYPE_NRCHARS: {
			int qualifier = spec.qualifier;

			if (qualifier == 'l') {
				long *ip = va_arg(args, long *);
				*ip = (str - buf);
			} else if (qualifier == 'Z' ||
					qualifier == 'z') {
				size_t *ip = va_arg(args, size_t *);
				*ip = (str - buf);
			} else {
				int *ip = va_arg(args, int *);
				*ip = (str - buf);
			}
			break;
		}

		default:
			switch (spec.type) {
			case FORMAT_TYPE_LONG_LONG:
				num = va_arg(args, long long);
				break;
			case FORMAT_TYPE_ULONG:
				num = va_arg(args, unsigned long);
				break;
			case FORMAT_TYPE_LONG:
				num = va_arg(args, long);
				break;
			case FORMAT_TYPE_SIZE_T:
				num = va_arg(args, size_t);
				break;
			case FORMAT_TYPE_PTRDIFF:
				num = va_arg(args, unsigned long);
				break;
			case FORMAT_TYPE_UBYTE:
				num = (unsigned char) va_arg(args, int);
				break;
			case FORMAT_TYPE_BYTE:
				num = (signed char) va_arg(args, int);
				break;
			case FORMAT_TYPE_USHORT:
				num = (unsigned short) va_arg(args, int);
				break;
			case FORMAT_TYPE_SHORT:
				num = (short) va_arg(args, int);
				break;
			case FORMAT_TYPE_INT:
				num = (int) va_arg(args, int);
				break;
			default:
				num = va_arg(args, unsigned int);
			}

			str = number(str, end, num, spec);
		}
	}

	if (size > 0) {
		if (str < end)
			*str = '\0';
		else
			end[-1] = '\0';
	}

	/* the trailing null byte doesn't count towards the total */
	return str-buf;

}
EXPORT_SYMBOL(vsnprintf);

/**
 * vscnprintf - Format a string and place it in a buffer
 * @buf: The buffer to place the result into
 * @size: The size of the buffer, including the trailing null space
 * @fmt: The format string to use
 * @args: Arguments for the format string
 *
 * The return value is the number of characters which have been written into
 * the @buf not including the trailing '\0'. If @size is <= 0 the function
 * returns 0.
 *
 * Call this function if you are already dealing with a va_list.
 * You probably want scnprintf() instead.
 *
 * See the vsnprintf() documentation for format string extensions over C99.
 */
int vscnprintf(char *buf, size_t size, const char *fmt, va_list args)
{
	int i;

	i=vsnprintf(buf,size,fmt,args);
	return (i >= size) ? (size - 1) : i;
}
EXPORT_SYMBOL(vscnprintf);

/**
 * snprintf - Format a string and place it in a buffer
 * @buf: The buffer to place the result into
 * @size: The size of the buffer, including the trailing null space
 * @fmt: The format string to use
 * @...: Arguments for the format string
 *
 * The return value is the number of characters which would be
 * generated for the given input, excluding the trailing null,
 * as per ISO C99.  If the return is greater than or equal to
 * @size, the resulting string is truncated.
 *
 * See the vsnprintf() documentation for format string extensions over C99.
 */
int snprintf(char * buf, size_t size, const char *fmt, ...)
{
	va_list args;
	int i;

	va_start(args, fmt);
	i=vsnprintf(buf,size,fmt,args);
	va_end(args);
	return i;
}
EXPORT_SYMBOL(snprintf);

/**
 * scnprintf - Format a string and place it in a buffer
 * @buf: The buffer to place the result into
 * @size: The size of the buffer, including the trailing null space
 * @fmt: The format string to use
 * @...: Arguments for the format string
 *
 * The return value is the number of characters written into @buf not including
 * the trailing '\0'. If @size is <= 0 the function returns 0.
 */

int scnprintf(char * buf, size_t size, const char *fmt, ...)
{
	va_list args;
	int i;

	va_start(args, fmt);
	i = vsnprintf(buf, size, fmt, args);
	va_end(args);
	return (i >= size) ? (size - 1) : i;
}
EXPORT_SYMBOL(scnprintf);

/**
 * vsprintf - Format a string and place it in a buffer
 * @buf: The buffer to place the result into
 * @fmt: The format string to use
 * @args: Arguments for the format string
 *
 * The function returns the number of characters written
 * into @buf. Use vsnprintf() or vscnprintf() in order to avoid
 * buffer overflows.
 *
 * Call this function if you are already dealing with a va_list.
 * You probably want sprintf() instead.
 *
 * See the vsnprintf() documentation for format string extensions over C99.
 */
int vsprintf(char *buf, const char *fmt, va_list args)
{
	return vsnprintf(buf, INT_MAX, fmt, args);
}
EXPORT_SYMBOL(vsprintf);

/**
 * sprintf - Format a string and place it in a buffer
 * @buf: The buffer to place the result into
 * @fmt: The format string to use
 * @...: Arguments for the format string
 *
 * The function returns the number of characters written
 * into @buf. Use snprintf() or scnprintf() in order to avoid
 * buffer overflows.
 *
 * See the vsnprintf() documentation for format string extensions over C99.
 */
int sprintf(char * buf, const char *fmt, ...)
{
	va_list args;
	int i;

	va_start(args, fmt);
	i=vsnprintf(buf, INT_MAX, fmt, args);
	va_end(args);
	return i;
}
EXPORT_SYMBOL(sprintf);

#ifdef CONFIG_BINARY_PRINTF
/*
 * bprintf service:
 * vbin_printf() - VA arguments to binary data
 * bstr_printf() - Binary data to text string
 */

/**
 * vbin_printf - Parse a format string and place args' binary value in a buffer
 * @bin_buf: The buffer to place args' binary value
 * @size: The size of the buffer(by words(32bits), not characters)
 * @fmt: The format string to use
 * @args: Arguments for the format string
 *
 * The format follows C99 vsnprintf, except %n is ignored, and its argument
 * is skiped.
 *
 * The return value is the number of words(32bits) which would be generated for
 * the given input.
 *
 * NOTE:
 * If the return value is greater than @size, the resulting bin_buf is NOT
 * valid for bstr_printf().
 */
int vbin_printf(u32 *bin_buf, size_t size, const char *fmt, va_list args)
{
	struct printf_spec spec = {0};
	char *str, *end;
	int read;

	str = (char *)bin_buf;
	end = (char *)(bin_buf + size);

#define save_arg(type)							\
do {									\
	if (sizeof(type) == 8) {					\
		unsigned long long value;				\
		str = PTR_ALIGN(str, sizeof(u32));			\
		value = va_arg(args, unsigned long long);		\
		if (str + sizeof(type) <= end) {			\
			*(u32 *)str = *(u32 *)&value;			\
			*(u32 *)(str + 4) = *((u32 *)&value + 1);	\
		}							\
	} else {							\
		unsigned long value;					\
		str = PTR_ALIGN(str, sizeof(type));			\
		value = va_arg(args, int);				\
		if (str + sizeof(type) <= end)				\
			*(typeof(type) *)str = (type)value;		\
	}								\
	str += sizeof(type);						\
} while (0)


	while (*fmt) {
		read = format_decode(fmt, &spec);

		fmt += read;

		switch (spec.type) {
		case FORMAT_TYPE_NONE:
			break;

		case FORMAT_TYPE_WIDTH:
		case FORMAT_TYPE_PRECISION:
			save_arg(int);
			break;

		case FORMAT_TYPE_CHAR:
			save_arg(char);
			break;

		case FORMAT_TYPE_STR: {
			const char *save_str = va_arg(args, char *);
			size_t len;
			if ((unsigned long)save_str > (unsigned long)-PAGE_SIZE
					|| (unsigned long)save_str < PAGE_SIZE)
				save_str = "<NULL>";
			len = strlen(save_str);
			if (str + len + 1 < end)
				memcpy(str, save_str, len + 1);
			str += len + 1;
			break;
		}

		case FORMAT_TYPE_PTR:
			save_arg(void *);
			/* skip all alphanumeric pointer suffixes */
			while (isalnum(*fmt))
				fmt++;
			break;

		case FORMAT_TYPE_PERCENT_CHAR:
			break;

		case FORMAT_TYPE_INVALID:
			break;

		case FORMAT_TYPE_NRCHARS: {
			/* skip %n 's argument */
			int qualifier = spec.qualifier;
			void *skip_arg;
			if (qualifier == 'l')
				skip_arg = va_arg(args, long *);
			else if (qualifier == 'Z' || qualifier == 'z')
				skip_arg = va_arg(args, size_t *);
			else
				skip_arg = va_arg(args, int *);
			break;
		}

		default:
			switch (spec.type) {

			case FORMAT_TYPE_LONG_LONG:
				save_arg(long long);
				break;
			case FORMAT_TYPE_ULONG:
			case FORMAT_TYPE_LONG:
				save_arg(unsigned long);
				break;
			case FORMAT_TYPE_SIZE_T:
				save_arg(size_t);
				break;
			case FORMAT_TYPE_PTRDIFF:
				save_arg(ptrdiff_t);
				break;
			case FORMAT_TYPE_UBYTE:
			case FORMAT_TYPE_BYTE:
				save_arg(char);
				break;
			case FORMAT_TYPE_USHORT:
			case FORMAT_TYPE_SHORT:
				save_arg(short);
				break;
			default:
				save_arg(int);
			}
		}
	}
	return (u32 *)(PTR_ALIGN(str, sizeof(u32))) - bin_buf;

#undef save_arg
}
EXPORT_SYMBOL_GPL(vbin_printf);

/**
 * bstr_printf - Format a string from binary arguments and place it in a buffer
 * @buf: The buffer to place the result into
 * @size: The size of the buffer, including the trailing null space
 * @fmt: The format string to use
 * @bin_buf: Binary arguments for the format string
 *
 * This function like C99 vsnprintf, but the difference is that vsnprintf gets
 * arguments from stack, and bstr_printf gets arguments from @bin_buf which is
 * a binary buffer that generated by vbin_printf.
 *
 * The format follows C99 vsnprintf, but has some extensions:
 * %pS output the name of a text symbol
 * %pF output the name of a function pointer with its offset
 * %pf output the name of a function pointer without its offset
 * %pR output the address range in a struct resource
 * %n is ignored
 *
 * The return value is the number of characters which would
 * be generated for the given input, excluding the trailing
 * '\0', as per ISO C99. If you want to have the exact
 * number of characters written into @buf as return value
 * (not including the trailing '\0'), use vscnprintf(). If the
 * return is greater than or equal to @size, the resulting
 * string is truncated.
 */
int bstr_printf(char *buf, size_t size, const char *fmt, const u32 *bin_buf)
{
	unsigned long long num;
	char *str, *end, c;
	const char *args = (const char *)bin_buf;

	struct printf_spec spec = {0};

	if (unlikely((int) size < 0)) {
		/* There can be only one.. */
		static char warn = 1;
		WARN_ON(warn);
		warn = 0;
		return 0;
	}

	str = buf;
	end = buf + size;

#define get_arg(type)							\
({									\
	typeof(type) value;						\
	if (sizeof(type) == 8) {					\
		args = PTR_ALIGN(args, sizeof(u32));			\
		*(u32 *)&value = *(u32 *)args;				\
		*((u32 *)&value + 1) = *(u32 *)(args + 4);		\
	} else {							\
		args = PTR_ALIGN(args, sizeof(type));			\
		value = *(typeof(type) *)args;				\
	}								\
	args += sizeof(type);						\
	value;								\
})

	/* Make sure end is always >= buf */
	if (end < buf) {
		end = ((void *)-1);
		size = end - buf;
	}

	while (*fmt) {
		int read;
		const char *old_fmt = fmt;

		read = format_decode(fmt, &spec);

		fmt += read;

		switch (spec.type) {
		case FORMAT_TYPE_NONE: {
			int copy = read;
			if (str < end) {
				if (copy > end - str)
					copy = end - str;
				memcpy(str, old_fmt, copy);
			}
			str += read;
			break;
		}

		case FORMAT_TYPE_WIDTH:
			spec.field_width = get_arg(int);
			break;

		case FORMAT_TYPE_PRECISION:
			spec.precision = get_arg(int);
			break;

		case FORMAT_TYPE_CHAR:
			if (!(spec.flags & LEFT)) {
				while (--spec.field_width > 0) {
					if (str < end)
						*str = ' ';
					++str;
				}
			}
			c = (unsigned char) get_arg(char);
			if (str < end)
				*str = c;
			++str;
			while (--spec.field_width > 0) {
				if (str < end)
					*str = ' ';
				++str;
			}
			break;

		case FORMAT_TYPE_STR: {
			const char *str_arg = args;
			size_t len = strlen(str_arg);
			args += len + 1;
			str = string(str, end, (char *)str_arg, spec);
			break;
		}

		case FORMAT_TYPE_PTR:
			str = pointer(fmt+1, str, end, get_arg(void *), spec);
			while (isalnum(*fmt))
				fmt++;
			break;

		case FORMAT_TYPE_PERCENT_CHAR:
			if (str < end)
				*str = '%';
			++str;
			break;

		case FORMAT_TYPE_INVALID:
			if (str < end)
				*str = '%';
			++str;
			break;

		case FORMAT_TYPE_NRCHARS:
			/* skip */
			break;

		default:
			switch (spec.type) {

			case FORMAT_TYPE_LONG_LONG:
				num = get_arg(long long);
				break;
			case FORMAT_TYPE_ULONG:
				num = get_arg(unsigned long);
				break;
			case FORMAT_TYPE_LONG:
				num = get_arg(unsigned long);
				break;
			case FORMAT_TYPE_SIZE_T:
				num = get_arg(size_t);
				break;
			case FORMAT_TYPE_PTRDIFF:
				num = get_arg(ptrdiff_t);
				break;
			case FORMAT_TYPE_UBYTE:
				num = get_arg(unsigned char);
				break;
			case FORMAT_TYPE_BYTE:
				num = get_arg(signed char);
				break;
			case FORMAT_TYPE_USHORT:
				num = get_arg(unsigned short);
				break;
			case FORMAT_TYPE_SHORT:
				num = get_arg(short);
				break;
			case FORMAT_TYPE_UINT:
				num = get_arg(unsigned int);
				break;
			default:
				num = get_arg(int);
			}

			str = number(str, end, num, spec);
		}
	}

	if (size > 0) {
		if (str < end)
			*str = '\0';
		else
			end[-1] = '\0';
	}

#undef get_arg

	/* the trailing null byte doesn't count towards the total */
	return str - buf;
}
EXPORT_SYMBOL_GPL(bstr_printf);

/**
 * bprintf - Parse a format string and place args' binary value in a buffer
 * @bin_buf: The buffer to place args' binary value
 * @size: The size of the buffer(by words(32bits), not characters)
 * @fmt: The format string to use
 * @...: Arguments for the format string
 *
 * The function returns the number of words(u32) written
 * into @bin_buf.
 */
int bprintf(u32 *bin_buf, size_t size, const char *fmt, ...)
{
	va_list args;
	int ret;

	va_start(args, fmt);
	ret = vbin_printf(bin_buf, size, fmt, args);
	va_end(args);
	return ret;
}
EXPORT_SYMBOL_GPL(bprintf);

#endif /* CONFIG_BINARY_PRINTF */

/**
 * vsscanf - Unformat a buffer into a list of arguments
 * @buf:	input buffer
 * @fmt:	format of buffer
 * @args:	arguments
 */
int vsscanf(const char * buf, const char * fmt, va_list args)
{
	const char *str = buf;
	char *next;
	char digit;
	int num = 0;
	int qualifier;
	int base;
	int field_width;
	int is_sign = 0;

	while(*fmt && *str) {
		/* skip any white space in format */
		/* white space in format matchs any amount of
		 * white space, including none, in the input.
		 */
		if (isspace(*fmt)) {
			while (isspace(*fmt))
				++fmt;
			while (isspace(*str))
				++str;
		}

		/* anything that is not a conversion must match exactly */
		if (*fmt != '%' && *fmt) {
			if (*fmt++ != *str++)
				break;
			continue;
		}

		if (!*fmt)
			break;
		++fmt;
		
		/* skip this conversion.
		 * advance both strings to next white space
		 */
		if (*fmt == '*') {
			while (!isspace(*fmt) && *fmt)
				fmt++;
			while (!isspace(*str) && *str)
				str++;
			continue;
		}

		/* get field width */
		field_width = -1;
		if (isdigit(*fmt))
			field_width = skip_atoi(&fmt);

		/* get conversion qualifier */
		qualifier = -1;
		if (*fmt == 'h' || *fmt == 'l' || *fmt == 'L' ||
		    *fmt == 'Z' || *fmt == 'z') {
			qualifier = *fmt++;
			if (unlikely(qualifier == *fmt)) {
				if (qualifier == 'h') {
					qualifier = 'H';
					fmt++;
				} else if (qualifier == 'l') {
					qualifier = 'L';
					fmt++;
				}
			}
		}
		base = 10;
		is_sign = 0;

		if (!*fmt || !*str)
			break;

		switch(*fmt++) {
		case 'c':
		{
			char *s = (char *) va_arg(args,char*);
			if (field_width == -1)
				field_width = 1;
			do {
				*s++ = *str++;
			} while (--field_width > 0 && *str);
			num++;
		}
		continue;
		case 's':
		{
			char *s = (char *) va_arg(args, char *);
			if(field_width == -1)
				field_width = INT_MAX;
			/* first, skip leading white space in buffer */
			while (isspace(*str))
				str++;

			/* now copy until next white space */
			while (*str && !isspace(*str) && field_width--) {
				*s++ = *str++;
			}
			*s = '\0';
			num++;
		}
		continue;
		case 'n':
			/* return number of characters read so far */
		{
			int *i = (int *)va_arg(args,int*);
			*i = str - buf;
		}
		continue;
		case 'o':
			base = 8;
			break;
		case 'x':
		case 'X':
			base = 16;
			break;
		case 'i':
                        base = 0;
		case 'd':
			is_sign = 1;
		case 'u':
			break;
		case '%':
			/* looking for '%' in str */
			if (*str++ != '%') 
				return num;
			continue;
		default:
			/* invalid format; stop here */
			return num;
		}

		/* have some sort of integer conversion.
		 * first, skip white space in buffer.
		 */
		while (isspace(*str))
			str++;

		digit = *str;
		if (is_sign && digit == '-')
			digit = *(str + 1);

		if (!digit
                    || (base == 16 && !isxdigit(digit))
                    || (base == 10 && !isdigit(digit))
                    || (base == 8 && (!isdigit(digit) || digit > '7'))
                    || (base == 0 && !isdigit(digit)))
				break;

		switch(qualifier) {
		case 'H':	/* that's 'hh' in format */
			if (is_sign) {
				signed char *s = (signed char *) va_arg(args,signed char *);
				*s = (signed char) simple_strtol(str,&next,base);
			} else {
				unsigned char *s = (unsigned char *) va_arg(args, unsigned char *);
				*s = (unsigned char) simple_strtoul(str, &next, base);
			}
			break;
		case 'h':
			if (is_sign) {
				short *s = (short *) va_arg(args,short *);
				*s = (short) simple_strtol(str,&next,base);
			} else {
				unsigned short *s = (unsigned short *) va_arg(args, unsigned short *);
				*s = (unsigned short) simple_strtoul(str, &next, base);
			}
			break;
		case 'l':
			if (is_sign) {
				long *l = (long *) va_arg(args,long *);
				*l = simple_strtol(str,&next,base);
			} else {
				unsigned long *l = (unsigned long*) va_arg(args,unsigned long*);
				*l = simple_strtoul(str,&next,base);
			}
			break;
		case 'L':
			if (is_sign) {
				long long *l = (long long*) va_arg(args,long long *);
				*l = simple_strtoll(str,&next,base);
			} else {
				unsigned long long *l = (unsigned long long*) va_arg(args,unsigned long long*);
				*l = simple_strtoull(str,&next,base);
			}
			break;
		case 'Z':
		case 'z':
		{
			size_t *s = (size_t*) va_arg(args,size_t*);
			*s = (size_t) simple_strtoul(str,&next,base);
		}
		break;
		default:
			if (is_sign) {
				int *i = (int *) va_arg(args, int*);
				*i = (int) simple_strtol(str,&next,base);
			} else {
				unsigned int *i = (unsigned int*) va_arg(args, unsigned int*);
				*i = (unsigned int) simple_strtoul(str,&next,base);
			}
			break;
		}
		num++;

		if (!next)
			break;
		str = next;
	}

	/*
	 * Now we've come all the way through so either the input string or the
	 * format ended. In the former case, there can be a %n at the current
	 * position in the format that needs to be filled.
	 */
	if (*fmt == '%' && *(fmt + 1) == 'n') {
		int *p = (int *)va_arg(args, int *);
		*p = str - buf;
	}

	return num;
}
EXPORT_SYMBOL(vsscanf);

/**
 * sscanf - Unformat a buffer into a list of arguments
 * @buf:	input buffer
 * @fmt:	formatting of buffer
 * @...:	resulting arguments
 */
int sscanf(const char * buf, const char * fmt, ...)
{
	va_list args;
	int i;

	va_start(args,fmt);
	i = vsscanf(buf,fmt,args);
	va_end(args);
	return i;
}
EXPORT_SYMBOL(sscanf);
