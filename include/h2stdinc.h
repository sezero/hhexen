/*
	h2stdinc.h
	includes the minimum necessary stdc headers,
	defines common and / or missing types.
*/

#ifndef __H2STDINC_H
#define __H2STDINC_H

#include "config.h"

#if defined(_WIN32) && !defined(WIN32_LEAN_AND_MEAN)
#define WIN32_LEAN_AND_MEAN
#endif	/* WIN32_LEAN_AND_MEAN */

#include <sys/types.h>
#include <stddef.h>
#include <limits.h>

/* We expect these type sizes:
   sizeof (char)	== 1
   sizeof (short)	== 2
   sizeof (int)		== 4
   sizeof (float)	== 4
   sizeof (long)	== 4 / 8
   sizeof (pointer *)	== 4 / 8
   For this, we need stdint.h (or inttypes.h)
   FIXME: On some platforms, only inttypes.h is available.
   FIXME: Properly replace certain short and int usage
	  with int16_t and int32_t.
 */
#if defined(HAVE_STDINT_H)
# include <stdint.h>
#endif
#if defined(HAVE_INTTYPES_H)
# include <inttypes.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#ifdef HAVE_STRINGS_H
#include <strings.h>	/* strcasecmp and strncasecmp	*/
#endif

/*==========================================================================*/

#ifndef NULL
#if defined(__cplusplus)
#define	NULL		0
#else
#define	NULL		((void *)0)
#endif
#endif

#define	H2MAXCHAR	((char)0x7f)
#define	H2MAXSHORT	((short)0x7fff)
#define	H2MAXINT	((int)0x7fffffff)	/* max positive 32-bit integer */
#define	H2MINCHAR	((char)0x80)
#define	H2MINSHORT	((short)0x8000)
#define	H2MININT	((int)0x80000000)	/* max negative 32-bit integer */

/* Make sure the types really have the right
 * sizes: These macros are from SDL headers.
 */
#define	COMPILE_TIME_ASSERT(name, x)	\
	typedef int dummy_ ## name[(x) * 2 - 1]

COMPILE_TIME_ASSERT(char, sizeof(char) == 1);
COMPILE_TIME_ASSERT(float, sizeof(float) == 4);
COMPILE_TIME_ASSERT(long, sizeof(long) >= 4);
COMPILE_TIME_ASSERT(int, sizeof(int) == 4);
COMPILE_TIME_ASSERT(short, sizeof(short) == 2);

/* make sure enums are the size of ints for structure packing */
typedef enum {
	THE_DUMMY_VALUE
} THE_DUMMY_ENUM;
COMPILE_TIME_ASSERT(enum, sizeof(THE_DUMMY_ENUM) == sizeof(int));


/*==========================================================================*/

typedef unsigned char		byte;

/* some structures have boolean members and the x86 asm code expect
 * those members to be 4 bytes long.  i.e.: boolean must be 32 bits.  */
typedef int	boolean;
#undef true
#undef false
#if !defined(__cplusplus)
#if defined __STDC_VERSION__ && (__STDC_VERSION__ >= 199901L)
#include <stdbool.h>
#else
enum {
	false = 0,
	true  = 1
};
#endif
#endif /* */
COMPILE_TIME_ASSERT(falsehood, ((1 != 1) == false));
COMPILE_TIME_ASSERT(truth, ((1 == 1) == true));
COMPILE_TIME_ASSERT(boolean, sizeof(boolean) == 4);

/*==========================================================================*/

/* math */
#define	FRACBITS		16
#define	FRACUNIT		(1 << FRACBITS)

typedef int	fixed_t;


/*==========================================================================*/

/* compatibility with M$ types */
#if !defined(_WIN32)
#define	PASCAL
#define	FAR
#define	APIENTRY
#endif	/* ! WINDOWS */

/*==========================================================================*/

/* function attributes, etc */

#if defined(__GNUC__)
#define FUNC_PRINTF(x,y)	__attribute__((__format__(__printf__,x,y)))
#else
#define FUNC_PRINTF(x,y)
#endif

/* argument format attributes for function pointers are supported for gcc >= 3.1 */
#if defined(__GNUC__) && (__GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ > 0))
#define FUNCP_PRINTF	FUNC_PRINTF
#else
#define FUNCP_PRINTF(x,y)
#endif

/* llvm's optnone function attribute started with clang-3.5.0 */
#if defined(__clang__) && \
           (__clang_major__ > 3 || (__clang_major__ == 3 && __clang_minor__ >= 5))
#define FUNC_NO_OPTIMIZE	__attribute__((__optnone__))
/* function optimize attribute is added starting with gcc 4.4.0 */
#elif defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ > 3))
#define FUNC_NO_OPTIMIZE	__attribute__((__optimize__("0")))
#else
#define FUNC_NO_OPTIMIZE
#endif

#if defined(__GNUC__)
#define FUNC_NORETURN	__attribute__((__noreturn__))
#elif defined(_MSC_VER) && (_MSC_VER >= 1200)
#define FUNC_NORETURN		__declspec(noreturn)
#elif defined(__WATCOMC__)
#define FUNC_NORETURN /* use the 'aborts' aux pragma */
#else
#define FUNC_NORETURN
#endif

#if defined(__GNUC__)
#define FUNC_CONST	__attribute__((__const__))
#else
#define FUNC_CONST
#endif

#if defined(__GNUC__) && ((__GNUC__ > 3) || (__GNUC__ == 3 && __GNUC_MINOR__ >= 1))
#define FUNC_NOINLINE	__attribute__((__noinline__))
#elif defined(_MSC_VER) && (_MSC_VER >= 1300)
#define FUNC_NOINLINE		__declspec(noinline)
#else
#define FUNC_NOINLINE
#endif

#if defined(__GNUC__) && ((__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 5))
#define FUNC_NOCLONE	__attribute__((__noclone__))
#else
#define FUNC_NOCLONE
#endif

#if defined(__GNUC__) || defined(__clang__)
#define STRUCT_PACKED	__attribute__((__packed__))
#else
#define STRUCT_PACKED
#endif

#if defined(_MSC_VER) && !defined(__cplusplus)
#define inline __inline
#endif	/* _MSC_VER */

/*==========================================================================*/

/* Provide a substitute for offsetof() if we don't have one.
 * This variant works on most (but not *all*) systems...
 */
#ifndef offsetof
#define offsetof(t,m) ((size_t)&(((t *)0)->m))
#endif

/*==========================================================================*/


#endif	/* __H2STDINC_H */

