/* opsys.h
 * musc- scheme translator & interpreter
 * author: denis.lepekhin@gmail.com
 * created on: 2009
 */

#ifndef OPSYS_H_
#define OPSYS_H_


#include <setjmp.h>
#include <stdbool.h>
typedef void * os_thread_t;
typedef void * os_thread_key_t;




int os_thread_start(os_thread_t *thread,  void *(*thread_proc)(void*), void *args);
os_thread_t os_thread_this();
bool os_thread_eq(os_thread_t t1, os_thread_t t2);
int  os_thread_key_create(os_thread_key_t* key);
void  *os_thread_key_get(os_thread_key_t key);
int  os_thread_key_set(os_thread_key_t key, const void* val);




#endif /* OPSYS_H_ */
