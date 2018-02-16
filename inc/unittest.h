/* unittest.h
 * musc- scheme translator & interpreter
 * author: denis.lepekhin@gmail.com
 * created on: 2009
 */

#ifndef UNITTEST_H_
#define UNITTEST_H_
#include "logger.h"

typedef void *test_group_t;

struct _TestParam {
	test_group_t group;
	int index;
	const char* name;
};

#define EXCEP_TESTINT 303 // test interrupt exception

typedef void* (*tester_t)(void*);

#define decl_test(name) \
	void *name(void *_data) { \
		struct _TestParam *__par = (struct _TestParam *)_data;\
		static const char* __name = #name; \
		logger_t __logger = testgroup_get_logger(__par->group); \
		int _result = 0;

#define end_decl_test return (void*)__name;}

#define test_prelude(name) \
	struct _TestParam *__par = (struct _TestParam *)__data;\
			static const char* __name = #name; \
			logger_t __logger = testgroup_get_logger(__par->group); \
			int _result = 0;

#define test_postlude  return (void*)__name

#define unitchk(cond, msg) \
	if(!cond) {\
			_result = 1; char buf1[MAX_LOGSTR_SIZE]; file_get_line(buf1, __FILE__, __LINE__); \
			logf_error(__logger, "%s:%i: test [%s] check aborted - %s\nline - <<%s>>",__FILE__,__LINE__, __name, msg, buf1); \
		_mthrow_(EXCEP_TESTINT, __name);\
	}

void unittest_init();
test_group_t testgroup_make(const char*);
void testgroup_add(test_group_t tg, tester_t);
void testgroup_adda(test_group_t tg, tester_t tt[]);
void testgroup_set_logger(test_group_t tg, logger_t);
void testgroup_run(test_group_t tg);
inline logger_t testgroup_get_logger(test_group_t tg);
void testgroup_free(test_group_t tg);

#endif /* UNITTEST_H_ */
