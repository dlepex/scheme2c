/* interpret.c
 * musc- scheme translator & interpreter
 * author: denis.lepekhin@gmail.com
 * created on: 2009
 */

#include "loggers.h"
#include "sysexcep.h"
#include "interpret.h"
#include "svm.h"



#define SEGM_MAX_SZ  4096
void *Label_icall = NULL, *Label_iret = NULL;
void *Fcodes[SEGM_MAX_SZ];
ulong Fcodes_size = 1;
oport_scp Geninit_oport = 0;

void vm_panic_global_error(svm_t vm,  int ex, fcode_scp fc, ulong ip, int pos);
descr  icod_interpret(svm_t vm, __arg fcode_scp fc);
/*LCode to VMI-code*/


void interpret_init(svm_t vm) {
	global_once_preamble();

	icod_interpret(NULL, NULL); // initialize labels

	type_reserve(vm, TYPEIX_VMI_LIT, "<vmi-lit>", TYPE_CATEGORY_NONTERM ,
	sizeof(struct VmiLitDatum), 0, field_offs_scp(vmil_scp,_FIELD_VMI_LIT ), _SLOTC_VMI_LIT);


	type_reserve(vm, TYPEIX_VMI_VAR, "<vmi-var>", TYPE_CATEGORY_NONTERM ,
	sizeof(struct VmiVarDatum), 0, field_offs_scp(vmiv_scp, _FIELD_VMI_VAR ), _SLOTC_VMI_VAR);


	type_reserve(vm, TYPEIX_VMI_VAR_VAR, "<vmi-var-var>", TYPE_CATEGORY_NONTERM ,
	sizeof(struct VmiVarVarDatum), 0, field_offs_scp(vmivv_scp, _FIELD_VMI_VAR_VAR), _SLOTC_VMI_VAR_VAR);

	type_reserve(vm, TYPEIX_VMI_VAR_LIT, "<vmi-var-lit>", TYPE_CATEGORY_NONTERM ,
		sizeof(struct VmiVarLitDatum), 0, field_offs_scp(vmivl_scp, _FIELD_VMI_VAR_LIT), _SLOTC_VMI_VAR_LIT);

	type_reserve(vm, TYPEIX_VMI_PREP, "<vmi-prep>", TYPE_CATEGORY_NONTERM ,
		sizeof(struct VmiPrepDatum), 0, field_offs_scp(vmiprep_scp, _FIELD_VMI_PREP), _SLOTC_VMI_PREP);
	type_reserve(vm, TYPEIX_VMI_MARK, "<vmi-mark>", TYPE_CATEGORY_CONST,
		sizeof(struct VmiDatum), 0, 0, 0);

}


void *fcod_entry_point(ulong uniq)
{
	if(uniq < Fcodes_size) {
		return Fcodes[uniq];
	} else {
		return Label_icall;
	}
}

inline int var_cat(descr var)
{
	if(datum_is_id(var)) var = id_var(var);
	if(desc_is_local(var)) {
		if(desc_loc_fr(var)) {
			return VAR_CAT_K;
		} else {
			return VAR_CAT_L;
		}
	} else if(desc_is_global(var)) {
		return VAR_CAT_G;
	} else {
		// not a var;
		return VAR_NONVAR; // DATUN
	}
}


void varinf_set(descr var, varinfo_t *vi)
{
	vi->fr = 0;
	vi->ix = 0;
	if(datum_is_id(var)) var = id_var(var);
	if(desc_is_local(var)) {
		vi->fr = desc_loc_fr(var);
		vi->ix = desc_loc_ix(var);
	} else if(desc_is_global(var)) {
		vi->ix = desc_to_glob(var);
	} else {
		_mthrow_(EXCEP_TRANSFORM_FATAL, "variable expected in lcode");
	}
}



// x - V, y - D or V
vmi_scp vmi_set_xy(svm_t vm, __arg descr x, __arg descr y)
{
	decl_gc_of_vm();
	SAVE_RSTK();
	PUSH_RSTK2(x, y);


	int x_cat = var_cat(x);
	int y_cat = var_cat(y);

	assert(x_cat != VAR_NONVAR && "lefyt side is var");
	__protect(vmi_scp, res, NIHIL);
	if(y_cat == VAR_NONVAR) {
		res = (vmi_scp) inew_inst(gc, TYPEIX_VMI_VAR_LIT);
		((vmivl_scp)res)->lit = y;
		((vmivl_scp)res)->id = datum_is_id(x) ? x : NIHIL;
		varinf_set(x, &((vmivl_scp)res)->var);
	} else {
		res = (vmi_scp) inew_inst(gc, TYPEIX_VMI_VAR_VAR);
		varinf_set(y, &((vmivv_scp)res)->var2);
		((vmivv_scp)res)->id1 = datum_is_id(x) ? x : NIHIL;
		((vmivv_scp)res)->id2 = datum_is_id(y) ? y : NIHIL;
		varinf_set(x, &((vmivv_scp)res)->var1);
	}


	switch(x_cat) {
	case VAR_CAT_L:
		switch(y_cat) {
		case VAR_CAT_L:  res->iun = desc_to_spec(SYMB_VM_SET_L_L); break;
		case VAR_CAT_G:  res->iun = desc_to_spec(SYMB_VM_SET_L_G); break;
		case VAR_CAT_K:  res->iun = desc_to_spec(SYMB_VM_SET_L_K); break;
		case VAR_NONVAR: res->iun = desc_to_spec(SYMB_VM_SET_L_D); break;
		}
		break;
	case VAR_CAT_G:
		switch(y_cat) {
		case VAR_CAT_L:  res->iun = desc_to_spec(SYMB_VM_SET_G_L); break;
		case VAR_CAT_G:  res->iun = desc_to_spec(SYMB_VM_SET_G_G); break;
		case VAR_CAT_K:  res->iun = desc_to_spec(SYMB_VM_SET_G_K); break;
		case VAR_NONVAR: res->iun = desc_to_spec(SYMB_VM_SET_G_D); break;
		}
		break;
	case VAR_CAT_K:
		switch(y_cat) {
		case VAR_CAT_L:  res->iun = desc_to_spec(SYMB_VM_SET_K_L); break;
		case VAR_CAT_G:  res->iun = desc_to_spec(SYMB_VM_SET_K_G); break;
		case VAR_CAT_K:  res->iun = desc_to_spec(SYMB_VM_SET_K_K); break;
		case VAR_NONVAR: res->iun = desc_to_spec(SYMB_VM_SET_K_D); break;
		}
		break;
	}
	UNWIND_RSTK();
	res->bits = 0;
	return res;
}

vmi_scp vmi_set_xr(svm_t vm, __arg descr x)
{
	decl_gc_of_vm();
	SAVE_RSTK();
	PUSH_RSTK(x);
	int x_cat = var_cat(x);
	assert(x_cat != VAR_NONVAR && "left side is var");
	__protect(vmiv_scp, res, (vmiv_scp) inew_inst(gc, TYPEIX_VMI_VAR));
	res->id = datum_is_id(x) ? x : NIHIL;
	varinf_set(x, &res->var);

	switch(x_cat) {
	case VAR_CAT_L:  res->iun = desc_to_spec(SYMB_VM_SET_L_R); break;
	case VAR_CAT_G:  res->iun = desc_to_spec(SYMB_VM_SET_G_R); break;
	case VAR_CAT_K:  res->iun = desc_to_spec(SYMB_VM_SET_K_R); break;
	}
	UNWIND_RSTK();
	res->bits = 0;
	return (vmi_scp) res;
}

vmi_scp vmi_def_xy(svm_t vm, __arg descr x, __arg descr y)
{
	decl_gc_of_vm();
	SAVE_RSTK();
	PUSH_RSTK2(x, y);


	int x_cat = var_cat(x);
	int y_cat = var_cat(y);

	assert(x_cat == VAR_CAT_G && "left side is G-var");
	__protect(vmi_scp, res, NIHIL);
	if(y_cat == VAR_NONVAR) {
		res = (vmi_scp) inew_inst(gc, TYPEIX_VMI_VAR_LIT);
		((vmivl_scp)res)->lit = y;
		((vmivl_scp)res)->id = datum_is_id(x) ? x : NIHIL;
		res->iun = desc_to_spec(SYMB_VM_DEF_D);
		varinf_set(x, &((vmivl_scp)res)->var);
	} else {
		assert(y_cat == VAR_CAT_G && "r is G-var");
		res = (vmi_scp) inew_inst(gc, TYPEIX_VMI_VAR_VAR);
		varinf_set(y, &((vmivv_scp)res)->var2);
		((vmivv_scp)res)->id1 = datum_is_id(x) ? x : NIHIL;
		((vmivv_scp)res)->id2 = datum_is_id(y) ? y : NIHIL;
		res->iun = desc_to_spec(SYMB_VM_DEF_G);
		varinf_set(x, &((vmivv_scp)res)->var1);
	}

	UNWIND_RSTK();
	res->bits = 0;
	return res;
}





vmi_scp vmi_def_xr(svm_t vm, __arg descr x)
{
	decl_gc_of_vm();
	SAVE_RSTK();
	PUSH_RSTK(x);
	int x_cat = var_cat(x);
	assert(x_cat == VAR_CAT_G && "left side is G-var");
	__protect(vmiv_scp, res, (vmiv_scp) inew_inst(gc, TYPEIX_VMI_VAR));
	varinf_set(x, &((vmiv_scp)res)->var);
	((vmiv_scp)res)->id = datum_is_id(x) ? x : NIHIL;
	res->iun = desc_to_spec(SYMB_VM_DEF_R);
	UNWIND_RSTK();
	return (vmi_scp) res;
}


vmi_scp vmi_ret_x(svm_t vm, __arg descr x)
{
	decl_gc_of_vm();
	SAVE_RSTK();
	PUSH_RSTK(x);
	int x_cat = var_cat(x);
	__protect(vmi_scp, res, NIHIL);

	if(x_cat != VAR_NONVAR) {
		res = (vmi_scp) inew_inst(gc, TYPEIX_VMI_VAR);
		varinf_set(x, &((vmiv_scp)res)->var);
		((vmiv_scp)res)->id = datum_is_id(x) ? x : NIHIL;
		switch(x_cat) {
		case VAR_CAT_L:  res->iun = desc_to_spec(SYMB_VM_RET_L); break;
		case VAR_CAT_G:  res->iun = desc_to_spec(SYMB_VM_RET_G); break;
		case VAR_CAT_K:  res->iun = desc_to_spec(SYMB_VM_RET_K); break;
		}
	} else {
		res = (vmi_scp) inew_inst(gc, TYPEIX_VMI_LIT);
		((vmil_scp)res)->lit = x;
		res->iun = desc_to_spec(SYMB_VM_RET_D);

	}
	UNWIND_RSTK();
	res->bits = 0;
	return (vmi_scp) res;
}


vmi_scp vmi_argpush(svm_t vm, __arg descr x)
{
	decl_gc_of_vm();
	SAVE_RSTK();
	PUSH_RSTK(x);
	int x_cat = var_cat(x);
	__protect(vmi_scp, res, NIHIL);

	if(x_cat != VAR_NONVAR) {
		res = (vmi_scp) inew_inst(gc, TYPEIX_VMI_VAR);
		varinf_set(x, &((vmiv_scp)res)->var);
		((vmiv_scp)res)->id = datum_is_id(x) ? x : NIHIL;
		switch(x_cat) {
		case VAR_CAT_L:  res->iun = desc_to_spec(SYMB_VM_ARGPUSH_L); break;
		case VAR_CAT_G:  res->iun = desc_to_spec(SYMB_VM_ARGPUSH_G); break;
		case VAR_CAT_K:  res->iun = desc_to_spec(SYMB_VM_ARGPUSH_K); break;
		}
	} else {
		res = (vmi_scp) inew_inst(gc, TYPEIX_VMI_LIT);
		((vmil_scp)res)->lit = x;
		res->iun = desc_to_spec(SYMB_VM_ARGPUSH_D);
	}
	UNWIND_RSTK();
	res->bits = 0;
	return (vmi_scp) res;
}


vmi_scp vmi_prep_x(svm_t vm, __arg descr x, __arg txtpos_scp pos,  ulong fargc, bool retcall )
{
	decl_gc_of_vm();
	SAVE_RSTK();
	PUSH_RSTK2(x,pos);
	int x_cat = var_cat(x);
	assert(x_cat != VAR_NONVAR && "left side is var");
	__protect(vmiprep_scp, res, (vmiprep_scp) inew_inst(gc, TYPEIX_VMI_PREP));
	res->id = datum_is_id(x) ? x : NIHIL;
	res->pos = pos;
	res->fargc = fargc;
	varinf_set(x, &res->var);
	if(retcall) {
		switch(x_cat) {
		case VAR_CAT_L:  res->iun = desc_to_spec(SYMB_VM_RPREP_L); break;
		case VAR_CAT_G:  res->iun = desc_to_spec(SYMB_VM_RPREP_G); break;
		case VAR_CAT_K:  res->iun = desc_to_spec(SYMB_VM_RPREP_K); break;
		}
	} else {
		switch(x_cat) {
		case VAR_CAT_L:  res->iun = desc_to_spec(SYMB_VM_PREP_L); break;
		case VAR_CAT_G:  res->iun = desc_to_spec(SYMB_VM_PREP_G); break;
		case VAR_CAT_K:  res->iun = desc_to_spec(SYMB_VM_PREP_K); break;
		}
	}
	UNWIND_RSTK();
	res->bits = 0;
	return (vmi_scp) res;
}

vmi_scp vmi_prep_r(svm_t vm,  __arg txtpos_scp pos,  ulong fargc, bool retcall )
{
	decl_gc_of_vm();
	SAVE_RSTK();
	PUSH_RSTK(pos);
	__protect(vmiprep_scp, res, (vmiprep_scp) inew_inst(gc, TYPEIX_VMI_PREP));
	res->pos = pos;
	res->fargc = fargc;
	res->iun = retcall ? desc_to_spec(SYMB_VM_RPREP_R) : desc_to_spec(SYMB_VM_PREP_R);
	UNWIND_RSTK();
	res->bits = 0;
	return (vmi_scp) res;
}


vmi_scp vmi_if_x(svm_t vm, __arg descr x, __arg descr mark)
{
	decl_gc_of_vm();
	SAVE_RSTK();
	PUSH_RSTK2(mark, x);
	int x_cat = var_cat(x);
	assert(x_cat != VAR_NONVAR && "left side is var");
	__protect(vmivl_scp, res, (vmivl_scp) inew_inst(gc, TYPEIX_VMI_VAR_LIT));
	res->id = datum_is_id(x) ? x : NIHIL;
	res->lit = mark;
	varinf_set(x, &res->var);

	switch(x_cat) {
	case VAR_CAT_L:  res->iun = desc_to_spec(SYMB_VM_IF_L); break;
	case VAR_CAT_G:  res->iun = desc_to_spec(SYMB_VM_IF_G); break;
	case VAR_CAT_K:  res->iun = desc_to_spec(SYMB_VM_IF_K); break;
	}
	UNWIND_RSTK();
	res->bits = 0;
	return (vmi_scp) res;
}

vmi_scp vmi_if_r(svm_t vm,   __arg descr mark)
{
	decl_gc_of_vm();
	SAVE_RSTK();
	PUSH_RSTK(mark);
	__protect(vmil_scp, res, (vmil_scp) inew_inst(gc, TYPEIX_VMI_LIT));
	res->iun = desc_to_spec(SYMB_VM_IF_R);
	res->lit = mark;
	UNWIND_RSTK();
	res->bits = 0;
	return (vmi_scp) res;
}

vmi_scp vmi_jmp(svm_t vm,   __arg descr mark)
{
	decl_gc_of_vm();
	SAVE_RSTK();
	PUSH_RSTK(mark);
	__protect(vmil_scp, res, (vmil_scp) inew_inst(gc, TYPEIX_VMI_LIT));
	res->iun = desc_to_spec(SYMB_VM_JMP);
	res->lit = mark;
	UNWIND_RSTK();
	res->bits = 0;
	return (vmi_scp) res;
}

vmi_scp vmi_ret_r(svm_t vm)
{
	decl_gc_of_vm();
	__loc vmi_scp res = (vmi_scp) inew_inst(gc, TYPEIX_VMI_MARK);
	res->iun = desc_to_spec(SYMB_VM_RET_R);
	res->bits = 0;
	return res;
}

vmi_scp vmi_call(svm_t vm)
{
	decl_gc_of_vm();
	__loc vmi_scp res = (vmi_scp) inew_inst(gc, TYPEIX_VMI_MARK);
	res->iun = desc_to_spec(SYMB_VM_CALL);
	res->bits = 0;
	return res;
}

vmi_scp vmi_retcall(svm_t vm)
{
	decl_gc_of_vm();
	__loc vmi_scp res = (vmi_scp) inew_inst(gc, TYPEIX_VMI_MARK);
	res->iun = desc_to_spec(SYMB_VM_RETCALL);
	res->bits = 0;
	return res;
}

typedef struct {
	bool prevmark;
} icflags_t;

void icod_add(svm_t vm, icflags_t *pf,  __sbp(fcode_scp) pfc,  vmi_scp vmi)
{
	if(pf->prevmark) {
		vmi_set_marked(vmi);
	}
	__loc astk_scp a = astk_push(vm, _DF(pfc, icod), vmi);
	FIELD_DSET(pfc, icod, a);
	pf->prevmark = false;
}



inline bool varinf_eq_loc(varinfo_t *v, descr loc) {
	return v->ix == desc_loc_ix(loc) && v->fr == desc_loc_fr(loc);
}


bool icod_rem_set_xr(svm_t vm, icflags_t *pf, __sbp(fcode_scp) pfc,  __arg descr var) {
	if(!desc_is_var(var)) {
		return false;
	}
	astk_scp a = _DF(pfc, icod);
	ulong asz = astk_size(a);
	if(!pf->prevmark && desc_is_local(var) && asz >= 2) {
		vmi_scp tail = astk_top(a);
		if( ((vmi_scp)tail)->iun == desc_to_spec(SYMB_VM_SET_L_R)  ) {
			if( varinf_eq_loc(& ( (vmiv_scp) (tail))->var, var)) {
				astk_pop(a);
				return true;
			}
		}
	}
	return false;
}

void icod_add_lmd(svm_t vm, icflags_t *pf, __sbp(fcode_scp) pfc, __sbp(cons_scp) lmd  )
{
	decl_gc_of_vm();
	assert(CAR(*lmd) == SYMB_VM_LAMBDA);
	__protect( vmi_scp, res,  (vmi_scp) inew_inst(gc, TYPEIX_VMI_LIT));
	res->iun = desc_to_spec(SYMB_VM_LAMBDA);
	((vmil_scp)res)->lit = CAR(CDR(_DS(lmd)));
	icod_add(vm, pf, pfc, res);
}



void icod_add_call(svm_t vm, icflags_t *pf, __sbp(fcode_scp) pfc,
		__sbp(cons_scp) call, bool retcall  )
{
	SAVE_RSTK();
	assert(CAR(*call) == SYMB_VM_CALL);
	__protect( vmi_scp, cur,  NIHIL);
	__protect(txtpos_scp, pos, syn_getpos(*call));
	__protect(cons_scp, p, CDR(*call));
	__protect(vec_scp, args, CDR(p));

	p = CAR(p); // p = var

	bool remxr = icod_rem_set_xr(vm, pf,pfc, p );


	ulong argc = vec_len(args);

	cur = remxr ? vmi_prep_r(vm, pos, argc, retcall) :
			vmi_prep_x(vm,p, pos, argc, retcall );

	((vmiprep_scp)cur)->retcall = retcall;

	icod_add(vm,pf,pfc,cur);

	for(ulong i = 0; i < argc; i++) {
		cur = vmi_argpush(vm, vec_tail(args)[i]);
		icod_add(vm,pf,pfc,cur);
	}

	cur = retcall ? vmi_retcall(vm) : vmi_call(vm);
	icod_add(vm,pf,pfc,cur);
	UNWIND_RSTK();
}

#define ICOD_DEF_SZ 1

void icod_create(svm_t vm, __arg fcode_scp fc)
{
	decl_gc_of_vm();
	SAVE_RSTK();
	PUSH_RSTK(fc);

	__protect(vmi_scp, cur, NIHIL);
	__protect(vmi_scp,  v, NIHIL);
	__protect(cons_scp, p, fc->lcod);
	__protect(cons_scp, l, NIHIL);
	__protect(cons_scp, r, NIHIL);
	__loc descr sym = NIHIL;

	__loc astk_scp a = new_astk(gc, ICOD_DEF_SZ );
	FIELD_SET(fc, icod, a);
	icflags_t icf = {.prevmark = false};

	p = CDR(p);
	for(;p;p=CDR(p)) {
		l = CAR(p);
		switch(desc_to_spec(CAR(l))) {
		case desc_to_spec(SYMB_VM_SET):
			l = CDR(l); r = CAR(CDR(l)); l = CAR(l);
			 sym = lcod_sym(r);
			if(sym) {
				switch(desc_to_spec(sym)) {
				case desc_to_spec(SYMB_VM_LAMBDA): {
					icod_add_lmd(vm,&icf, &fc, &r);

				}break;
				case desc_to_spec(SYMB_VM_CALL): {
					icod_add_call(vm, &icf, &fc, &r, false);
				}
				break;
				default:
					_mthrow_(EXCEP_TRANSFORM_FATAL, "lcode - bad SET!");
				}
				cur = vmi_set_xr(vm, l);
			} else {
				cur = vmi_set_xy(vm, l, r);

			}
			icod_add(vm, &icf, &fc, cur);
			break;
		case desc_to_spec(SYMB_VM_DEF):
			l = CDR(l); r = CAR(CDR(l)); l = CAR(l);
			 sym = lcod_sym(r);
			if(sym) {
				switch(desc_to_spec(sym)) {
				case desc_to_spec(SYMB_VM_LAMBDA): {
					icod_add_lmd(vm,&icf, &fc, &r);

				}break;
				case desc_to_spec(SYMB_VM_CALL): {
					icod_add_call(vm, &icf, &fc, &r, false);
				}
				break;
				default:
					_mthrow_(EXCEP_TRANSFORM_FATAL, "lcode - bad DEF!");
				}
				cur = vmi_def_xr(vm, l);
			} else {
				cur = vmi_def_xy(vm, l, r);
			}
			icod_add(vm, &icf, &fc, cur);
			break;
		case desc_to_spec(SYMB_VM_RET):
			l = CAR(CDR(l));
			sym = lcod_sym(l);
			if(sym) {
				switch(desc_to_spec(sym)) {
				case desc_to_spec(SYMB_VM_LAMBDA): {
					icod_add_lmd(vm,&icf, &fc, &l);
					cur = vmi_ret_r(vm);
					icod_add(vm, &icf, &fc, cur);
				}break;
				case desc_to_spec(SYMB_VM_CALL): {
					icod_add_call(vm, &icf, &fc, &l, true);
				}

				break;
				default:
					_mthrow_(EXCEP_TRANSFORM_FATAL, "lcode - bad RET!");
				}
			} else {
				cur = vmi_ret_x(vm, l);
				icod_add(vm, &icf, &fc, cur);
			}

			break;
		case desc_to_spec(SYMB_VM_CALL):
			icod_add_call(vm, &icf, &fc, &l, false);
			break;
		case desc_to_spec(SYMB_VM_MARK): {
			l = CDR(l);
			if(CAR(l)) {
				if(CDR(l)) {
					//cur = vmi_mark(vm, desc_to_fixnum(CAR(l)));
					//_ADD_VMI(cur);
					v = (vmi_scp) CDR(l);
					switch(datum_get_typeix(v)) {
					case TYPEIX_VMI_LIT:
						((vmil_scp)v)->lit = desc_from_fixnum(astk_size(fc->icod));
						break;
					case TYPEIX_VMI_VAR_LIT:
						((vmivl_scp)v)->lit= desc_from_fixnum(astk_size(fc->icod));
						break;
					default:
						assert(0 && "no such mark generating construct");
					}
					l->cdr = NIHIL;
				} else {
					_mthrow_(EXCEP_TRANSFORM_FATAL, "non backward-ref mark");
				}
			}
			icf.prevmark = true;
		} break;
		case desc_to_spec(SYMB_VM_IF): {
			l = CDR(l);
			r = CAR(l);
			bool remxr = icod_rem_set_xr(vm, &icf, &fc, (descr)r);
			r= CAR(CDR(l));
			assert(CAR(r) == SYMB_VM_MARK && "mark expected in !if");
			cur = remxr ? vmi_if_r(vm, NIHIL) : vmi_if_x(vm, CAR(l), NIHIL);
			icod_add(vm, &icf, &fc, cur);
			r = CDR(r);
			FIELD_SET(r, cdr, cur);
		} break;
		case desc_to_spec(SYMB_VM_JMP): {
			l = CDR(l); r = CAR(CAR(l));
			assert(CAR(r) == SYMB_VM_MARK && "mark expected in !jmp");
			r = CDR(r);
			cur = vmi_jmp(vm, NIHIL);
			icod_add(vm, &icf, &fc, cur);
			FIELD_SET(r, cdr, cur);
		} break;
		default:
			_mthrow_(EXCEP_TRANSFORM_FATAL, "no such lcode instruction");
		}
	}

	UNWIND_RSTK();
}
void varinf_print(varinfo_t* v, oport_scp out)
{
	oport_write_asci(out, "?$");
	if(v->fr) {
		oport_print_int(out, v->fr);
		oport_write_asci(out, ".");
	}
	oport_print_int(out, v->ix);
}






void icod_print(svm_t vm,  fcode_scp fc, oport_scp op)
{

	oport_write_asci(op, "\nicode  ");
	oport_print_int(op, fc->uniq);
	oport_write_asci(op, ":\n");



	astk_scp a = fc->icod;
	for(ulong i = 0;i < astk_size(a); i++)
	{
		vmi_scp p = (vmi_scp) astk_tail(a)[i];
		oport_print_int(op, i);
		oport_write_asci(op, ": ");
		oport_print_datum(vm, op,  desc_from_spec(p->iun));
		oport_write_asci(op, " ");

		switch(datum_get_typeix(p)) {
		case TYPEIX_VMI_VAR:
			varinf_print(&((vmiv_scp)p)->var, op);
			break;
		case TYPEIX_VMI_LIT:
			switch(p->iun) {
			case desc_to_spec(SYMB_VM_IF_R):
			case desc_to_spec(SYMB_VM_JMP):
				oport_write_asci(op, " ->");
				oport_print_datum(vm, op, ((vmil_scp)p)->lit);
				break;
			default:
				oport_print_datum(vm, op, ((vmil_scp)p)->lit);
			}
			break;
		case TYPEIX_VMI_VAR_LIT:
			varinf_print(&((vmivl_scp)p)->var, op);
			oport_write_asci(op, " ");
			switch(p->iun) {
			case desc_to_spec(SYMB_VM_IF_G):
			case desc_to_spec(SYMB_VM_IF_L):
			case desc_to_spec(SYMB_VM_IF_K):
				oport_write_asci(op, " ->");
				oport_print_datum(vm, op, ((vmivl_scp)p)->lit);
				break;
			default:
				oport_print_datum(vm, op, ((vmivl_scp)p)->lit);
			}
			break;
		case TYPEIX_VMI_VAR_VAR:
			varinf_print(&((vmivv_scp)p)->var1, op);
			oport_write_asci(op, " ");
			varinf_print(&((vmivv_scp)p)->var2, op);
			break;
		case TYPEIX_VMI_PREP:
			if(((vmiprep_scp)p)->retcall) {
				oport_write_asci(op, " [RET] ");
			}
			if(p->iun != desc_to_spec(SYMB_VM_PREP_R) && p->iun != desc_to_spec(SYMB_VM_RPREP_R))
				varinf_print(&((vmiprep_scp)p)->var, op);
			oport_write_asci(op, " ");
			oport_print_int(op, ((vmiprep_scp)p)->fargc);
			break;

		}
		oport_write_asci(op, "\n");
	}
}

#include "__interpret.h"

inline descr __glob_get( svm_t vm, fcode_scp fc, ulong _ip, ulong ix) {
	descr glob = __global(ix);
	if(!glob)__PANIC_GLOBAL_UNDEF(_ip, 2) ;
	return unbox(glob);
}


descr icod_interpret(svm_t vm, __arg fcode_scp fc);

void vm_panic(svm_t vm, int excep, descr _pos,  const UChar *msg)
{
	const txtpos_scp pos = (txtpos_scp) _pos;
	char *s = "";
	switch(excep) {
	case EXCEP_VM_WRONG_PARAM: s = "wrong argument"; break;
	case EXCEP_VM_BAD_CALL: s = "wrong call"; break;
	case EXCEP_VM_BAD_ARGC: s = "wrong number of arguments"; break;
	case EXCEP_VM_GLOBAL_UNDEF: s = "global undefined"; break;
	case EXCEP_VM_GLOBAL_DUP: s = "global redefinition"; break;
	}

	const char * fmt = (pos ?
			"\nError[RUNTIME] %i:%i-%i:%i - %s - %S\n" :
		"\nError[RUNTIME] - %s - %S\n");

	if(pos) {
		u_fprintf(ustderr, fmt, pos->row_begin, pos->col_begin,
				pos->row_end, pos->col_end, s , msg);
	} else {
		u_fprintf(ustderr, fmt, s , msg);
	}
	u_fflush(ustderr);
	_mthrow_(excep, s);
}

void vm_panic_bad_argc(svm_t vm, fcode_scp fc, ulong ip)
{
	vmiprep_scp v = (vmiprep_scp) fcod_get_vmi(fc, ip);
	assert(datum_is_instof_ix(v, TYPEIX_VMI_PREP) && "must be prep");
	UChar u = 0;
	vm_panic_f(vm, EXCEP_VM_BAD_ARGC, v->pos, " %S",
			(v->id ? ss_ucs( symbol_getname( id_symbol(v->id)))  : &u));
}

void vm_panic_bad_call(svm_t vm, fcode_scp fc, ulong ip)
{
	vmiprep_scp v = (vmiprep_scp) fcod_get_vmi(fc, ip);
	assert(datum_is_instof_ix(v, TYPEIX_VMI_PREP) && "must be prep");
	UChar u = 0;
	vm_panic_f(vm, EXCEP_VM_BAD_CALL, v->pos, "entity is not closure or continuation %S ",
			(v->id ? ss_ucs( symbol_getname( id_symbol(v->id)))  : &u));
}

void vm_panic_global_error(svm_t vm,  int ex, fcode_scp fc, ulong ip, int pos)
{
	vmiprep_scp v = (vmiprep_scp) fcod_get_vmi(fc, ip);
	ident_scp id = NIHIL;

	assert(scp_valid(v));
	switch(datum_get_typeix(v)) {
	case TYPEIX_VMI_VAR_LIT: id = ((vmivl_scp)v)->id;  break;
	case TYPEIX_VMI_VAR_VAR:
		if(pos == 1) {id = ((vmivv_scp)v)->id1;}
		else if(pos==2) {id = ((vmivv_scp)v)->id2;}
		else {assert(0 && "nh"); }
		break;
	case TYPEIX_VMI_VAR: id = ((vmiv_scp)v)->id;break;
	case TYPEIX_VMI_PREP: id = ((vmiprep_scp)v)->id;break;
	default:
		assert("nh" && 0);
	}
	txtpos_scp txp = id == NULL? NULL : id->pos;
	UChar u = 0;
	vm_panic_f(vm, ex, txp, "%S",
			(v->id ? ss_ucs( symbol_getname( id_symbol(id)))  : &u));
}



void fill_multiarg_vars(svm_t vm, __sbp(vec_scp) ploc, ulong count, ulong fixed)
{
	SAVE_RSTK();
	memcpy(vec_tail(_DS(ploc)), vm->binargs, fixed);
	PUSHA_RST(vm->binargs, fixed, count);
	GC_CHECK_OLD(_DS(ploc));
	descr d = args_to_cl(vm, vm->binargs,fixed, count);
	VEC_DSET(ploc, fixed, d);
	UNWIND_RSTK();
}

descr vm_run(svm_t vm, __arg fcode_scp fc)
{

	SAVE_RSTK();
	__loc descr r = SYMB_UNDEF;
	_try_
		r = icod_interpret(vm, fc);
	_except_
	_catchall_
		r = SYMB_VOID;
		log_excep_error(Sys_log);
		printf("\nvm_run() Exception caught: %s\n", excep_msg() );
		//_rethrow_;
	_end_try_
	UNWIND_RSTK();
	return r;
}



descr vm_eval(svm_t vm, __arg descr datum)
{

	SAVE_RSTK();
	decl_gc_of_vm();
	PUSH_RSTK(datum);
	__protect(transformer_scp, tr, new_transformer(vm));
	trf_check_syntax(vm, (transformer_scp*)_RS(tr), _RS(datum) );
	descr r = SYMB_UNDEF;
	if(!trfp_error(tr) ) {
		if(trf_transform_to_anf(vm, &tr, CONSCAR(datum))) {
			r = vm_run(vm, tr->gfc);
		}
	}
	UNWIND_RSTK();
	return r;
}

void vm_compile(svm_t vm, __arg descr datum, oport_scp ccode_op)
{

	SAVE_RSTK();
	decl_gc_of_vm();
	__protect(transformer_scp, tr, new_transformer(vm));
	trf_check_syntax(vm, (transformer_scp*)_RS(tr), _RS(datum) );
	descr r = SYMB_UNDEF;
	if(!trfp_error(tr) ) {
		if(trf_transform_to_anf(vm, &tr, CONSCAR(datum))) {
		//	r = vm_run(vm, tr->gfc);
			 icod_to_ccode(vm,  ccode_op, tr->gfc);
		}
	}
	UNWIND_RSTK();
}


#ifndef __GEN

/*top*/
descr  __attribute__((__noinline__)) icod_interpret(svm_t vm, __arg fcode_scp fc)
{


	if(fc == NULL) {
		// label initialization
		Label_icall = LABEL_ADDR(interpret_call);
		Label_iret = LABEL_ADDR(interpret_ret);
		/*init*/
		return;
	}
	assert(Label_icall);
	decl_gc_of_vm();

	SAVE_RSTK();
	PUSH_RSTK(fc); // fc - current fc
	assert(fc->closure != NULL);
	__protect(frame_scp,   fr_c, NIHIL); // frame of the call (after prepare
	__protect(closure_scp, cl_c, NIHIL); // current closure

	__protect(vec_scp, locals, new_vec(gc, fc->fvc) ); // local variables
	__protect(frame_scp,   fr_top, FRAME()  ); // top frame
		fr_top->vars = locals;
		fr_top->up = fr_top;
		fr_top->ret_cc = &&final;
		fr_top->closr = fc->closure;
	__protect(closure_scp, cl_top, fc->closure); //  closure top
	__loc descr rr = SYMB_VOID; // register of return
	descr *parg = NIHIL;
	ulong lnfr, i ;
	bool  tailcall = 0, multy = 0;
	fcode_scp lfc;
	frame_scp f;
	descr binargs[DEF_BINARGS_SZ];
	vm->binargs = binargs;
	__loc vec_scp c_locals; // vector of arguments
	ulong fargc = 0; // factual argc
	void *cc;
	descr glob;

	__protect(astk_scp, icod, NIHIL);
	ulong ip, ipmax;
	vmi_scp cmd;

	 GOTO_ADDR(fc->ccode);

interpret_call:
	ip = 0;
interpret_ret:
	icod = fc->icod;
	assert(cl_top->fcod == fc && "fc = cl.fc axiom");

	ipmax =  astk_size(icod);;


	 while(ip < ipmax){
		cmd = astk_tail(icod)[ip];
		switch(cmd->iun) {
		// SET FAMILY
		case desc_to_spec(SYMB_VM_SET_L_D): SET_VAR_L(((vmivl_scp)cmd)->var, ((vmivl_scp)cmd)->lit); IPNEXT; break;
		case desc_to_spec(SYMB_VM_SET_L_L): SET_VAR_L(((vmivv_scp)cmd)->var1, GET_VAR_L(((vmivv_scp)cmd)->var2)); IPNEXT; break;
		case desc_to_spec(SYMB_VM_SET_L_G): SET_VAR_L(((vmivv_scp)cmd)->var1,  GET_VAR_G(((vmivv_scp)cmd)->var2) ); IPNEXT; break;
		case desc_to_spec(SYMB_VM_SET_L_K): SET_VAR_L(((vmivv_scp)cmd)->var1, GET_VAR_K(((vmivv_scp)cmd)->var2)); IPNEXT;break;
		case desc_to_spec(SYMB_VM_SET_L_R): SET_VAR_L(((vmiv_scp)cmd)->var, rr); IPNEXT;break;

		case desc_to_spec(SYMB_VM_SET_K_D): SET_VAR_K(((vmivl_scp)cmd)->var, ((vmivl_scp)cmd)->lit); break;
		case desc_to_spec(SYMB_VM_SET_K_L): SET_VAR_K(((vmivv_scp)cmd)->var1, GET_VAR_L(((vmivv_scp)cmd)->var2)); IPNEXT;break;
		case desc_to_spec(SYMB_VM_SET_K_G):  SET_VAR_K(((vmivv_scp)cmd)->var1, GET_VAR_G(((vmivv_scp)cmd)->var2) ); IPNEXT;break;
		case desc_to_spec(SYMB_VM_SET_K_K): SET_VAR_K(((vmivv_scp)cmd)->var1, GET_VAR_K(((vmivv_scp)cmd)->var2)); IPNEXT;break;
		case desc_to_spec(SYMB_VM_SET_K_R): SET_VAR_K(((vmiv_scp)cmd)->var, rr); IPNEXT;break;

		case desc_to_spec(SYMB_VM_SET_G_D): SET_VAR_G(((vmivl_scp)cmd)->var, ((vmivl_scp)cmd)->lit); IPNEXT;break;
		case desc_to_spec(SYMB_VM_SET_G_L): SET_VAR_G(((vmivv_scp)cmd)->var1, GET_VAR_L(((vmivv_scp)cmd)->var2)); IPNEXT;break;
		case desc_to_spec(SYMB_VM_SET_G_G): SET_VAR_G(((vmivv_scp)cmd)->var1, GET_VAR_G(((vmivv_scp)cmd)->var2) ); IPNEXT;break;
		case desc_to_spec(SYMB_VM_SET_G_K): SET_VAR_G(((vmivv_scp)cmd)->var1, GET_VAR_K(((vmivv_scp)cmd)->var2)); IPNEXT;break;
		case desc_to_spec(SYMB_VM_SET_G_R): SET_VAR_G(((vmiv_scp)cmd)->var, rr); IPNEXT;break;

		// DEF family
		case desc_to_spec(SYMB_VM_DEF_D): INIT_VARIX_G(ip, ((vmivl_scp)cmd)->var.ix, ((vmivl_scp)cmd)->lit); IPNEXT;break;
		case desc_to_spec(SYMB_VM_DEF_G): INIT_VARIX_G(ip, ((vmivv_scp)cmd)->var1.ix, GET_VAR_G(((vmivv_scp)cmd)->var2) );IPNEXT; break;
		case desc_to_spec(SYMB_VM_DEF_R): INIT_VARIX_G(ip,  ((vmiv_scp)cmd)->var.ix, rr);IPNEXT; break;

		// IF MAMILY
		case desc_to_spec(SYMB_VM_IF_L): if ( GET_VAR_L(((vmivl_scp)cmd)->var) == SYMB_F) { ip = desc_to_fixnum(((vmivl_scp)cmd)->lit); } else {IPNEXT;} break;
		case desc_to_spec(SYMB_VM_IF_G):   if (GET_VAR_G(((vmivl_scp)cmd)->var) == SYMB_F) { ip = desc_to_fixnum(((vmivl_scp)cmd)->lit); } else {IPNEXT;} break;
		case desc_to_spec(SYMB_VM_IF_K): if ( GET_VAR_K(((vmivl_scp)cmd)->var) == SYMB_F) { ip = desc_to_fixnum(((vmivl_scp)cmd)->lit); } else {IPNEXT;} break;
		case desc_to_spec(SYMB_VM_IF_R): if ( rr == SYMB_F) { ip = desc_to_fixnum(((vmil_scp)cmd)->lit); } else {IPNEXT;} break;

		// JMP
		case desc_to_spec(SYMB_VM_JMP): ip  = desc_to_fixnum(((vmil_scp)cmd)->lit); break;
		//MARK
		case desc_to_spec(SYMB_VM_MARK): IPNEXT; break;

		case desc_to_spec(SYMB_VM_LAMBDA):
		{
			lfc = (fcode_scp) ((vmil_scp)cmd)->lit;
			rr = new_closure(vm, lfc );
			lnfr = fcod_framemax(lfc);
			if(lnfr) {
				parg = (descr*) closr_tail(rr);
				for(f = fr_top, i = 0; i < lnfr; i++, f = f->up ) {
					if(!f) {
						_mthrow_(EXCEP_INTERPRET_FATAL, "not enough frames for !LMD");
					}
					parg[i] = f->vars;
				}
			}

		}
		IPNEXT;
		break;





		//PREP
		case desc_to_spec(SYMB_VM_PREP_L): cl_c = (closure_scp) GET_VAR_L(((vmiprep_scp)cmd)->var);	goto prepare;
		case desc_to_spec(SYMB_VM_PREP_G):  cl_c = (closure_scp) GET_VAR_G(((vmiprep_scp)cmd)->var) ;	goto prepare;
		case desc_to_spec(SYMB_VM_PREP_K): cl_c = (closure_scp) GET_VAR_K(((vmiprep_scp)cmd)->var);	goto prepare;
		case desc_to_spec(SYMB_VM_PREP_R): cl_c = (closure_scp) rr;
prepare:
			if(!datum_is_instof_ix(cl_c, TYPEIX_CLOSURE)) {
				vm_panic_bad_call(vm, fc, ip);
			}
			fargc = ((vmiprep_scp)cmd)->fargc; multy = closrp_multiarg(cl_c);
			unless( (!multy && fargc == cl_c->argc) || (multy && fargc >= cl_c->argc)) {
				vm_panic_bad_argc(vm, fc, ip);
			}
			if(cl_c->bin || multy ) {
				parg = binargs;
			} else {
				c_locals = new_vec(gc, fcod_varcount(cl_c->fcod));
				parg = vec_tail(c_locals);
			}
			IPNEXT;
		break;


		//PREP
		case desc_to_spec(SYMB_VM_RPREP_L): cl_c = (closure_scp) GET_VAR_L(((vmiprep_scp)cmd)->var);	goto rprepare;
		case desc_to_spec(SYMB_VM_RPREP_G): cl_c = (closure_scp) GET_VAR_G(((vmiprep_scp)cmd)->var);	goto rprepare;
		case desc_to_spec(SYMB_VM_RPREP_K): cl_c = (closure_scp) GET_VAR_K(((vmiprep_scp)cmd)->var);	goto rprepare;
		case desc_to_spec(SYMB_VM_RPREP_R): cl_c = (closure_scp) rr;
rprepare:
			if(!datum_is_instof_ix(cl_c, TYPEIX_CLOSURE)) {
				vm_panic_bad_call(vm, fc, ip);
			}
			fargc = ((vmiprep_scp)cmd)->fargc;
			tailcall = (cl_c == cl_top);
			multy = closrp_multiarg(cl_c);
			unless( (!multy && fargc == cl_c->argc) || (multy && fargc >= cl_c->argc)) {
				vm_panic_bad_argc(vm, fc, ip);
			}
			if(cl_c->bin || multy ) {
				parg = binargs;
			} else {
				c_locals = tailcall ? locals :  new_vec(gc, fcod_varcount(cl_c->fcod));
				parg = vec_tail(c_locals);
			}
			IPNEXT;
		break;

		case desc_to_spec(SYMB_VM_ARGPUSH_D): *parg++ = ((vmil_scp)cmd)->lit; IPNEXT; break;
		case desc_to_spec(SYMB_VM_ARGPUSH_L): *parg++ = GET_VAR_L(((vmiv_scp)cmd)->var); IPNEXT; break;
		case desc_to_spec(SYMB_VM_ARGPUSH_G):  *parg++ = GET_VAR_G(((vmiv_scp)cmd)->var);  IPNEXT; break;
		case desc_to_spec(SYMB_VM_ARGPUSH_K): *parg++ = GET_VAR_K(((vmiv_scp)cmd)->var); IPNEXT; break;

		case desc_to_spec(SYMB_VM_CALL):
			if(cl_c->bin) {
			//	printf("[call-bin] ");
				rr = cl_c->bin(vm, fargc, binargs);
				IPNEXT;
			} else {
				PUSH_RSTK(c_locals);
			//	printf("[call] ");
				if(multy) {
					fill_multiarg_vars(vm, &c_locals, cl_c->argc, fargc);
				}
				fr_c = (frame_scp) inew_inst(gc, TYPEIX_FRAME);
				fr_c->closr = cl_c; fr_c->vars = c_locals; fr_c->up = fr_top;
				FRAME_SET_IRET(fr_c);
				POP_RSTK();
				c_locals = NIHIL;
				FRAME_PUSH;
				CLOSR_JMP;
			}
			break;
		case desc_to_spec(SYMB_VM_RETCALL):

			if(cl_c->bin) {
			//	printf("[retcall-bin] ");
				rr = cl_c->bin(vm, fargc, binargs);
				FRAME_RETPOP;
			} else {
				PUSH_RSTK(c_locals);
				if(multy) {
					fill_multiarg_vars(vm, &c_locals, cl_c->argc, fargc);
				}
				if(tailcall) {
				//	printf("[tail] ");
					POP_RSTK();
					assert(c_locals == locals);
					GC_CHECK_OLD(c_locals);
					CLOSR_JMP;
				} else {
					//printf("[retcall] ");
					fr_c = (frame_scp) inew_inst(gc, TYPEIX_FRAME);
					fr_c->closr = cl_c; fr_c->vars = c_locals;
					fr_c->up = fr_top->up;
					fr_c->ret_cc = fr_top->ret_cc;
					fr_c->ret_ip = fr_top->ret_ip;
					POP_RSTK();
					FRAME_PUSH;
					CLOSR_JMP;
				}
			}
			break;
		case desc_to_spec(SYMB_VM_RET_D): rr = ((vmil_scp)cmd)->lit; FRAME_RETPOP; break;
		case desc_to_spec(SYMB_VM_RET_L): rr = GET_VAR_L( ((vmiv_scp)cmd)->var);  FRAME_RETPOP; break;
		case desc_to_spec(SYMB_VM_RET_G): rr = GET_VAR_G( ((vmiv_scp)cmd)->var);  FRAME_RETPOP; break;
		case desc_to_spec(SYMB_VM_RET_K): rr = GET_VAR_K( ((vmiv_scp)cmd)->var); FRAME_RETPOP;
			break;
		case desc_to_spec(SYMB_VM_RET_R): FRAME_RETPOP; break;

		default:
			_fthrow_(EXCEP_INTERPRET_FATAL, "icode unknown instruction %lu",  cmd->iun);
		}
	}

/*fin*/
 final:
	//oport_write_asci(Out_console_port, "\nResult: >> ");
	//oport_print_datum(vm, Out_console_port, rr);
	UNWIND_RSTK();
	return rr;
 }

#else
#include "__gentop.h"
#include "__genbody.h"
#include "__genfin.h"
#endif












const char* msg_not_enough_frames = "Not enough frames";


#define ccod_start_br(op) ccod_print_asci(op, "(")
#define ccod_end_br(op) ccod_print_asci(op, ")")
#define ccod_print_ul(op, x)  ccod_printf(op, sbuf, "%lu", x )
#define ccod_print_asci(op, asci) oport_write_asci(op, asci)
#define ccod_printf(op, fs, format, ...) \
	sprintf(fs, format, __VA_ARGS__); \
	ccod_print_asci(op, fs);




#define DEF_FSZ 1024
char sbuf[DEF_FSZ];

void ccod_print_v(oport_scp op, varinfo_t *v1)
{
	if(v1->fr) {
		ccod_printf(op, sbuf, "%lu, %lu", v1->fr, v1->ix);
	} else {
		ccod_printf(op, sbuf, "%lu", v1->ix);
	}
}

void ccod_print_d(oport_scp op, fcode_scp fc,   ulong ip, ulong offs)
{
	descr d = GET_DATUM(ip, offs );
	if(scp_valid(d)) {
		ccod_printf(op, sbuf, "GET_DATUM(%lu, %lu)",  ip, offs);
	} else {
		ccod_printf(op, sbuf, "((descr)(%p))",  d);
	}

}

void ccod_print_set_vv(oport_scp op, ulong ip,  const char* set, varinfo_t *v1,  const char* get, varinfo_t *v2)
{
	ccod_print_asci(op, set); ccod_print_asci(op, "(");
		ccod_print_ul(op, ip); ccod_print_asci(op, ",");
		ccod_print_v(op, v1); ccod_print_asci(op, ",");
		ccod_print_asci(op, get); ccod_print_asci(op, "(");
			ccod_print_ul(op, ip); ccod_print_asci(op, ",");
			ccod_print_v(op, v2);
		ccod_print_asci(op, ")");
	ccod_print_asci(op, ");\n");

}


void ccod_print_set_vd(oport_scp op, fcode_scp fc, ulong ip, const char* set, varinfo_t *v1,   ulong offs)
{
	ccod_print_asci(op, set); ccod_print_asci(op, "(");
		ccod_print_ul(op, ip); ccod_print_asci(op, ",");
		ccod_print_v(op, v1);  ccod_print_asci(op, ",");
		ccod_print_d(op, fc, ip, offs);
	ccod_print_asci(op, ");\n");


}


void ccod_print_set_vr(oport_scp op, ulong ip, const char* set, varinfo_t *v)
{
	ccod_print_asci(op, set); ccod_print_asci(op, "(");
			ccod_print_ul(op, ip); ccod_print_asci(op, ",");
			ccod_print_v(op, v); ccod_print_asci(op, ",");
			ccod_print_asci(op, "rr");
	ccod_print_asci(op, ");\n");

}

void ccod_print_mark_line(oport_scp op, ulong uniq, ulong ip)
{
	ccod_print_asci(op, "____M_LINE_");
	ccod_print_ul(op, uniq);
	ccod_print_asci(op, "_");
	ccod_print_ul(op, ip);
}

void ccod_print_mark_fcode(oport_scp op, ulong uniq)
{
	ccod_print_asci(op, "____M_FCODE_");
	ccod_print_ul(op, uniq);
}

void ccod_print_if_v(oport_scp op, ulong ip, const char* get, varinfo_t *v,  ulong gotouniq,  ulong gotoip)
{
	ccod_print_asci(op, "if (");
				ccod_print_asci(op, get); ccod_print_asci(op, "(");
					ccod_print_ul(op, ip); ccod_print_asci(op, ",");
					ccod_print_v(op, v); ccod_print_asci(op, ",");
				ccod_print_asci(op, ")");
				ccod_print_asci(op, "== SYMB_F) goto ");
				ccod_print_mark_line(op, gotouniq, gotoip);
	ccod_print_asci(op, ";\n");
}

void ccod_print_if_r(oport_scp op, ulong ip, ulong gotouniq,  ulong gotoip)
{
	ccod_print_asci(op, "if (rr");
				ccod_print_asci(op, "== SYMB_F) goto ");
				ccod_print_mark_line(op, gotouniq, gotoip);
	ccod_print_asci(op, ";\n");
}



void icod_to_ccode(svm_t vm, oport_scp op, __arg fcode_scp fc)
{

	ccod_print_asci(Geninit_oport, "Fcodes[Fcodes_size++] = && ");
	ccod_print_mark_fcode(Geninit_oport, fc->uniq);
	ccod_print_asci(Geninit_oport, ";\n");

	ccod_print_mark_fcode(op, fc->uniq);
	ccod_print_asci(op, ":\n");
	astk_scp icod = fc->icod;
	ulong ipmax =  astk_size(icod);
	vmi_scp cmd;
	ulong fargc = 0;



	for(ulong ip = 0;ip < ipmax;ip++) {
		cmd = astk_tail(icod)[ip];
		if(vmi_is_marked(cmd) || vmi_is_callmarked(cmd)) {
			ccod_print_mark_line(op, fc->uniq, ip);
			ccod_print_asci(op, ":\n\t");
		} else {
			ccod_print_asci(op, "\t");
		}
		switch(cmd->iun) {
		case desc_to_spec(SYMB_VM_SET_L_D):
			ccod_print_set_vd(op, fc, ip, "SET_VARIX_L",  &((vmivl_scp)cmd)->var,  field_offs_scp(vmivl_scp, lit)); break;
		case desc_to_spec(SYMB_VM_SET_L_L):
			ccod_print_set_vv(op, ip, "SET_VARIX_L", &((vmivv_scp)cmd)->var1, "GET_VARIX_L", &((vmivv_scp)cmd)->var2);  break;
		case desc_to_spec(SYMB_VM_SET_L_G):
			ccod_print_set_vv(op, ip, "SET_VARIX_L", &((vmivv_scp)cmd)->var1, "GET_VARIX_G", &((vmivv_scp)cmd)->var2);  break;
		case desc_to_spec(SYMB_VM_SET_L_K):
			ccod_print_set_vv(op, ip, "SET_VARIX_L", &((vmivv_scp)cmd)->var1, "GET_VARIX_K", &((vmivv_scp)cmd)->var2);  break;
		case desc_to_spec(SYMB_VM_SET_L_R):
			ccod_print_set_vr(op, ip, "SET_VARIX_L", &((vmiv_scp)cmd)->var);  break;

		case desc_to_spec(SYMB_VM_SET_G_D):
			ccod_print_set_vd(op, fc, ip, "SET_VARIX_G", &((vmivl_scp)cmd)->var, field_offs_scp(vmivl_scp, lit)); break;
		case desc_to_spec(SYMB_VM_SET_G_L):
			ccod_print_set_vv(op, ip, "SET_VARIX_G", &((vmivv_scp)cmd)->var1, "GET_VARIX_L", &((vmivv_scp)cmd)->var2);  break;
		case desc_to_spec(SYMB_VM_SET_G_G):
			ccod_print_set_vv(op, ip, "SET_VARIX_G", &((vmivv_scp)cmd)->var1, "GET_VARIX_G", &((vmivv_scp)cmd)->var2);  break;
		case desc_to_spec(SYMB_VM_SET_G_K):
			ccod_print_set_vv(op, ip, "SET_VARIX_G", &((vmivv_scp)cmd)->var1, "GET_VARIX_K", &((vmivv_scp)cmd)->var2);  break;
		case desc_to_spec(SYMB_VM_SET_G_R):
			ccod_print_set_vr(op, ip, "SET_VARIX_G", &((vmiv_scp)cmd)->var);  break;

		case desc_to_spec(SYMB_VM_SET_K_D):

			ccod_print_set_vd(op, fc,ip, "SET_VARIX_K", &((vmivl_scp)cmd)->var,  field_offs_scp(vmivl_scp, lit)); break;
		case desc_to_spec(SYMB_VM_SET_K_L):
			ccod_print_set_vv(op, ip, "SET_VARIX_K", &((vmivv_scp)cmd)->var1, "GET_VARIX_L", &((vmivv_scp)cmd)->var2);  break;
		case desc_to_spec(SYMB_VM_SET_K_G):
			ccod_print_set_vv(op, ip, "SET_VARIX_K", &((vmivv_scp)cmd)->var1, "GET_VARIX_G", &((vmivv_scp)cmd)->var2);  break;
		case desc_to_spec(SYMB_VM_SET_K_K):
			ccod_print_set_vv(op, ip, "SET_VARIX_K", &((vmivv_scp)cmd)->var1, "GET_VARIX_K", &((vmivv_scp)cmd)->var2);  break;
		case desc_to_spec(SYMB_VM_SET_K_R):
			ccod_print_set_vr(op, ip, "SET_VARIX_K", &((vmiv_scp)cmd)->var);  break;


		case desc_to_spec(SYMB_VM_DEF_D):
			ccod_print_set_vd(op, fc,ip, "INIT_VARIX_G", &((vmivl_scp)cmd)->var,  field_offs_scp(vmivl_scp, lit)); break;
		case desc_to_spec(SYMB_VM_DEF_G):
			ccod_print_set_vv(op, ip, "INIT_VARIX_G", &((vmivv_scp)cmd)->var1, "GET_VARIX_G", &((vmivv_scp)cmd)->var2);  break;
		case desc_to_spec(SYMB_VM_DEF_R):
			ccod_print_set_vr(op, ip, "INIT_VARIX_G", &((vmiv_scp)cmd)->var);  break;

		case desc_to_spec(SYMB_VM_IF_L):
			ccod_print_if_v(op, ip, "GET_VARIX_L", &((vmivl_scp)cmd)->var, fc->uniq, desc_to_fixnum(((vmivl_scp)cmd)->lit) ); break;
		case desc_to_spec(SYMB_VM_IF_G):
			ccod_print_if_v(op, ip, "GET_VARIX_G", &((vmivl_scp)cmd)->var, fc->uniq, desc_to_fixnum(((vmivl_scp)cmd)->lit) ); break;

		case desc_to_spec(SYMB_VM_IF_K):
			ccod_print_if_v(op, ip, "GET_VARIX_K", &((vmivl_scp)cmd)->var, fc->uniq, desc_to_fixnum(((vmivl_scp)cmd)->lit) ); break;
		case desc_to_spec(SYMB_VM_IF_R):
			ccod_print_if_r( op,  ip, fc->uniq, desc_to_fixnum(((vmil_scp)cmd)->lit)); break;

		case desc_to_spec(SYMB_VM_JMP):
			ccod_print_asci(op, "goto "); ccod_print_mark_line(op, fc->uniq, desc_to_fixnum(((vmil_scp)cmd)->lit));
			ccod_print_asci(op, ";\n");  break;

		case desc_to_spec(SYMB_VM_LAMBDA):
			ccod_print_asci(op, "LOAD_CLOSURE(");
			ccod_print_ul(op, ip);
			ccod_print_asci(op, ");\n");
			break;
			//ccod_print_asci(op, "goto ")

		case desc_to_spec(SYMB_VM_PREP_L):
			fargc=((vmiprep_scp)cmd)->fargc;
			ccod_print_asci(op,"PREPARE_CALL(");ccod_print_asci(op,"GET_VARIX_L(");ccod_print_ul(op, ip);
			ccod_print_asci(op,","); ccod_print_v(op, &((vmiprep_scp)cmd)->var);ccod_print_asci(op,")");
			ccod_print_asci(op,",");ccod_print_ul(op, ip); ccod_print_asci(op,","); ccod_print_ul(op,((vmiprep_scp)cmd)->fargc);
			ccod_print_asci(op,");\n");break;
		case desc_to_spec(SYMB_VM_PREP_G):
			fargc=((vmiprep_scp)cmd)->fargc;
			ccod_print_asci(op,"PREPARE_CALL(");ccod_print_asci(op,"GET_VARIX_G(");ccod_print_ul(op, ip);
						ccod_print_asci(op,","); ccod_print_v(op, &((vmiprep_scp)cmd)->var);ccod_print_asci(op,")");
						ccod_print_asci(op,",");ccod_print_ul(op, ip); ccod_print_asci(op,","); ccod_print_ul(op,((vmiprep_scp)cmd)->fargc);
						ccod_print_asci(op,");\n");break;
		case desc_to_spec(SYMB_VM_PREP_K):
			fargc=((vmiprep_scp)cmd)->fargc;
			ccod_print_asci(op,"PREPARE_CALL(");ccod_print_asci(op,"GET_VARIX_K(");ccod_print_ul(op, ip);
						ccod_print_asci(op,","); ccod_print_v(op, &((vmiprep_scp)cmd)->var);ccod_print_asci(op,")");
						ccod_print_asci(op,",");ccod_print_ul(op, ip); ccod_print_asci(op,","); ccod_print_ul(op,((vmiprep_scp)cmd)->fargc);
						ccod_print_asci(op,");\n");break;
		case desc_to_spec(SYMB_VM_PREP_R):
			fargc=((vmiprep_scp)cmd)->fargc;
			ccod_print_asci(op,"PREPARE_CALL(");ccod_print_asci(op,"rr");ccod_print_asci(op,",");
						ccod_print_ul(op, ip); ccod_print_asci(op,","); ccod_print_ul(op,((vmiprep_scp)cmd)->fargc);
						ccod_print_asci(op,");\n");break;


		case desc_to_spec(SYMB_VM_RPREP_L):
			fargc=((vmiprep_scp)cmd)->fargc;
			ccod_print_asci(op,"PREPARE_RETCALL(");ccod_print_asci(op,"GET_VARIX_L(");ccod_print_ul(op, ip);
			ccod_print_asci(op,","); ccod_print_v(op, &((vmiprep_scp)cmd)->var);ccod_print_asci(op,")");
			ccod_print_asci(op,",");ccod_print_ul(op, ip); ccod_print_asci(op,","); ccod_print_ul(op,((vmiprep_scp)cmd)->fargc);
			ccod_print_asci(op,");\n");break;
		case desc_to_spec(SYMB_VM_RPREP_G):
			fargc=((vmiprep_scp)cmd)->fargc;
			ccod_print_asci(op,"PREPARE_RETCALL(");ccod_print_asci(op,"GET_VARIX_G(");ccod_print_ul(op, ip);
						ccod_print_asci(op,","); ccod_print_v(op, &((vmiprep_scp)cmd)->var);ccod_print_asci(op,")");
						ccod_print_asci(op,",");ccod_print_ul(op, ip); ccod_print_asci(op,","); ccod_print_ul(op,((vmiprep_scp)cmd)->fargc);
						ccod_print_asci(op,");\n");break;
		case desc_to_spec(SYMB_VM_RPREP_K):
			fargc=((vmiprep_scp)cmd)->fargc;
			ccod_print_asci(op,"PREPARE_RETCALL(");ccod_print_asci(op,"GET_VARIX_K(");ccod_print_ul(op, ip);
						ccod_print_asci(op,","); ccod_print_v(op, &((vmiprep_scp)cmd)->var);ccod_print_asci(op,")");
						ccod_print_asci(op,",");ccod_print_ul(op, ip); ccod_print_asci(op,","); ccod_print_ul(op,((vmiprep_scp)cmd)->fargc);
						ccod_print_asci(op,");\n");break;
		case desc_to_spec(SYMB_VM_RPREP_R):
			fargc=((vmiprep_scp)cmd)->fargc;
			ccod_print_asci(op,"PREPARE_RETCALL(");ccod_print_asci(op,"rr");ccod_print_asci(op,",");
						ccod_print_ul(op, ip); ccod_print_asci(op,","); ccod_print_ul(op,((vmiprep_scp)cmd)->fargc);
						ccod_print_asci(op,");\n");break;


		case desc_to_spec(SYMB_VM_CALL):

			ccod_print_asci(op,"CALL(");ccod_print_ul(op, fc->uniq); ccod_print_asci(op,",");
			ccod_print_ul(op, ip); ccod_print_asci(op,",");
			ccod_print_ul(op, ip+1); ccod_print_asci(op,",");
			ccod_print_ul(op, fargc);ccod_print_asci(op,");\n");
			vmi_set_callmarked( ((vmi_scp)astk_tail(icod)[ip+1]) );
			break;
		case desc_to_spec(SYMB_VM_RETCALL):
				ccod_print_asci(op,"RETCALL(");ccod_print_ul(op, ip); ccod_print_asci(op,",");
				ccod_print_ul(op, fargc);ccod_print_asci(op,");\n"); break;



		case desc_to_spec(SYMB_VM_ARGPUSH_D):
			ccod_print_asci(op,"ARG_PUSH(");ccod_print_d(op, fc,ip, field_offs_scp(vmil_scp, lit) );
			ccod_print_asci(op,");\n"); break;
		case desc_to_spec(SYMB_VM_ARGPUSH_L):
			ccod_print_asci(op,"ARG_PUSH(");ccod_print_asci(op,"GET_VARIX_L(");ccod_print_ul(op, ip);
			ccod_print_asci(op,","); ccod_print_v(op, &((vmiv_scp)cmd)->var);ccod_print_asci(op,")");
			ccod_print_asci(op,");\n"); break;
			; break;
		case desc_to_spec(SYMB_VM_ARGPUSH_G):
			ccod_print_asci(op,"ARG_PUSH(");ccod_print_asci(op,"GET_VARIX_G(");ccod_print_ul(op, ip);
					ccod_print_asci(op,","); ccod_print_v(op, &((vmiv_scp)cmd)->var);ccod_print_asci(op,")");
					ccod_print_asci(op,");\n"); break;
			break;
		case desc_to_spec(SYMB_VM_ARGPUSH_K):
			ccod_print_asci(op,"ARG_PUSH(");ccod_print_asci(op,"GET_VARIX_K(");ccod_print_ul(op, ip);
					ccod_print_asci(op,","); ccod_print_v(op, &((vmiv_scp)cmd)->var);ccod_print_asci(op,")");
					ccod_print_asci(op,");\n"); break;
			break;





		case desc_to_spec(SYMB_VM_RET_D):
			ccod_print_asci(op,"RET(");ccod_print_d(op,fc, ip, field_offs_scp(vmil_scp, lit) );
				ccod_print_asci(op,");\n"); break;
			case desc_to_spec(SYMB_VM_RET_L):
				ccod_print_asci(op,"RET(");ccod_print_asci(op,"GET_VARIX_L(");ccod_print_ul(op, ip);
							ccod_print_asci(op,","); ccod_print_v(op, &((vmiv_scp)cmd)->var);ccod_print_asci(op,")");
							ccod_print_asci(op,");\n"); break;
			case desc_to_spec(SYMB_VM_RET_G):
				ccod_print_asci(op,"RET(");ccod_print_asci(op,"GET_VARIX_L(");ccod_print_ul(op, ip);
							ccod_print_asci(op,","); ccod_print_v(op, &((vmiv_scp)cmd)->var);ccod_print_asci(op,")");
							ccod_print_asci(op,");\n"); break;
			case desc_to_spec(SYMB_VM_RET_K):
				ccod_print_asci(op,"RET(");ccod_print_asci(op,"GET_VARIX_K(");ccod_print_ul(op, ip);
							ccod_print_asci(op,","); ccod_print_v(op, &((vmiv_scp)cmd)->var);ccod_print_asci(op,")");
							ccod_print_asci(op,");\n"); break;
			case desc_to_spec(SYMB_VM_RET_R):
				ccod_print_asci(op,"RET(rr);\n"); break;
		default:
			ccod_print_asci(op, "<<UNKNOWN>>;\n");
		}
	}


	cons_scp p = fc->children;
	until(cl_end(p)) {
		icod_to_ccode(vm, op, CAR(p));
		p = CDR(p);
	}




}




