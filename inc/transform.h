/* transform.h
 * - scheme translator & interpreter
 * author: denis.lepekhin@gmail.com
 * created on: 2009
 */

#ifndef TRANSFORM_H_
#define TRANSFORM_H_

#include "parse.h"

#define EXCEP_TRANSFORM_FATAL 1067

typedef struct TransformerDatum* transformer_scp;
typedef struct CodeDatum* fcode_scp;
#define TRANSF_BITM_ERR_KWSYN  _ubit_(2)

struct TransformerDatum {
	tag_t tag;
	ulong bits;
	env_scp env; // transformer global environment
	arlist_scp arl;
	fcode_scp gfc; // global code object
	oport_scp vmport;
};

#define _SLOTC_TRANSFORMER 4

extern void *Label_icall, *Label_iret;



#define FCODE_BITM_MULTIARG 1

struct CodeDatum {
	tag_t tag;
	ulong uniq; // unical id;
	ulong bits; // properties
	ulong argc; // number of arguments
	ulong fvc; // free variable counter;
	ulong frmax; // count of remembred frames max
	void  *ccode; // c-code defaults to Label_icall
	env_scp defenv; // local env before definition;
	cons_scp children; // children code-datums
	fcode_scp parent; // parent code datum
	cons_scp lcod; // linear code start
	cons_scp lcod_end; // linear code end

	astk_scp  icod; // interpreted code
	closure_scp closure; //  closure cache
	descr synbody; // body of code (e. g.lambda form minus parameters)
};

extern bool Anf_verbose;
#define _SLOTC_CODEBLOCK 8

#define fcodp_multiarg(f)  ((f)->bits & FCODE_BITM_MULTIARG)
#define fcod_argc(f)       ((f)->argc)
#define fcod_framemax(f)   (((fcode_scp)(f))->frmax)

#define fcod_varcount(f)    (((fcode_scp)(f))->fvc)
#define fcod_clsrcache(f)   (((fcode_scp)(f))->closure)
#define symbol_reserved(d)  desc_is_spec(d)
inline descr lcod_sym(descr d);
void *fcod_entry_point(ulong uniq);

#define desc_is_var(d) (desc_is_global(d) || desc_is_local(d) ||datum_is_id(d))

#define trfp_error(tr)  ((tr)->bits & TRANSF_BITM_ERR_KWSYN)
void fcode_print(svm_t vm, oport_scp op, fcode_scp fc);
void icod_print(svm_t vm,  fcode_scp fc, oport_scp op);
void transformer_init(svm_t vm);
transformer_scp new_transformer(svm_t vm);
inline closure_scp new_closure(svm_t vm, __arg fcode_scp fc);
void trf_check_syntax(svm_t vm, __sbp(transformer_scp) ptr, __sbp( descr) cdat);
bool trf_transform_to_anf(svm_t vm, __sbp(transformer_scp) ptr, __arg descr datum);

#define anf_do_verbose() Anf_verbose = true
#endif /* TRANSFORM_H_ */
