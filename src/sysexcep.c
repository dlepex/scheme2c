/* sysexcep.c
 * musc- scheme translator & interpreter
 * author: denis.lepekhin@gmail.com
 * created on: 2009
 */

#include "opsys.h"
#include <malloc.h>
#include "sysexcep.h"

static os_thread_key_t excep_node_key = 0;
static os_thread_key_t excep_data_key = 0;



struct ExcepNode *__excep_node(volatile struct ExcepNode*n) {
	static os_thread_key_t excep_node_key;
	 struct ExcepNode *cur =
		(struct ExcepNode*)os_thread_key_get(excep_node_key);
	if(n) {
		os_thread_key_set(excep_node_key, (void*) n);
	}
	return cur;
}
/**
 * Global init.
 */
void excep_init()
{
	global_once_preamble();
	os_thread_key_create(&excep_node_key);
	os_thread_key_create(&excep_data_key);
}

/**
 * Per-thread init
 */
void excep_thread_init() {
	excep_init();
	struct ExcepData *d = (struct ExcepData*) malloc(sizeof(struct ExcepData));
	d->msg =  (char*)calloc(1024,1);
	os_thread_key_set(excep_data_key, d);
}

inline exdata_t excep_get_data()
{
	return (exdata_t)os_thread_key_get(excep_data_key);
}

inline void __excep_node_push( volatile struct ExcepNode *n)
{
   volatile struct ExcepNode *cur = __excep_node(n);
	n->prev = cur;
}

inline void __excep_node_pop()
{
	volatile const struct ExcepNode *cur = __excep_node(0);
	assert(cur != NULL);
	__excep_node(cur->prev);
}

inline void __excep_raise(int excode,  unsigned long flags, const char *msg, const char* srcfile, int line) {
	struct ExcepNode *cur = __excep_node(0);
	assert( imply(flags & EXCEPTION_USER, excep_code_valid(excode)) && "user exception code >=  100");
	assert(cur && "cx: no handler");
	if(!(flags & EXCEPTION_RERAISE)) {
		exdata_t ed = excep_get_data();
		ed->code = excode;
		if(msg) {
			strcpy(ed->msg, msg);
		} else {
			*((unsigned long long*)msg) = 0;
		}
		ed->srcline = line;
		ed->srcpath = srcfile;
	}
	longjmp(cur->buf, excode);
}

inline const char* excep_msg()
{
	return excep_get_data()->msg;
}
