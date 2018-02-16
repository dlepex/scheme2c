/* __vm_init_spec.h
 * musc- scheme translator & interpreter
 * author: denis.lepekhin@gmail.com
 * created on: 2009
 */

#ifndef __VM_INIT_SPEC_H_
#define __VM_INIT_SPEC_H_


#define OFFS_DATUM  sizeof(struct Datum)
#define OFFS_ARRAY  sizeof(struct ArrayDatum)


static void  vm_init_symbols(svm_t vm)
{
	symbol_reserve_asci( "(",   SYMB_OPEN_RO);
	symbol_reserve_asci( ")",   SYMB_CLOSE_RO);
	symbol_reserve_asci( "[",   SYMB_OPEN_SQ);
	symbol_reserve_asci( "]",   SYMB_CLOSE_SQ);
	symbol_reserve_asci( "{",   SYMB_OPEN_CURL);
	symbol_reserve_asci( "}",   SYMB_CLOSE_CURL);
	symbol_reserve_asci( "#;",   SYMB_COMMENT);
	symbol_reserve_asci( "#(",  SYMB_OPEN_VEC);
	symbol_reserve_asci( "#vu(",SYMB_OPEN_VU);
	symbol_reserve_asci( ".",   SYMB_SGN_POINT);
	symbol_reserve_asci( "'",   SYMB_SGN_QUOTE);
	symbol_reserve_asci( "`",   SYMB_SGN_QUASI);
	symbol_reserve_asci( ",",   SYMB_SGN_UNQUOTE);
	symbol_reserve_asci( ",@",  SYMB_SGN_SPLICING);
	symbol_reserve_asci( "#'",  SYMB_SGN_SYN_QUOTE);
	symbol_reserve_asci( "#`",  SYMB_SGN_SYN_QUASI);
	symbol_reserve_asci( "#,",  SYMB_SGN_SYN_UNQUOTE);
	symbol_reserve_asci( "#,@", SYMB_SGN_SYN_SPLICING);

	symbol_reserve_asci( "()", SYMB_NIL);
	symbol_reserve_asci( "#t", SYMB_T);
	symbol_reserve_asci( "#f", SYMB_F);
	symbol_reserve_asci( "#$void", SYMB_VOID);
	symbol_reserve_asci( "#$undef", SYMB_UNDEF);
	symbol_reserve_asci( "#$eof", SYMB_EOF);

	symbol_reserve_asci( "let", SYMB_KW_LET);
	symbol_reserve_asci( "let*", SYMB_KW_LET_STAR);
	symbol_reserve_asci( "letrec", SYMB_KW_LETREC);
	symbol_reserve_asci( "letrec*", SYMB_KW_LETREC_STAR);
	symbol_reserve_asci( "lambda", SYMB_KW_LAMBDA);
	symbol_reserve_asci( "define", SYMB_KW_DEFINE);
	symbol_reserve_asci( "if", SYMB_KW_IF);
	symbol_reserve_asci( "cond", SYMB_KW_COND);
	symbol_reserve_asci( "case", SYMB_KW_CASE);
	symbol_reserve_asci( "set!", SYMB_KW_SET);
	symbol_reserve_asci( "begin", SYMB_KW_BEGIN);
	symbol_reserve_asci( "else", SYMB_ELSE);
	symbol_reserve_asci( "begin0", SYMB_KW_BEGIN0);
	symbol_reserve_asci( "call/cc", SYMB_KW_CALLCC);
	//symbol_reserve_asci( "", SYMB_KW_);

	symbol_reserve_asci( "quote", SYMB_QUOTE);
	symbol_reserve_asci( "quasiquote", SYMB_QUASI);
	symbol_reserve_asci( "unquote", SYMB_UNQUOTE);
	symbol_reserve_asci( "unquote-splicing", SYMB_SPLICING);

	symbol_reserve_asci("#$vec",  SYMB_LIT_VEC);
	symbol_reserve_asci("#$vu8",   SYMB_LIT_VU);


	symbol_reserve_asci("!SET",   SYMB_VM_SET);
	symbol_reserve_asci("!DEF",   SYMB_VM_DEF);
	symbol_reserve_asci("!RET",   SYMB_VM_RET);
	symbol_reserve_asci("!CONT",   SYMB_VM_CONT);
	symbol_reserve_asci("!CALLCC",   SYMB_VM_CALLCC);
	symbol_reserve_asci("!IF",    SYMB_VM_IF);
	symbol_reserve_asci("!LMD",    SYMB_VM_LAMBDA);



	symbol_reserve_asci("!MARK",    SYMB_VM_MARK);
	symbol_reserve_asci("!JMP",    SYMB_VM_JMP);
	symbol_reserve_asci("!CALL",    SYMB_VM_CALL);


	symbol_reserve_asci("!IF_L",    SYMB_VM_IF_L);
	symbol_reserve_asci("!IF_G",    SYMB_VM_IF_G);
	symbol_reserve_asci("!IF_K",    SYMB_VM_IF_K);
	symbol_reserve_asci("!IF_R",    SYMB_VM_IF_R);
	symbol_reserve_asci("!RETCALL",    SYMB_VM_RETCALL);



	symbol_reserve_asci("!SET_L_D",   SYMB_VM_SET_L_D);
	symbol_reserve_asci("!SET_L_L",   SYMB_VM_SET_L_L);
	symbol_reserve_asci("!SET_L_G",   SYMB_VM_SET_L_G);
	symbol_reserve_asci("!SET_L_K",   SYMB_VM_SET_L_K);
	symbol_reserve_asci("!SET_L_R",   SYMB_VM_SET_L_R);

	symbol_reserve_asci("!SET_G_D",   SYMB_VM_SET_G_D);
	symbol_reserve_asci("!SET_G_L",   SYMB_VM_SET_G_L);
	symbol_reserve_asci("!SET_G_G",   SYMB_VM_SET_G_G);
	symbol_reserve_asci("!SET_G_K",   SYMB_VM_SET_G_K);
	symbol_reserve_asci("!SET_G_R",   SYMB_VM_SET_G_R);

	symbol_reserve_asci("!SET_K_D",   SYMB_VM_SET_K_D);
	symbol_reserve_asci("!SET_K_L",   SYMB_VM_SET_K_L);
	symbol_reserve_asci("!SET_K_G",   SYMB_VM_SET_K_G);
	symbol_reserve_asci("!SET_K_K",   SYMB_VM_SET_K_K);
	symbol_reserve_asci("!SET_K_R",   SYMB_VM_SET_K_R);

	symbol_reserve_asci("!RET_D",   SYMB_VM_RET_D);
	symbol_reserve_asci("!RET_L",   SYMB_VM_RET_L);
	symbol_reserve_asci("!RET_G",   SYMB_VM_RET_G);
	symbol_reserve_asci("!RET_K",   SYMB_VM_RET_K);
	symbol_reserve_asci("!RET_R",   SYMB_VM_RET_R);

	symbol_reserve_asci("!PREP_L",   SYMB_VM_PREP_L);
	symbol_reserve_asci("!PREP_G",   SYMB_VM_PREP_G);
	symbol_reserve_asci("!PREP_K",   SYMB_VM_PREP_K);
	symbol_reserve_asci("!PREP_R",   SYMB_VM_PREP_R);

	symbol_reserve_asci("!RPREP_L",   SYMB_VM_RPREP_L);
	symbol_reserve_asci("!RPREP_G",   SYMB_VM_RPREP_G);
	symbol_reserve_asci("!RPREP_K",   SYMB_VM_RPREP_K);
	symbol_reserve_asci("!RPREP_R",   SYMB_VM_RPREP_R);

	symbol_reserve_asci("!ARGPUSH_D",   SYMB_VM_ARGPUSH_D);
	symbol_reserve_asci("!ARGPUSH_L",   SYMB_VM_ARGPUSH_L);
	symbol_reserve_asci("!ARGPUSH_G",   SYMB_VM_ARGPUSH_G);
	symbol_reserve_asci("!ARGPUSH_K",   SYMB_VM_ARGPUSH_K);

	symbol_reserve_asci("!CALLCC_L",   SYMB_VM_CALLCC_L);
	symbol_reserve_asci("!CALLCC_G",   SYMB_VM_CALLCC_G);
	symbol_reserve_asci("!CALLCC_K",   SYMB_VM_CALLCC_K);
	symbol_reserve_asci("!CALLCC_R",   SYMB_VM_CALLCC_R);


	symbol_reserve_asci("!DEF_D",   SYMB_VM_DEF_D);
	symbol_reserve_asci("!DEF_G",   SYMB_VM_DEF_G);
	symbol_reserve_asci("!DEF_R",   SYMB_VM_DEF_R);
}

#define __TT_TAG  tag_from_tixcateg(TYPEIX_TYPE, TYPE_CATEGORY_CONST | TYPE_CATEGORY_NONTERM )

static struct DatumType Typetype =
	{
		.tag = tag_set_gcclass( __TT_TAG, GC_CLASS_PERM),
		.asci = "<type>",
		.tix = TYPEIX_TYPE, .category =  TYPE_CATEGORY_CONST | TYPE_CATEGORY_NONTERM,
		.szobj = sizeof(struct DatumType), .slot_offs = (ulong) &((type_scp)0)->finalizer, .slot_count=3,
		.itag = __TT_TAG
	};

static void vm_init_types(svm_t vm)
{
	descr bin_print_string(svm_t vm, uint argc,__binargv(argv));
	descr bin_print_symbol(svm_t vm, uint argc,__binargv(argv));
	descr bin_print_real(svm_t vm, uint argc,__binargv(argv));
	descr bin_print_cons(svm_t vm, uint argc,__binargv(argv));
	descr bin_print_vec(svm_t vm, uint argc,__binargv(argv) );
	descr bin_print_buf(svm_t vm, uint argc,__binargv(argv) );
	descr bin_print_default(svm_t vm, uint argc,__binargv(argv) );

	__Glob_table__[TYPEIX_TYPE] = &Typetype;


	type_reserve(vm, TYPEIX_NUM_REAL, "<real>", TYPE_CATEGORY_NUMBER | TYPE_CATEGORY_CONST,
			sizeof(struct RealDatum), 0, 0, 0);



	// array types

	type_reserve(vm, TYPEIX_VECTOR, "<vector>", TYPE_CATEGORY_NONTERM | TYPE_CATEGORY_ARRAY,
			sizeof(struct ArrayDatum), sizeof(descr), OFFSET_ARRAY, 0);

	type_reserve(vm, TYPEIX_ARRAYSTACK, "<array-stack>", TYPE_CATEGORY_NONTERM | TYPE_CATEGORY_ARRAY,
			sizeof(struct ArrayStackDatum), sizeof(descr),sizeof(struct ArrayStackDatum) , 0)->slot_count_offs =
				field_offs_scp(astk_scp, ptr);


	type_reserve(vm, TYPEIX_BUFFER, "<buffer>", TYPE_CATEGORY_ARRAY,
			sizeof(struct ArrayDatum), sizeof(byte),  0, 0);

	// array-based types

	type_reserve(vm, TYPEIX_STRING, "<string>", TYPE_CATEGORY_NONTERM,
				sizeof(struct StringDatum), 0,  field_offs_scp(string_scp, buf), 1);

	type_reserve(vm, TYPEIX_ARRAY_LIST, "<array-list>", TYPE_CATEGORY_NONTERM,
					sizeof(struct ArrayListDatum), 0,  field_offs_scp(arlist_scp, data), 1);

	type_reserve(vm, TYPEIX_HASHTABLE, "<hashtable>", TYPE_CATEGORY_NONTERM,
			sizeof(struct HashTableDatum), 0, field_offs_scp(hashtable_scp, b), _SLOTC_HTB);


	// Basic structures: pair, symbol, environment, weak box

	 type_reserve(vm, TYPEIX_CONS, "<pair>", TYPE_CATEGORY_CARCDR | TYPE_CATEGORY_NONTERM,
			sizeof(struct ConsDatum), 0, field_offs_scp(cons_scp, car), 2);

	 type_reserve(vm, TYPEIX_SYMASSOC, "<symbol>", TYPE_CATEGORY_CONST | TYPE_CATEGORY_NONTERM,
			sizeof(struct SymAssocDatum), 0, field_offs_scp(symasc_scp, name), _SLOTC_SYMBOL );


	type_reserve(vm, TYPEIX_ENVIRONMENT, "<env>", TYPE_CATEGORY_CARCDR | TYPE_CATEGORY_NONTERM,
				sizeof(struct ConsDatum), 0,  field_offs_scp(cons_scp, car), 2 );


	type_reserve(vm, TYPEIX_WEAKBOX, "<weak-box>", TYPE_CATEGORY_WEAKCONT | TYPE_CATEGORY_NONTERM,
			sizeof(struct BoxDatum), 0, field_offs_scp(box_scp, boxed), 1);
	type_reserve(vm, TYPEIX_BOX, "<box>",  TYPE_CATEGORY_NONTERM,
				sizeof(struct BoxDatum), 0, field_offs_scp(box_scp, boxed), 1);

	// Run-time structures: frame, closure, code block

	type_reserve(vm, TYPEIX_CLOSURE, "<closure>", TYPE_CATEGORY_NONTERM | TYPE_CATEGORY_ARRAY,
			sizeof(struct ClosureDatum), sizeof(descr), field_offs_scp(closure_scp, fcod ), _SLOTC_CLOSURE);
	type_reserve(vm, TYPEIX_FRAME, "<frame>", TYPE_CATEGORY_NONTERM,
			sizeof(struct FrameDatum), 0, field_offs_scp(frame_scp, _FIELD_FRAME), _SLOTC_FRAME);

	type_reserve(vm, TYPEIX_CONTINUATION, "<continuation>", TYPE_CATEGORY_NONTERM,
			sizeof(struct ContinuationDatum), 0, field_offs_scp(continuation_scp, _FIELD_CONTINUATION), _SLOTC_CONTINUATION);




	// set finalizers
	Bin_print_def = new_bin(2, bin_print_default);

	type_setfin(type_from_ix(TYPEIX_SYMASSOC), new_bin(1, bin_symbol_finalize ));

	type_setprint(type_from_ix(TYPEIX_NUM_REAL), new_bin(2, bin_print_real));
	//type_setprint(type_from_ix(TYPEIX_), new_bin(2, bin_print_));
	//type_setprint(type_from_ix(TYPEIX_), new_bin(2, bin_print_));
	//type_setprint(type_from_ix(TYPEIX_), new_bin(2, bin_print_));

	type_setprint(type_from_ix(TYPEIX_VECTOR), new_bin(2, bin_print_vec));
	type_setprint(type_from_ix(TYPEIX_BUFFER), new_bin(2, bin_print_buf));
	type_setprint(type_from_ix(TYPEIX_STRING), new_bin(2, bin_print_string));
	type_setprint(type_from_ix(TYPEIX_ARRAY_LIST), Bin_print_def);


	type_setprint(type_from_ix(TYPEIX_CONS), new_bin(2, bin_print_cons));
	type_setprint(type_from_ix(TYPEIX_SYMASSOC), new_bin(2, bin_print_symbol));


	type_setprint(type_from_ix(TYPEIX_HASHTABLE),  Bin_print_def);
	type_setprint(type_from_ix(TYPEIX_WEAKBOX),  Bin_print_def);
	type_setprint(type_from_ix(TYPEIX_BOX),  Bin_print_def);
	type_setprint(type_from_ix(TYPEIX_ENVIRONMENT),  Bin_print_def);
	type_setprint(type_from_ix(TYPEIX_FRAME),  Bin_print_def);
	type_setprint(type_from_ix(TYPEIX_CLOSURE),Bin_print_def);
	type_setprint(type_from_ix(TYPEIX_CONTINUATION),Bin_print_def);
	/*
	type_setprint(type_from_ix(TYPEIX_), new_bin(2, Bin_print_def));
	type_setprint(type_from_ix(TYPEIX_), new_bin(2, Bin_print_def));
	type_setprint(type_from_ix(TYPEIX_), new_bin(2, Bin_print_def));
	type_setprint(type_from_ix(TYPEIX_), new_bin(2, Bin_print_def));
	type_setprint(type_from_ix(TYPEIX_), new_bin(2, Bin_print_def));
*/



	// set printers
}

#endif /* __VM_INIT_SPEC_H_ */
