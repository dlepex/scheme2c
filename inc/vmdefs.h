/* vmdefs.h Structures and symbols of VIRTUAL MACHINE GC PARSER
 * musc- scheme translator & interpreter
 * author: denis.lepekhin@gmail.com
 * created on: 2009
 */

#ifndef VMDEFS_H_
#define VMDEFS_H_

#include "datum.h"

#define EXCEP_DUP_GLOBAL 601
#define EXCEP_HTB_REPEATED_PUT    602

#define EXCEP_GLOBAL_UNDEF 603

#define MAX_RESERVED_SYM 500
#define MAX_RESERVED_TYPE 100


typedef struct GcBase  *gc_t;
typedef struct SVM     *svm_t;

typedef struct FrameDatum           *frame_scp;
typedef struct ClosureDatum         *closure_scp;
typedef struct GlobalCellDatum      *globcell_scp;
typedef struct ContinuationDatum    *continuation_scp;

#define __binargv(argv)              const descr argv[]

typedef descr (*builtin_op_t) (svm_t vm, uint argc, __binargv(argv));

#define decl_bin_argv(var)  descr var[] =


#define CLOSR_ARGC_BIT_MANY 31
#define CLOSR_ARGC_MASK     _umask_(31)
#define fargc_many(argc_lowest) ((argc_lowest) | _ubit_(CLOSR_ARGC_BIT_MANY))
#define _closr_argc_get(argc)      ((argc) & CLOSR_ARGC_MASK)
#define fargs_count(argc)     ((argc) & CLOSR_ARGC_MASK)
#define fargsp_many(argc)       ((argc) & _ubit_(CLOSR_ARGC_BIT_MANY))


#define CLOSR_CAT_BIN    0
#define CLOSR_CAT_INTERP 1
#define CLOSR_CAT_COMPIL 2

struct ClosureDatum {
	tag_t tag;
	ulong count;  // count of saved frames
	ulong bits; // bits
	ulong category;
	ulong argc;   // parameters count

	builtin_op_t bin;
	/*slots*/
	descr fcod; // functional block
};

#define _SLOTC_CLOSURE 1

#define CLOSR_MULTI_ARGS _ubit_(0)

#define closrp_multiarg(c)  ((c)->bits & CLOSR_MULTI_ARGS)

#define closr_tail(c)  __datum_tail(vec_scp, c, struct ClosureDatum)


struct FrameDatum {
	tag_t tag;

	ulong ret_ip;
	void  *ret_cc;

	/*slots*/
	//frame_scp ret_fr;
	frame_scp up; /* upper frame is always older! */
	closure_scp closr;
	vec_scp     vars; // variable part of the frame
};
#define  _SLOTC_FRAME 3
#define  _FIELD_FRAME up

// frame is very similiar to continuations
struct ContinuationDatum {
	tag_t tag;
	ulong ret_ip; // capturet IP
	void  *ret_cc; // captured label addres
	frame_scp ret_fr; // ret fram
};

#define  _FIELD_CONTINUATION ret_fr
#define  _SLOTC_CONTINUATION 1

// global allocators
extern gc_t Gc_malloc;
extern gc_t Gc_perm;
extern closure_scp Bin_print_defshort;

typedef struct
{
	double allocated_percent;
	double dur;
	ulong  collected, becomeold;
	double alive;
} gc_stat_t;
typedef datum_scp (*datum_allocator) (gc_t gc, type_scp t, ulong array_size);
typedef void  (*datum_freer)(gc_t gc, datum_scp dm);
typedef bool  (*gc_belonger_t)(gc_t gc, datum_scp datum);
typedef void (*gc_getstat_t)(gc_t gc,gc_stat_t *stat);


struct GcBase {
	datum_allocator alloc; /**< allocation function */
	datum_freer     free;
	gc_belonger_t   belong;
	gc_getstat_t    getstat;
	svm_t           vm;
};

#define __pure  // function is pure - no GC is invoked.
#define __throw
// decoration for ROOT_STACK vars types
// local variable decor
#define __loc  					volatile
// local return var decor
#define __ret  					volatile
// argument wich may be invalidated after call
#define __arg             		volatile
// protected argument (Shoud Be Pushed argument!) type decor
#define __sbp(xxx_scp)       	volatile const xxx_scp*
#define _DS(rp)             (*(rp)) // dereference SBP argument
#define _DF(rp, field)      (_DS(rp)->field)
#define _RS(rp)             ( (descr*) &(rp)) // when calling SPB argument
#define _IN(rp)             _RS(rp)

#define __nongc

// declare variable and push (protect)
#define __protect(tp, var, ini)   __loc tp var =  ini; PUSH_RSTK(var)
#define __retdecl(tp, var, ini)   __protect(tp, var, ini)


#define new_array(gc, t, size)    ((gc)->alloc((gc), (t), (size)))
#define new_inst(gc, t)           new_array(gc, t, 0)

#define inew_array(gc, tix, size) new_array(gc, type_from_ix(tix), size)
#define inew_inst(gc, tix) 		 inew_array(gc, tix, 0)


#define del_inst(gc, datum) ((gc)->free((gc), (datum_scp) (datum)))
#define gc_belong(gc, datum)  ((gc)->belong((gc), (datum_scp)(datum)))
#define gc_getstat(gc, pstat)  ((gc)->getstat((gc), pstat) )

#define gc_getvm(gc)			(( (gc) &&  (gc)->vm) ? ((gc)->vm) : local_vm() )
#define decl_vm_by_gc(gc)   svm_t vm = gc_getvm(gc)
#define decl_vm()             svm_t vm = local_vm()

#define decl_vmgc()         decl_vm(); decl_gc_of_vm()
#define decl_gc_of_vm()             gc_t gc = vm->gc

//inline void __gc_slot_set(svm_t vm, datum_scp l,  descr slot, descr r);
inline void __gc_check_old(svm_t vm, datum_scp l);
void gc_collect(svm_t vm);


// some mem bits (publ use)
#define GC_BIT_OLD    27
#define GC_BIT_GSCAN 30
#define tag_is_old(t)				((t) & _lbit_(GC_BIT_OLD))
#define tag_is_gscan(t)          ((t) & _lbit_(GC_BIT_GSCAN))

// from memmgr.h#define GC_BYTE_SHIFT   24
#define tag_set_gscan(t)         	((t) | _lbit_(GC_BIT_GSCAN))
#define datum_set_globscan(dm)    	datum_set_tag(dm, tag_set_gscan(datum_get_tag(dm)))
#define datum_is_globscan(dm)       tag_is_gscan(datum_get_tag(dm))

#define GC_BYTE_SHIFT   24
#define GC_CLASS_MASK	(_lmask_(3) << GC_BYTE_SHIFT )
#define tag_is_generalcls(tag)      !((tag) & GC_CLASS_MASK)
#define datum_is_generalcls(dm)      tag_is_generalcls(datum_get_tag(dm))
#define datum_is_old(d)    tag_is_old(datum_get_tag(d))
#define datum_is_young(dm)   (datum_is_generalcls(dm) && !datum_is_old(dm))


#define GC_CHECK_OLD(l) if(datum_is_old(l) ){__gc_check_old(vm, (datum_scp)(l));}
// those macrosi dmnd vm local var. l r are __arg r- must be __pure
#define SLOT_SET(l, elem, r)  \
{ \
	(elem) =  (r); \
	if(datum_is_old(l) && (scp_valid(r) && datum_is_young(r))) { \
		__gc_check_old(vm, (datum_scp)(l)); \
	} }






#define FIELD_SET(l, field, r)  SLOT_SET(l, (l)->field, r)

#define VEC_SET(l, index, r);   SLOT_SET(l, vec_tail(l)[index] , r)
#define ASTK_SET(l, index, r);   SLOT_SET(l, astk_tail(l)[index] , r)


#define VEC_DSET(l, index, r);   SLOT_SET(_DS(l), vec_tail(_DS(l))[index] , r)
#define VEC_DSETD(l, index, r);   SLOT_SET(_DS(l), vec_tail(_DS(l))[index] , _DS(r))
#define FIELD_DSET(dl, field, r) SLOT_SET(_DS(dl), _DF(dl, field), r )
#define FIELD_DSETD(dl, field, dr) SLOT_SET(_DS(dl), _DF(dl, field), _DS(dr))

void gc_minor(svm_t vm);

void gc_major(svm_t vm);







struct SVM {
	/*additional Roots stack (or R-stack)*/
	volatile descr **rstk_begin, **rstk_end, **rstk_ptr;
	frame_scp topframe;
	descr *binargs;
	gc_t gc;
};

inline svm_t local_vm();

#define local_gc()       (local_vm()->gc)
#define vm_gc(vm)     ((vm)->gc)



#define R_PUSH(vm, d)  (* ((vm)->rstk_ptr++)) = ((descr*)&(d))
#define R_POP(vm)			((--(vm)->rstk_ptr))




#define PUSH_RSTK(var)       R_PUSH(vm, var)
#define PUSHA_RST(arr, from, lim) for(ulong i = from; i < lim; i++){ PUSH_RSTK(arr[i]); }
#define PUSH_RSTK2(v1,v2)    PUSH_RSTK(v1); PUSH_RSTK(v2)
#define PUSH_RSTK3(v1,v2,v3) PUSH_RSTK2(v1,v2);  PUSH_RSTK(v3)
#define PUSH_RSTK4(v1,v2,v3,v4) PUSH_RSTK2(v1,v2); PUSH_RSTK2(v3,v4);

#define POP_RSTK()		     R_POP(vm)


#define SAVE_RSTK()      	 VAR_SAVE_RSTK(__saved_rstk)
#define UNWIND_RSTK()       VAR_UNWIND_RSTK(__saved_rstk)

#define	VAR_SAVE_RSTK(x)      volatile descr** x = (vm)->rstk_ptr
#define 	VAR_UNWIND_RSTK(x)      (vm)->rstk_ptr = x


#define vm_topframe(vm)  ((vm)->topframe)
inline void vm_get_globals(svm_t vm, descr **from, descr **to );

#define vm_call(vm, cls, argc, argv) \
	((((closure_scp)(cls))->bin) ? ((closure_scp)(cls))->bin(vm, argc, argv) : 0)



#define datum_finalize(vm, dm) \
{ \
		descr fin = datum_get_type(dm)->finalizer; \
		decl_bin_argv(argv) {dm};\
		if(fin) { vm_call(vm, fin, 1, argv);} \
}

/* some SPECIAL symbols used in parsing */

#define SYMB_OPEN_RO   	desc_from_spec(20) // (
#define SYMB_CLOSE_RO 	desc_from_spec(21) // )
#define SYMB_OPEN_SQ   	desc_from_spec(22) // [
#define SYMB_CLOSE_SQ 	desc_from_spec(23) // ]
#define SYMB_OPEN_VEC     desc_from_spec(24) // #(
#define SYMB_OPEN_VU      desc_from_spec(25) // #vu(
#define SYMB_OPEN_CURL     desc_from_spec(26) // {
#define SYMB_CLOSE_CURL     desc_from_spec(27) // }
#define SYMB_COMMENT      	  desc_from_spec(28)   // #;

#define SYMB_SGN_POINT      	  desc_from_spec(106)   // .
#define SYMB_SGN_QUOTE      	  desc_from_spec(107)   // '
#define SYMB_SGN_QUASI      	  desc_from_spec(108)   // `
#define SYMB_SGN_UNQUOTE          desc_from_spec(109)   // ,
#define SYMB_SGN_SPLICING         desc_from_spec(110)  // ,@
#define SYMB_SGN_SYN_QUOTE        desc_from_spec(111)  // #'
#define SYMB_SGN_SYN_QUASI        desc_from_spec(112)  // #`
#define SYMB_SGN_SYN_UNQUOTE      desc_from_spec(113)  // #,
#define SYMB_SGN_SYN_SPLICING     desc_from_spec(114)  // #,@


#define SYMB_KW_LET 				   desc_from_spec(30) // let
#define SYMB_KW_LET_STAR 			desc_from_spec(31) // let*
#define SYMB_KW_LETREC 			   desc_from_spec(32) // letrec
#define SYMB_KW_LETREC_STAR 		desc_from_spec(33) // letrec*
#define SYMB_KW_LAMBDA 		      desc_from_spec(34) // lambda
#define SYMB_KW_DEFINE		 		desc_from_spec(35) // define
#define SYMB_KW_IF			 		desc_from_spec(36) // if
#define SYMB_KW_COND  		 		desc_from_spec(37) // cond
#define SYMB_KW_CASE  		 		desc_from_spec(38) // case
#define SYMB_KW_SET 		 		   desc_from_spec(39) // set!
#define SYMB_KW_LAMBDAGREEK 		desc_from_spec(40) // λ
#define SYMB_KW_BEGIN 		      desc_from_spec(41) // begin
#define SYMB_KW_BEGIN0 		      desc_from_spec(42) // begin0
#define SYMB_KW_CALLCC 		      desc_from_spec(43) // call/cc

#define SYMB_QUOTE              	desc_from_spec(60) // quote
#define SYMB_QUASI              	desc_from_spec(61) // quasiquote
#define SYMB_UNQUOTE             desc_from_spec(62) // unquote
#define SYMB_SPLICING            desc_from_spec(63) // unquote


#define SYMB_LIT_VEC             desc_from_spec(90) // #%vec
#define SYMB_LIT_VU              desc_from_spec(91) // #%vu

#define SYMB_ELSE                desc_from_spec(92) // else

// META-INSTRUCTIONS
#define SYMB_VM_SET					desc_from_spec(301)
#define SYMB_VM_IF               desc_from_spec(302)
#define SYMB_VM_RET 					desc_from_spec(303)
#define SYMB_VM_LAMBDA           desc_from_spec(439)
//#define SYMB_VM_CALL             desc_from_spec(445) see forward
#define SYMB_VM_DEF              desc_from_spec(306)
#define SYMB_VM_CONT             desc_from_spec(307)
#define SYMB_VM_CALLCC           desc_from_spec(308)




// not ONLY META
#define SYMB_VM_MARK             desc_from_spec(400)
#define SYMB_VM_JMP              desc_from_spec(401)

// Prepare family (prepare closure)

#define SYMB_VM_PREP_L            desc_from_spec(405)
#define SYMB_VM_PREP_G            desc_from_spec(406)
#define SYMB_VM_PREP_K            desc_from_spec(407)
#define SYMB_VM_PREP_R            desc_from_spec(408)

#define SYMB_VM_RPREP_L            desc_from_spec(409)
#define SYMB_VM_RPREP_G            desc_from_spec(410)
#define SYMB_VM_RPREP_K            desc_from_spec(411)
#define SYMB_VM_RPREP_R            desc_from_spec(412)

// IF family

#define SYMB_VM_IF_L            desc_from_spec(415)
#define SYMB_VM_IF_G            desc_from_spec(416)
#define SYMB_VM_IF_K            desc_from_spec(417)
#define SYMB_VM_IF_R            desc_from_spec(418)

// SET family

#define SYMB_VM_SET_L_D               desc_from_spec(420)
#define SYMB_VM_SET_L_L               desc_from_spec(421)
#define SYMB_VM_SET_L_G               desc_from_spec(422)
#define SYMB_VM_SET_L_K               desc_from_spec(423)
#define SYMB_VM_SET_L_R               desc_from_spec(424)

#define SYMB_VM_SET_K_D               desc_from_spec(425)
#define SYMB_VM_SET_K_L               desc_from_spec(426)
#define SYMB_VM_SET_K_G               desc_from_spec(427)
#define SYMB_VM_SET_K_K               desc_from_spec(428)
#define SYMB_VM_SET_K_R               desc_from_spec(429)

#define SYMB_VM_SET_G_D               desc_from_spec(430)
#define SYMB_VM_SET_G_L               desc_from_spec(431)
#define SYMB_VM_SET_G_G               desc_from_spec(432)
#define SYMB_VM_SET_G_K               desc_from_spec(433)
#define SYMB_VM_SET_G_R               desc_from_spec(434)

// DEF family

#define SYMB_VM_DEF_D               desc_from_spec(435)
#define SYMB_VM_DEF_G               desc_from_spec(436)
#define SYMB_VM_DEF_R               desc_from_spec(437)



// RET family

#define SYMB_VM_RET_D               desc_from_spec(440)
#define SYMB_VM_RET_L               desc_from_spec(441)
#define SYMB_VM_RET_G               desc_from_spec(442)
#define SYMB_VM_RET_K               desc_from_spec(443)
#define SYMB_VM_RET_R					desc_from_spec(444)


// PROCEDURAL

#define SYMB_VM_CALL                 desc_from_spec(445) // CLMD <fcode>
#define SYMB_VM_RETCALL					  desc_from_spec(446) //see lambda

#define SYMB_VM_ARGPUSH_D				  desc_from_spec(447)
#define SYMB_VM_ARGPUSH_L				  desc_from_spec(448)
#define SYMB_VM_ARGPUSH_G				  desc_from_spec(449)
#define SYMB_VM_ARGPUSH_K				  desc_from_spec(450)

// CONTINUATIONS
#define SYMB_VM_CALLCC_L               desc_from_spec(451)
#define SYMB_VM_CALLCC_G               desc_from_spec(452)
#define SYMB_VM_CALLCC_K               desc_from_spec(453)
#define SYMB_VM_CALLCC_R					desc_from_spec(454)


#define SYMB_TYPEIX_NUM_REAL 			   desc_from_spec(200)
#define SYMB_TYPEIX_NUM_COMPLEX			desc_from_spec(201)
#define SYMB_TYPEIX_NUM_RATIONAL		   desc_from_spec(202)
#define SYMB_TYPEIX_NUM_BIGNUM       	desc_from_spec(203)
#define SYMB_TYPEIX_VECTOR					desc_from_spec(210)
#define SYMB_TYPEIX_BUFFER					desc_from_spec(211)
#define SYMB_TYPEIX_ARRAY_LIST			desc_from_spec(212)
#define SYMB_TYPEIX_CONS					desc_from_spec(213)
#define SYMB_TYPEIX_SYNTAX_CONS 			desc_from_spec(214)
#define SYMB_TYPEIX_SYNTAX_IDENT 		desc_from_spec(215)
#define SYMB_TYPEIX_TEXTPOS           	desc_from_spec(216)
#define SYMB_TYPEIX_STRING           	desc_from_spec(217)
#define SYMB_TYPEIX_HASHTABLE        	desc_from_spec(218)
#define SYMB_TYPEIX_HASHNODE         	desc_from_spec(219)
#define SYMB_TYPEIX_TYPE	            desc_from_spec(220)
#define SYMB_TYPEIX_SYMASSOC          	desc_from_spec(221)
#define SYMB_TYPEIX_OPORT            	desc_from_spec(222)
#define SYMB_TYPEIX_IPORT             	desc_from_spec(223)
#define SYMB_TYPEIX_PARSER            	desc_from_spec(224)
#define SYMB_TYPEIX_FRAME             	desc_from_spec(225)
#define SYMB_TYPEIX_CODEBLOCK         	desc_from_spec(226)
#define SYMB_TYPEIX_CLOSURE          	desc_from_spec(227)
#define SYMB_TYPEIX_WEAKBOX           	desc_from_spec(228)






#endif /* VMDEFS_H_ */
