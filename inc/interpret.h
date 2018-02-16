/* interpret.h
 * musc- scheme translator & interpreter
 * author: denis.lepekhin@gmail.com
 * created on: 2009
 */

#ifndef INTERPRET_H_
#define INTERPRET_H_
#include "svm.h"
#include "transform.h"
//#define __GEN

#define EXCEP_INTERPRET_FATAL 1123

#define VAR_NONVAR -1
#define VAR_CAT_L 0
#define VAR_CAT_G 1
#define VAR_CAT_K 2


typedef struct VmiDatum *vmi_scp;


typedef struct VmiVarDatum *vmiv_scp;
typedef struct VmiLitDatum *vmil_scp;
typedef struct VmiVarVarDatum *vmivv_scp;
typedef struct VmiVarLitDatum *vmivl_scp;

typedef struct VmiDatum  *vmimark_scp;
typedef struct VmiPrepDatum  *vmiprep_scp;

#define VMI_BITM_MARK 1
#define VMI_BITM_CALLMARK 1
#define vmi_set_marked(vmi) ((vmi)->bits |= VMI_BITM_MARK)
#define vmi_set_callmarked(vmi) ((vmi)->bits |= VMI_BITM_MARK)
#define vmi_is_marked(vmi)  ((vmi)->bits & VMI_BITM_MARK)
#define vmi_is_callmarked(vmi)  ((vmi)->bits & VMI_BITM_CALLMARK)

extern oport_scp Geninit_oport;

struct VmiDatum {
	tag_t tag;
	ulong iun; // instruction unical numner ( == desc_to_spec(SYMB_VM_XXX) >= 400)
	ulong bits;
	// special data
};

#define _SLOTC_VMI_MARK 1

typedef struct {
	uint fr, ix;
} __attribute__((packed)) varinfo_t;



struct VmiVarVarDatum {
	tag_t tag;
	ulong iun;
	ulong bits;

	varinfo_t var1;
	varinfo_t var2;


	ident_scp id1;
	ident_scp id2;
};
#define _FIELD_VMI_VAR_VAR id1
#define _SLOTC_VMI_VAR_VAR 2


struct VmiVarLitDatum {
	tag_t tag;
	ulong iun;
	ulong bits;

	varinfo_t var;

	ident_scp id;
	descr     lit;


};
#define _FIELD_VMI_VAR_LIT id
#define _SLOTC_VMI_VAR_LIT 2


struct VmiVarDatum {
	tag_t tag;
	ulong iun;
	ulong bits;

	varinfo_t var;
	ident_scp id;


};

#define _FIELD_VMI_VAR id
#define _SLOTC_VMI_VAR 1

struct VmiLitDatum {
	tag_t tag;
	ulong iun;
	ulong bits;

	descr lit;
};


#define _FIELD_VMI_LIT lit
#define _SLOTC_VMI_LIT 1

struct VmiPrepDatum {
	tag_t tag;
	ulong iun;
	ulong bits;


	ident_scp id;
	txtpos_scp pos;
	varinfo_t var;
	ulong fargc; // factual argc
	bool retcall;
};
#define _FIELD_VMI_PREP id
#define _SLOTC_VMI_PREP 2


#define fcod_get_vmi(fc, ip)   (astk_tail((fc)->icod)[ip])


descr vm_run(svm_t vm, __arg fcode_scp fc);
descr vm_eval(svm_t vm, __arg descr datum);



void icod_to_ccode(svm_t vm, oport_scp op, __arg fcode_scp fc);
void vm_compile(svm_t vm, __arg descr datum, oport_scp ccode_op);


#endif /* INTERPRET_H_ */
