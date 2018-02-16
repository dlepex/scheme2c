/* unittest.c
 * musc- scheme translator & interpreter
 * author: denis.lepekhin@gmail.com
 * created on: 2009
 */
#include "unittest.h"
#include <malloc.h>
#include "sysexcep.h"

static logger_t Lgr_test;

void unittest_init()
{
	global_once_preamble();
	Lgr_test = filelogger_make("test", stdout, 0);
}

#define MAX_TEST_COUNT 512

struct TestGroup {
	int index;
	tester_t *tests;
	const char* name;
	logger_t logger;
};

test_group_t testgroup_make(const char * name)
{
	struct TestGroup *tg = (struct TestGroup*)malloc(sizeof(struct TestGroup));
	tg->tests = (tester_t*)calloc(MAX_TEST_COUNT, sizeof(void*));
	tg->index = 0;
	tg->logger = Lgr_test;
	tg->name = name;
	return tg;
}

void testgroup_add(test_group_t tg, tester_t t)
{
	struct TestGroup *g = (struct TestGroup*)tg;
	assert(g->index < MAX_TEST_COUNT && t != 0);
	g->tests[g->index++]= t;
}

void testgroup_adda(test_group_t tg, tester_t tt[]) {
	for(int i = 0; tt[i]; i++) {
		testgroup_add(tg, tt[i]);
	}
}

void testgroup_run(test_group_t tg)   {
	struct TestGroup *g = (struct TestGroup*)tg;
	volatile int i = 0;
	volatile int failed = 0;
	logger_t l = testgroup_get_logger(tg);
	_try_
		for(; i < g->index; i++) {
			struct _TestParam tp = {.group = tg, .index = i, .name=0};
			const char * n = (const char*)g->tests[i](&tp);
			if(!n) {
				failed ++;
				logf_info(l, "test %i [%s] FAIL", i+1, n);
			} else {
				logf_info(l, "test %i [%s] OK", i+1,  n);
			}
		}
	_except_
	_catch_(EXCEP_TESTINT)
		failed ++;
		logf_info(l, "test %i [%s] FAIL", i+1, excep_msg());
		i++;

		_retry_;
	_catchall_
		log_excep_error(l);
	_end_try_
	logf_info(l, "===== test-group [%s] success - (%i/%i)", g->name, (g->index - failed), g->index);
}
inline logger_t testgroup_get_logger(test_group_t tg) {
	return ((struct TestGroup*)tg)->logger;
}

void testgroup_set_logger(test_group_t tg, logger_t l){
	((struct TestGroup*)tg)->logger = l;
}
void testgroup_free(test_group_t tg) {
	free(((struct TestGroup*)tg)->tests);
	free(tg);
}
