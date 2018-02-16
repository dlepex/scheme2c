/* sysexcep.h
 * - scheme translator & interpreter
 * author: denis.lepekhin@gmail.com
 * created on: 2009
 */

#ifndef SYSEXCEP_H_
#define SYSEXCEP_H_

#include "base.h"
#include <setjmp.h>

struct ExcepNode {
	jmp_buf buf;
	volatile struct ExcepNode *prev;
};

struct ExcepData {
	int code;
   char *msg;
   const char*srcpath;
	int srcline;
};

typedef struct ExcepData* exdata_t;
#define EXCEPTION_RERAISE 1
#define EXCEPTION_USER    2
inline void __excep_raise(int excode, unsigned long flags, const char *msg, const char* srcfile, int line);
inline const char* excep_msg();
inline exdata_t execp_get_data();
void excep_init();
void excep_thread_init();

#define excep_code_valid(c) (c >= 100)

#define EXCEP_RETRY_CODE -1

/**
 * Retries current guarded block
 * Pragmatics: usefull when we can correct what caused an exception.
 * (!) use ONLY in a context of the _catch_ or _catchall_ clause of _try_ block
 * For example: the death-ressurection cycle for a cat:
 * _try_
 * 		kill_cat();
 * _except_
 * _catch_
 * 		ressurect_cat();
 * 		_retry_;
 * _endtry_;
 *
 */
#define _retry_     longjmp(((struct ExcepNode*)(&__xnode__))->buf, EXCEP_RETRY_CODE)

/**
 * Throws exception with code.
 * ex must be: assert(ex >= 2 || ex <= -2) Never use: 1, 0, -1.
 * Used in any context (possibly in _try_ block)
 */
#define _throw_(ex) __excep_raise(ex, EXCEPTION_USER, "", __FILE__, __LINE__)
#define _mthrow_(ex,str) __excep_raise(ex,EXCEPTION_USER, str, __FILE__, __LINE__)
#define _mthrow_assert_(cond, ex, msg) \
	if(!(cond)) { __excep_raise(ex, EXCEPTION_USER , msg, __FILE__, __LINE__); }

#define _assert_(cond)  _mthrow_assert_(cond, EXCEP_ASSERT, "assertion failed")



#define _fthrow_(ex, format, ...)  { \
		 char _s_E[512]; \
		 sprintf(_s_E, format, __VA_ARGS__); \
		 _mthrow_(ex, _s_E); \
}
/**
 * Rethrows exeption (to outer _try_ blocks, if any)
 * (!) use in context of _try_ block only
 * ...
 * catch(myerror_code)
 *    recovery_routine();
 *    _rethrow_;
 * ...
 * \sa _chain_try_
 */
#define _rethrow_    __excep_raise(excode, EXCEPTION_USER | EXCEPTION_RERAISE, "", 0, 0)
/**
 * Begins _try_ block (switch-based exception handler)
 * For example:
 * _try_
 *    some_guarded_code(); //<- contains _throw_(my_error_XXX) somewhere in its deep
 * _except_
 * _catch_(my_error_1)
 *    recovery_code_1();
 * _catch_(my_error_2)
 *    recovery_code_2();
 *    _retry_;
 * _catchall
 * 	  printf("other error: %i", excode);
 * _endtry_
 */

inline void __excep_node_push(volatile struct ExcepNode*);
inline void __excep_node_pop();
struct ExcepNode *__excep_node(volatile struct ExcepNode*);
#define _try_  { \
	volatile struct ExcepNode __xnode__;\
	volatile const int excode = setjmp(      ((struct ExcepNode*)(&__xnode__))->buf);\
	switch(excode) {\
	case 0:case EXCEP_RETRY_CODE:{ __excep_node_push(&__xnode__);\

#define _except_ __excep_node_pop();
/**
 * Handles exception with code \a ex
 */
#define _catch_(ex) }break; case ex:{__excep_node_pop();
#define _catch2_(ex1, ex2) }break; case ex1:case ex2:{__excep_node_pop();
#define _catch3_(ex1,ex2,ex3) }break; case ex1:case ex2:case ex3:{__excep_node_pop();
/**
 * catch the rest errors (which were not caught by previous _catch_'s)
 * _try_ block should have zero or one _catchall_ clause.
 * Variable excode (implicit) contains error-code
 *
 */
#define _catchall_ }break; default:{__excep_node_pop();
/**
 * Ends _try_ block
 */
#define _end_try_       }}}
/**
 * Ends _try_ block and rethrows uncaught exceptions to outer _try_ blocks
 * (default behaviour of C++ Java etc).
 */
#define _chain_try_	 _catchall_ _rethrow_; _end_try_




#endif /* SYSEXCEP_H_ */
