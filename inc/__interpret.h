/* __interpret.h
 * musc- scheme translator & interpreter
 * author: denis.lepekhin@gmail.com
 * created on: 2009
 */

#ifndef __INTERPRET_H_
#define __INTERPRET_H_


#define GET_VARIX_L(_ip, ix) (vec_tail(locals)[ix])
#define SET_VARIX_L(_ip, ix, _a) VEC_SET(locals, ix, _a)

#define GET_VAR_L(v )    GET_VARIX_L(ip, (v).ix)
#define SET_VAR_L(v, _a) SET_VARIX_L(ip, (v).ix, _a)


#define __PANIC_GLOBAL_UNDEF(_ip, _pos ) vm_panic_global_error(vm,EXCEP_VM_GLOBAL_UNDEF, fc, _ip, _pos)
#define __PANIC_GLOBAL_REDEF(_ip, _pos ) vm_panic_global_error(vm,EXCEP_VM_GLOBAL_DUP, fc, _ip, _pos)

inline descr __glob_get( svm_t vm, fcode_scp fc, ulong _ip, ulong ix);


#define GET_VARIX_G(_ip, ix)     __glob_get(vm,fc,_ip,ix)
#define SET_VARIX_G(_ip, ix, _a)  glob = __global(ix); if(!glob) __PANIC_GLOBAL_UNDEF(_ip, 1); box_set(glob, _a)

#define INIT_VARIX_G(_ip, ix, _a)   if(__global(ix)) __PANIC_GLOBAL_REDEF(_ip, 1);  global_init(vm, ix, _a )

#define GET_VAR_G(v)  GET_VARIX_G(ip, (v).ix)
#define SET_VAR_G(v, _a) SET_VARIX_G(ip, (v).ix, _a)



#define GET_VARIX_K(_ip, _fr, _ix)  ( vec_tail(closr_tail(cl_top)[(_fr)-1])   [(_ix)])
#define SET_VARIX_K(_ip, _fr, _ix, _a)        VEC_SET((closr_tail(cl_top)[(_fr)-1]),_ix, _a)

#define GET_VAR_K(v)  GET_VARIX_K(ip, (v).fr, (v).ix)
#define SET_VAR_K(v, _a) SET_VARIX_K(ip, (v).fr, (v).ix, _a)


#define GET_DATUM(ip, offs) (*(descr*)(((char*)fcod_get_vmi(fc, ip)) + offs))

#define IPNEXT \
/*	printf("[%lu(%lu)(%lu):%lu]  ",  fc->uniq,((fcode_scp) ((closure_scp)fr_top->closr)->fcod)->uniq,fr_top->ret_ip ,ip);*/\
	ip++

#define FRAME_POP  fr_top = fr_top->up; cl_top = (closure_scp) fr_top->closr; fc = cl_top->fcod; locals = fr_top->vars
#define FRAME_PUSH fr_top = fr_c;fr_c=NULL; cl_top = (closure_scp) cl_c; cl_c=NULL; locals = fr_top->vars; fc = cl_top->fcod
#define CLOSR_JMP   GOTO_ADDR(fc->ccode)
#define FRAME_RETPOP       ip  = fr_top->ret_ip; cc = fr_top->ret_cc;  FRAME_POP; GOTO_ADDR(cc)
#define FRAME_SET_IRET(f) f->ret_cc = Label_iret; f->ret_ip = ip + 1;


#define FRAME()   (frame_scp) inew_inst(gc, TYPEIX_FRAME)
#define DEF_BINARGS_SZ 512





#define __VMIL_LIT_OFFS   field_offs_scp(vmil_scp, lit)
extern const char* msg_not_enough_frames;
#define LOAD_CLOSURE(ip) \
		lfc = (fcode_scp)GET_DATUM(ip, __VMIL_LIT_OFFS ); \
		rr = new_closure(vm, lfc ); \
		lnfr = fcod_framemax(lfc);\
		if(lnfr) {\
			parg = (descr*) closr_tail(rr);\
			for(f = fr_top, i = 0; i < lnfr; i++, f = f->up ) {\
				if(!f) {\
					_mthrow_(EXCEP_INTERPRET_FATAL, msg_not_enough_frames);\
				}\
				parg[i] = f->vars;\
			}\
		}

#define PREPARE_CALL(_v, ip, fargc) cl_c=(closure_scp)(_v); \
		if(!datum_is_instof_ix(cl_c, TYPEIX_CLOSURE)) {\
			vm_panic_bad_call(vm, fc, ip);\
		}\
		multy = closrp_multiarg(cl_c);\
		unless( (!multy && fargc == cl_c->argc) || (multy && fargc >= cl_c->argc)) {\
			vm_panic_bad_argc(vm, fc, ip);\
		}\
		if(cl_c->bin || multy ) {\
			parg = binargs;\
		} else {\
			c_locals = new_vec(gc, fcod_varcount(cl_c->fcod));\
			parg = vec_tail(c_locals);\
		}

#define PREPARE_RETCALL(_v, ip, fargc) cl_c=(closure_scp)(_v); \
			if(!datum_is_instof_ix(cl_c, TYPEIX_CLOSURE)) { \
				vm_panic_bad_call(vm, fc, ip);\
			}\
			tailcall = (cl_c == cl_top);\
			multy = closrp_multiarg(cl_c);\
			unless( (!multy && fargc == cl_c->argc) || (multy && fargc >= cl_c->argc)) {\
				vm_panic_bad_argc(vm, fc, ip);\
			}\
			if(cl_c->bin || multy ) {\
				parg = binargs;\
			} else {\
				c_locals = tailcall ? locals :  new_vec(gc, fcod_varcount(cl_c->fcod));\
				parg = vec_tail(c_locals);\
			}

#define CALL(uniq, ip,ipnext, fargc)\
			if(cl_c->bin) {\
				rr = cl_c->bin(vm, fargc, binargs);\
			} else {\
				PUSH_RSTK(c_locals);\
				if(multy) {\
					fill_multiarg_vars(vm, &c_locals, cl_c->argc, fargc);\
				}\
				fr_c = (frame_scp) inew_inst(gc, TYPEIX_FRAME);\
				fr_c->closr = cl_c; fr_c->vars = c_locals; fr_c->up = fr_top;\
				fr_c->ret_cc =&& ____M_LINE_##uniq##_##ipnext;\
				POP_RSTK();\
				c_locals = NIHIL;\
				FRAME_PUSH;\
				CLOSR_JMP;\
			}

#define RETCALL(ip, fargc)\
		if(cl_c->bin) {\
			rr = cl_c->bin(vm, fargc, binargs);\
			FRAME_RETPOP;\
		} else {\
			PUSH_RSTK(c_locals);\
			if(multy) {\
				fill_multiarg_vars(vm, &c_locals, cl_c->argc, fargc);\
			}\
			if(tailcall) {\
				POP_RSTK();\
				GC_CHECK_OLD(c_locals);\
				CLOSR_JMP;\
			} else {\
				fr_c = (frame_scp) inew_inst(gc, TYPEIX_FRAME);\
				fr_c->closr = cl_c; fr_c->vars = c_locals;\
				fr_c->up = fr_top->up; fr_c->ret_cc = fr_top->ret_cc; fr_c->ret_ip = fr_top->ret_ip;\
				POP_RSTK();\
				FRAME_PUSH;\
				CLOSR_JMP;\
			}\
		}


#define ARG_PUSH(_v)  *parg++ = _v



#define RET(_v)  rr = _v; FRAME_RETPOP








#endif /* __INTERPRET_H_ */
