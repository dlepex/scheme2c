/* opsys.c
 * musc- scheme translator & interpreter
 * author: denis.lepekhin@gmail.com
 * created on: 2009
 */

#include "opsys.h"
#include "base.h"
#include <assert.h>
#include <stdio.h>
#include <malloc.h>
#include <pthread.h>


int os_thread_start(os_thread_t *thread,  void *(*thread_proc)(void*), void *args)
{
	return pthread_create((pthread_t*)thread, NULL /*attr*/, thread_proc, args);
}
os_thread_t os_thread_this() {
	return (os_thread_t) pthread_self();
}
bool os_thread_eq(os_thread_t t1, os_thread_t t2) {
	return (bool)pthread_equal((pthread_t)t1, (pthread_t)t2);
}
int  os_thread_key_create(os_thread_key_t* key)
{
	return pthread_key_create((pthread_key_t*)key, NULL/*destructor*/);
}
void  *os_thread_key_get(os_thread_key_t key) {
	return pthread_getspecific((pthread_key_t)key);
}
int  os_thread_key_set(os_thread_key_t key, const void* val) {
	return pthread_setspecific((pthread_key_t)key, val);
}



