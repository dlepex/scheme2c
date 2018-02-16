/* base.h
 * - scheme translator & interpreter
 * author: denis.lepekhin@gmail.com
 * created on: 2009
 */

#ifndef BASE_H_
#define BASE_H_

#include <ctype.h>
#include <wchar.h>
#include <complex.h>
#include <stdbool.h>
#include <assert.h>
#include <ustring.h>
#include <ustdio.h>

#include <malloc.h>
#include <stdlib.h>


#define _GNU_SOURCE
#define __USE_GNU
#include <string.h>
#include <math.h>


 // basic exceptions 100, 200
#define EXCEP_INDEX_OUT_OF_RANGE 140
#define EXCEP_ASSERT             141
#define EXCEP_UNSUPPORTED_OP     142
#define EXCEP_IMPROPER_LIST      143

#define DECL_AUTO_PCHAR(name, size) char name[size]; *name=0
#define DECL_VOLAUTO_PCHAR(name,size)  volatile DECL_AUTO_PCHAR(name, size)

#define min(x,y) (((x) < (y))? (x):(y))
#define max(x,y) (((x) > (y))? (x):(y))
#define abs(x)    (((x) < 0) ? -(x) : (x))

/**\defgroup Types Basic types */
// @{
typedef unsigned char byte;
typedef unsigned long ulong;
typedef double complex complex_t;
typedef unsigned int uint;
typedef UChar32 bigchar;




//#define CHR_		_CHR_(0x)

// @}


#define MAX_FILE_PATH 528



#define global_once_preamble() \
	static volatile bool __g_once = false; \
	if(__g_once) {return;} else {__g_once=true;}

#define until(cond)     while(!(cond))
#define unless(cond)    if(!(cond))
#define imply(cause, effect)  (!(cause) || (effect))
#define goodstr(str)   ((str != 0) && (*str != 0))
#define inlen_0(x, len)     (  ((x) >= 0 ) && ((x) < (len)) )

#define _llshft_(x, shift)   ( ((uint64_t)(x)) << (shift) )

#define _lbit_(x)			(1ul << (x))
#define _lmask_(count)  ((1ul << (count)) - 1)
#define align_8(size)   (((ulong)(size) + 7) & 0xFFFFFFFFFFFFFFF8)

#define _ubit_(x)			(1u << (x))
#define _umask_(count)  ((1u << (count)) - 1)

#define _getmasked_(src, msk, shift)   		(((src) & (msk)) >> (shift))
// type of val must = type of src!
#define _setmasked_(src, val, msk, shift)    (((src) & ~(msk)) | (((val) << (shift))  & (msk)))

#define __newstruct(_name_struct) ((_name_struct*)tmalloc(sizeof(_name_struct)))
// throwing malloc
byte *tmalloc(ulong sz);

/* sizes */

#define _Kb_(x)    ( ((ulong)(x)) << 10)
#define _Mb_(x)	 ( ((ulong)(x)) << 20)

/* Array with addition */


#define __normalize(x_, eltype, headertype)	\
	(x_) = (eltype*)((char*)(x_) + sizeof(headertype))
#define __header(x_, headertype)\
	((headertype*)((char*)(x_)-sizeof(headertype)))


#define GOTO_ADDR(p)  goto *(p)
#define LABEL_ADDR(l)   &&l



#endif /* BASE_H_ */
