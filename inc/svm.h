/* svm.h Scheme Virtual Machine
 * - scheme translator & interpreter
 * author: denis.lepekhin@gmail.com
 * created on: 2009
 */

#ifndef SVM_H_
#define SVM_H_


#include "vmdefs.h"

#define EXCEP_VM_BAD_ARGC     1200
#define EXCEP_VM_BAD_CALL     1201
#define EXCEP_VM_WRONG_PARAM  1202
#define EXCEP_VM_ASSERT       1203
#define EXCEP_VM_GLOBAL_UNDEF 1204
#define EXCEP_VM_GLOBAL_DUP   1205



struct SVM_Params {
	ulong stack_size, symtable_count,
		symhash_count, perm_size,
		typetable_count, globtable_count, swapstr_len;
} svm_param_t;






#ifndef MAXSWAPSYMSTRSIZE
#define MAXSWAPSYMSTRSIZE 4096
#endif

#define EXCEP_SYMTABLE_OVERFLOW 700





void vm_default_params(struct SVM_Params *p);
svm_t vm_init(struct SVM_Params *p, gc_t gc);




void vm_panic(svm_t vm, int excep, descr pos,  const UChar *msg);



#define vm_panic_f(vm, ex, p, format, ...) \
{\
	UChar _u[256]; u_sprintf(_u, format, __VA_ARGS__);\
	vm_panic(vm, ex, p, _u); \
}

#define PANIC_BAD_ARG(format, ... )  vm_panic_f(vm, EXCEP_VM_WRONG_PARAM, NULL, format,__VA_ARGS__ )
#define PANIC_ASSERT(format, ... )  vm_panic_f(vm, EXCEP_VM_ASSERT, NULL, format,__VA_ARGS__ )
#define PANIC_BAD_ARG_ASCI(asci) PANIC_BAD_ARG("%s", asci)
#define PANIC_ASSERT_ASCI(asci) PANIC_ASSERT("%s", asci)

// constructors






#endif /* SVM_H_ */
