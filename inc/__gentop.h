/* __gentop.h
 * scheme translator & interpreter
 * author: denis.lepekhin@gmail.com
 * created on: 2009
 */

#ifndef __GENTOP_H_
#define __GENTOP_H_
descr  __attribute__((__noinline__)) icod_interpret(svm_t vm, __arg fcode_scp fc)
{


	if(fc == NULL) {
		// label initialization
		Label_icall = LABEL_ADDR(interpret_call);
		Label_iret = LABEL_ADDR(interpret_ret);
#include "__geninit.h"
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
	__protect(fcode_scp, lfc, 0);
	frame_scp f = 0;
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
#endif /* __GENTOP_H_ */
