/* transform.c
 * - scheme translator & interpreter
 * author: denis.lepekhin@gmail.com
 * created on: 2009
 */

#include "transform.h"
#include "loggers.h"
#include "sysexcep.h"

#define SYN_CONTEXT_QUOTE 		  1 // quoted context (or quasi)
#define SYN_CONTEXT_TOPLEVEL    2 // namespace context;
#define SYN_CONTEXT_EXPR        3 // expression context;
bool kw_check_form(svm_t vm, __sbp(transformer_scp) ptr, __sbp(descr) cdat, int cx);
fcode_scp new_fcode(svm_t vm, __sbp(transformer_scp) ptr, __arg fcode_scp parent, __arg env_scp defenv,
		__arg cl_scp args);

descr anf_match_form(svm_t vm, __sbp(transformer_scp) ptr, __sbp(fcode_scp) pfc, __sbp(env_scp) penv,
		__arg descr rid, __arg descr frm);
descr anf_rid(svm_t vm,  __sbp(transformer_scp) ptr, __sbp(fcode_scp) pfc,
		__arg descr rid, __arg descr expr);
descr bin_print_fcode(svm_t vm, uint argc, __binargv(argv));

void transformer_init(svm_t vm) {
	global_once_preamble();
	parser_init(vm);
	type_scp t = NIHIL;
	type_reserve(vm, TYPEIX_TRANSFORMER, "<transformer>", TYPE_CATEGORY_NONTERM ,
	sizeof(struct TransformerDatum), 0, field_offs_scp(transformer_scp, env), _SLOTC_TRANSFORMER);
	type_setprint(type_from_ix(TYPEIX_TRANSFORMER),  Bin_print_defshort);

	t = type_reserve(vm, TYPEIX_CODEBLOCK, "<fcode>", TYPE_CATEGORY_NONTERM ,
	sizeof(struct CodeDatum), 0, field_offs_scp(fcode_scp, defenv), _SLOTC_CODEBLOCK);

	type_setprint(t, new_bin(2, bin_print_fcode));

	log_info(Sys_log, "transformer module init ok");
	//type_setprint(type_from_ix(TYPEIX_CODEBLOCK),  Bin_print_defshort);
}

descr bin_print_fcode(svm_t vm, uint argc, __binargv(argv))
{
	oport_scp op =  (oport_scp) argv[0];
	fcode_scp dm = (fcode_scp) argv[1];

	oport_write_asci(op, "(%<fcode> uniqnum:");
	oport_print_int(op, dm->uniq);
	oport_write_asci(op, ")");
	return SYMB_VOID;
}

transformer_scp new_transformer(svm_t vm) {
	decl_gc_of_vm();
	SAVE_RSTK();
	__protect(arlist_scp,arl, new_arlist(vm, gc, 256));

	__protect(transformer_scp, r, (transformer_scp) inew_inst(gc, TYPEIX_TRANSFORMER));
	r->bits = 0;
	r->env = env_global();
	r->arl = arl;

	UNWIND_RSTK();
	return r;
}



#define kw_clear_idstack(tr) arlist_clear((tr)->arl)
#define trf_clear_idstack(tr) arlist_clear((tr)->arl)
bool kw_push_id(svm_t vm, __sbp(transformer_scp) ptr, symbol_d id) {
	decl_gc_of_vm();
	arlist_scp al = _DF(ptr, arl);
	for (int i = 0; i < al->index; i++) {
		if (id == arlist_tail(al)[i]) {
			return false;
		}
	}
	arlist_push(vm, gc, al, id);
	return true;
}

ulong anf_get_mark()
{
	static ulong mark = 0;
	return mark++;
}

void kw_error_asci(svm_t vm, __sbp(transformer_scp) ptr, __arg cons_scp node, const char* asci) {
	_DF(ptr,bits) |= TRANSF_BITM_ERR_KWSYN;
	txtpos_scp tpos = syn_getpos((datum_scp)node);
	if (tpos) {
		u_fprintf(ustderr, "Error[FORM]  %i:%i-%i:%i %s : ", tpos->row_begin,
				tpos->col_begin, tpos->row_end, tpos->col_end, asci);
	} else {
		u_fprintf(ustderr, "Error[FORM] %s : ", asci);
	}
	u_fflush(ustderr);
	SAVE_RSTK();
	PUSH_RSTK(node);

	oport_begin_limit(Err_console_port, 50);
	oport_print_datum(vm, Err_console_port, node);
	oport_end_limit(Err_console_port);
	UNWIND_RSTK();
	fprintf(stderr, "\n");
}

bool kw_error_ifnotid(svm_t vm, __sbp(transformer_scp) ptr, __arg cons_scp mbid, const char* header) {
	char buf[128];
	if (!datum_is_id(mbid)) {
		strcat(strcpy(buf, header), " - identifier expected");
		kw_error_asci(vm, ptr, mbid, buf);
		return false;
	}
	if (symbol_reserved(id_symbol(mbid))) {
		strcat(strcpy(buf, header), " - keyword as identifier");
		kw_error_asci(vm, ptr, mbid, buf);
		return false;
	}
	return true;
}

void trf_check_syntax(svm_t vm, __sbp(transformer_scp) ptr, __sbp( descr) cdat) {
	_try_
		_DF(ptr, bits) &= ~TRANSF_BITM_ERR_KWSYN;
		kw_check_form(vm, ptr, cdat, SYN_CONTEXT_TOPLEVEL);
	_except_
	_catch_(EXCEP_TRANSFORM_FATAL)
	_chain_try_
}

bool kw_match_tail_cx(svm_t vm, __sbp(transformer_scp) ptr, __arg descr p, int cx) {
	assert(datum_is_carcdr(p) || cl_end(p));
	bool _r = true;
	SAVE_RSTK();
	PUSH_RSTK(p);
	__protect(descr,q, NIHIL);
	until(cl_end(p)){
		q = CAR(p);
		if(!kw_check_form(vm, ptr, _RS(q), cx)) {
			_r = false;
		}
		p = CDR(p);
	}
	UNWIND_RSTK();
	return _r;
}

bool kw_match_tail(svm_t vm, __sbp(transformer_scp) ptr, __arg descr p) {
	return kw_match_tail_cx(vm, ptr, p, SYN_CONTEXT_EXPR);
}

bool kw_check_expr(svm_t vm, __sbp(transformer_scp) ptr, __sbp(cons_scp) cdat) {
	SAVE_RSTK();
	__protect(cons_scp,headcell, (cons_scp) _DS(cdat));
	bool result = true;

	if (!clp_proper(headcell)) {
		kw_error_asci(vm, ptr, headcell,
				"wrong application syntax - improper list");
		result = false;
		goto ret;
	}
	__protect(descr,head, CAR(headcell));
	if (datum_is_carcdr(head)) {
		kw_check_form(vm, ptr, _RS(head), SYN_CONTEXT_EXPR);
	} else if (!datum_is_id(head)) {
		kw_error_asci(vm, ptr, headcell,
				"wrong application syntax - bad list head");
		result = false;
		goto ret;
	}
	__protect(descr,p, CDR(headcell));
	result = kw_match_tail(vm, ptr, p);
	ret: UNWIND_RSTK();
	return result;
}

bool kw_check_quote(svm_t vm, __sbp(transformer_scp) ptr, __sbp(cons_scp) cdat) {
	__loc cons_scp tail = CDR(_DS(cdat));
	if (cli_len(tail) != 1) {
		kw_error_asci(vm, ptr, _DS(cdat), "quote must have 1 argument");
		return false;
	}
	return true;
}

bool kw_check_quasi(svm_t vm, __sbp(transformer_scp) ptr, __sbp(cons_scp) cdat) {
	bool result = true;
	SAVE_RSTK();
	__loc cons_scp tail = CDR(_DS(cdat));

	if (cli_len(tail) != 1) {
		kw_error_asci(vm, ptr, _DS(cdat), "quasiquote must have 1 argument");
		result = false;
		goto ret;
	}
	tail = CAR(tail);
	bool simple_quote = true;
	if (datum_is_carcdr(tail)) {
		__protect(descr,p, tail);
		__protect(descr,head, NIHIL);
		do {
			head = CAR(p);
			if (datum_is_id(head)) {
				symbol_d s = id_symbol(head);
				if (s == SYMB_UNQUOTE || s == SYMB_SPLICING) {
					kw_error_asci(
							vm,
							ptr,
							_DS( cdat), "unquote(splicing) used as identifier inside quasiquotes");
					result = false;
					goto ret;
				}
			} else if (datum_is_carcdr(head)) {
				descr ms = CAR(head);
				if (datum_is_id(ms)) {
					ms = id_symbol(ms);
					if (ms == SYMB_UNQUOTE || ms == SYMB_SPLICING) {
						simple_quote = false;
						if (!kw_check_form(vm, ptr, _RS(head), SYN_CONTEXT_QUOTE)) {
							result = false;
						}
					}
				}
			}
			p = CDR(p);
		}until(!datum_is_carcdr(p));
	}

	if (simple_quote) {
		decl_gc_of_vm();
		__loc descr id = new_id(vm, gc, SYMB_QUOTE);
		FIELD_DSET(cdat, car, id );
	}
	ret: UNWIND_RSTK();
	return result;
}

bool kw_check_unquote(svm_t vm, __sbp(transformer_scp) ptr, __sbp(cons_scp) cdat, int cx) {
	__loc cons_scp tail = CDR(_DS(cdat));

	if (cx != SYN_CONTEXT_QUOTE) {
		kw_error_asci(vm, ptr, _DS(cdat), "unquote(splicing) not in context of quasiquotes");
		return false;
	}

	if (cli_len(tail) != 1) {
		kw_error_asci(vm, ptr, _DS(cdat), "unquote(splicing) must have 1 argument");
		return false;
	}
	return true;
}

bool kw_check_let(svm_t vm, __sbp(transformer_scp) ptr, __sbp(cons_scp) cdat) {
	bool result = true;
	SAVE_RSTK();

	__protect(cons_scp,tail, CDR(_DS(cdat)));
	if (cli_len(tail) < 2) {
		kw_error_asci(vm, ptr, _DS(cdat), "illegal let syntax");
		result = false;
		goto ret;
	}
	__protect( cons_scp, p, CAR(tail));
	if (!clp_proper(p)) {
		kw_error_asci(vm, ptr, p, "illegal let binding list");
		result = false;
		goto ret;
	}
	__protect(descr,q, NIHIL);
	kw_clear_idstack(_DS(ptr));
	until(cl_end(p)){
		q = CAR(p);
		if(cli_len(q) != 2) {
			kw_error_asci(vm, ptr, q, "illegal binding in let form" );
			result = false;
			goto ret;
		}

		if(!kw_error_ifnotid(vm, ptr, CAR(q), "illegal let")) {
			result = false;
			goto ret;
		}

		if(!kw_push_id(vm, ptr, id_symbol(CAR(q)))) {
			kw_error_asci(vm, ptr, CAR(q), "bad let - repeated identifier" );
			result = false;

		}
		p = CDR(p);
	}
	if(!result) {
		goto ret;
	}
	p = CAR(tail);
	until(cl_end(p)) {
		q = CAR(CDR(CAR(p)));
		if(!kw_check_form(vm, ptr, _RS(q), SYN_CONTEXT_EXPR )) {
			result = false;
		}
		p = CDR(p);
	}
	bool _r = kw_match_tail(vm, ptr, tail);
	if(result) {
		result = _r;
	}
	ret:
	UNWIND_RSTK();
	return result;
}

bool kw_check_let_star(svm_t vm, __sbp(transformer_scp) ptr, __sbp(cons_scp) cdat) {
	bool result = true;
	decl_gc_of_vm();
	SAVE_RSTK();

	__protect(cons_scp,tail, CDR(_DS(cdat)));
	if (cli_len(tail) < 2) {
		kw_error_asci(vm, ptr, _DS(cdat), "illegal let* syntax");
		result = false;
		goto ret;
	}
	__protect( cons_scp, p, CAR(tail));
	if (!clp_proper(p)) {
		kw_error_asci(vm, ptr, p, "illegal let* binding list");
		result = false;
		goto ret;
	}

	__protect(descr,q, NIHIL);
	kw_clear_idstack(_DS(ptr));

	__protect(descr, idlet, new_id(vm, gc, SYMB_KW_LET));
	__protect(descr, q1, NIHIL);

	until(cl_end(p)){
		q = CAR(p);
		if(cli_len(q) != 2) {
			kw_error_asci(vm, ptr, q, "illegal binding in let* form" );
			result = false;
			goto ret;
		}

		if(!kw_error_ifnotid(vm, ptr, CAR(q), "illegal let* ")) {
			result = false;
			goto ret;
		}
		p = CDR(p);
	}
	if(!result) {
		goto ret;
	}
	p = CAR(tail);
	__protect(cons_scp, prev,  CONSCAR(SYMB_NIL));
	__protect(cons_scp, r, prev);
	prev = r;

	until(cl_end(p)) {
		q = CAR(CDR(CAR(p)));

		if(!kw_check_form(vm, ptr, _RS(q), SYN_CONTEXT_EXPR )) {
			result = false;
			goto ret;
		}
		q1 = CONSCAR(CONSCAR(CAR(p)));
		q = CONSCAR(CONS(idlet, q1));
		FIELD_SET(prev, cdr,q );
		prev = q1;
		p = CDR(p);
	}

	bool _r = kw_match_tail(vm, ptr, tail);

	if(result) {
		result = _r;

		tail=  _DS(cdat);
		FIELD_SET(tail, car, idlet);
		if(r->cdr != NIHIL) {
			FIELD_SET(prev, cdr, CDR(CDR(tail)));
			FIELD_SET(tail, cdr, CDR(CAR(CDR(r))));
		}
	}
	ret:
	UNWIND_RSTK();
	return result;
}

bool kw_check_proto(svm_t vm, __sbp(transformer_scp) ptr, __sbp(descr) cdat) {
	bool result = true;
	SAVE_RSTK();
	__protect(cons_scp,tail, CDR(_DS(cdat)));
	__protect(cons_scp,p, NIHIL);


	UNWIND_RSTK();
	return result;
}

bool kw_check_lambda(svm_t vm, __sbp(transformer_scp) ptr, __sbp(descr) cdat) {
	bool result = true;
	SAVE_RSTK();
	__protect(cons_scp,tail, CDR(_DS(cdat)));
	__protect(cons_scp,p, NIHIL);

	if (cli_len(tail) < 2) {
		kw_error_asci(vm, ptr, _DS(cdat), "illegal lambda syntax");
		result = false;
		goto ret;
	}

	p = CAR(tail);
	if (!datum_is_cl(p)) {
		kw_error_asci(vm, ptr, _DS(cdat), "illegal lambda");
		result = false;
		goto ret;
	}
	__protect(cons_scp,q, NIHIL);

	kw_clear_idstack(_DS(ptr));

	if(!cl_end(p)) {
		do {
			q = CAR(p);
			if (!kw_error_ifnotid(vm, ptr, q, "bad lambda")) {
				result = false;
			}

			if (!kw_push_id(vm, ptr, id_symbol(q))) {
				kw_error_asci(vm, ptr, q, "bad lambda - repeated arg");
				result = false;
			}

			p = CDR(p);
		} while (datum_is_carcdr(p));

		if (!cl_end(p) && !kw_error_ifnotid(vm, ptr, p, "bad lambda")) {
			result = false;
		}

		if (!cl_end(p) && !kw_push_id(vm, ptr, id_symbol(p))) {
			kw_error_asci(vm, ptr, q, "bad lambda - repeated argument");
			result = false;
		}
	}

	bool _r = kw_match_tail(vm, ptr, CDR(tail));
	if (result)
		result = _r;

	ret: UNWIND_RSTK();
	return result;
}

bool kw_check_if(svm_t vm, __sbp(transformer_scp) ptr, __sbp(descr) cdat) {
	bool result = true;
	SAVE_RSTK();
	__protect(cons_scp,tail, CDR(_DS(cdat)));
	ulong k = cli_len(tail);
	if (k != 2 && k != 3) {
		kw_error_asci(vm, ptr, _DS(cdat), "bad if - must have 2 or 3 subexpr");
		result = false;
		goto ret;
	}
	bool _r = kw_match_tail(vm, ptr, tail);
	if (result)
		result = _r;
	ret: UNWIND_RSTK();
	return result;
}

bool kw_check_begin(svm_t vm, __sbp(transformer_scp) ptr, __sbp(descr) cdat) {
	bool result = true;
	SAVE_RSTK();
	__protect(cons_scp,tail, CDR(_DS(cdat)));
	ulong k = cli_len(tail);
	if (k < 1) {
		kw_error_asci(vm, ptr, _DS(cdat), "bad begin - empty form or improper list");
		result = false;
		goto ret;
	}
	bool _r = kw_match_tail(vm, ptr, tail);
	if (result)
		result = _r;
	ret: UNWIND_RSTK();
	return result;
}

bool kw_check_cond(svm_t vm, __sbp(transformer_scp) ptr, __sbp(descr) cdat) {
	bool result = true;
	decl_gc_of_vm();
	SAVE_RSTK();
	__protect(cons_scp,tail, CDR(_DS(cdat)));
	__protect(cons_scp,p, tail);
	__protect(cons_scp,q, NIHIL);
	__protect(descr,q1, NIHIL);
	__protect(descr,q2, NIHIL);
	__protect(descr,q3, NIHIL);
	__protect(descr,q4, NIHIL);
	__protect(cons_scp,r, NIHIL);
	__protect(cons_scp,z, NIHIL);
	__protect(cons_scp,prev, NIHIL);

	bool felse = false;

	if (cli_len(tail) < 1) {
		kw_error_asci(vm, ptr, _DS(cdat), "bad cond");
		result = false;
		goto ret;
	}

	until(cl_end(p)){
	q = CAR(p);
	if(!datum_is_carcdr(q)) {
		kw_error_asci(vm, ptr, _DS(cdat), "bad cond - test-list expected" );
		result = false; goto ret;
	}
	if(cli_len(q) < 2) {
		kw_error_asci(vm, ptr, _DS(cdat), "bad cond - proper test list expected" );
		result = false; goto ret;
	}

	if(felse) {
		kw_error_asci(vm, ptr, _DS(cdat), "bad cond - else clause not last" );
		result = false; goto ret;
	}
	if(!datum_is_id(CAR(q)) || id_symbol(CAR(q)) != SYMB_ELSE) {
		if(!kw_match_tail(vm, ptr, q)) {
			result = false; goto ret;
		}
		q1 = CAR(q);
		q2 = CDR(q);
		q3 = new_id(vm, gc, SYMB_KW_IF);
		q4 = new_id(vm, gc, SYMB_KW_BEGIN);
		z = CONS(q3, CONS(q1, CONSCAR(CONS(q4,q2))));
		if(prev == NULL) {
			r = z;
			prev = z;
		} else {
			prev = CDR(CDR(prev));
			FIELD_SET(prev, cdr, CONSCAR(z));
			prev = z;
		}
	} else {
		if(!kw_match_tail(vm, ptr, CDR(q))) {
			result = false; goto ret;
		}

		felse = true;
		q4 = new_id(vm, gc, SYMB_KW_BEGIN);
		z = new_cons(vm, gc, q4,CDR(q));
		if(prev == NULL) {
			r = z;
			prev = z;
		} else {
			prev = CDR(CDR(prev));
			FIELD_SET(prev, cdr, new_cons(vm, gc, z, NIHIL));
			prev = z;
		}
	}
	p = CDR(p);
}

	tail = _DS(cdat);
	assert(r != NULL);
	if(id_symbol(CAR(r)) == SYMB_KW_IF) {
		q1 = new_id(vm, gc, SYMB_KW_IF);
		FIELD_SET(tail, car, q1);
		FIELD_SET(tail, cdr, CDR(r));
	} else {
		q1 = new_id(vm, gc, SYMB_KW_BEGIN);
		FIELD_SET(tail, car, q1);
		FIELD_SET(tail, cdr, CDR(r));
	}

ret:
	UNWIND_RSTK();
	return result;

}

bool kw_check_set(svm_t vm, __sbp(transformer_scp) ptr, __sbp(descr) cdat) {
	SAVE_RSTK();
	bool result = true;
	__protect(cons_scp,tail, CDR(_DS(cdat)));
	ulong k = cli_len(tail);
	if (k != 2) {
		kw_error_asci(vm, ptr, _DS(cdat), "bad set! syntax - must have 2 parts");
		result = false;
		goto ret;
	}
	if (!kw_error_ifnotid(vm, ptr, CAR(tail), "bad set!")) {
		result = false;
	}

	tail = CDR(tail);
	if (!kw_match_tail(vm, ptr, tail)) {
		result = false;
	}
ret: UNWIND_RSTK();
	return result;
}

bool kw_check_define(svm_t vm, __sbp(transformer_scp) ptr, __sbp(descr) cdat, int cx) {
	bool result = true;
	SAVE_RSTK();
	decl_gc_of_vm();
	__protect(cons_scp,tail, CDR(_DS(cdat)));
	__protect(cons_scp,p, NIHIL);
	__protect(cons_scp,q, NIHIL);
	__protect(cons_scp,q1, NIHIL);

	if (cx != SYN_CONTEXT_TOPLEVEL) {
		kw_error_asci(vm, ptr, _DS(cdat), "define is not in top-level context");
		result = false;
		goto ret;
	}

	if (cli_len(tail) < 2) {
		kw_error_asci(vm, ptr, _DS(cdat), "illegal define syntax");
		result = false;
		goto ret;
	}

	p = CAR(tail);

	if (datum_is_id(p)) {
		if (symbol_reserved(id_symbol(p))) {
			kw_error_asci(vm, ptr, p, "bad define - keyword as identifier");
			result = false; goto ret;
		}
		p = CAR(tail);
		bool _r = kw_match_tail(vm, ptr, tail);
		if (result)
			result = _r;
	} else if (datum_is_carcdr(p)) {

		kw_clear_idstack(_DS(ptr));
		bool other = false;
		do {
			q = CAR(p);
			if (!kw_error_ifnotid(vm, ptr, q, "bad define")) {
				result = false;
			}

			if (other && !kw_push_id(vm, ptr, id_symbol(q))) {
				kw_error_asci(vm, ptr, q, "bad define - repeated argument");
				result = false;
			}
			other = true;
			p = CDR(p);
		} while (datum_is_carcdr(p));

		if (!cl_end(p) && !kw_error_ifnotid(vm, ptr, p, "bad define")) {
			result = false;
		}

		if (!cl_end(p) && !kw_push_id(vm, ptr, id_symbol(p))) {
			kw_error_asci(vm, ptr, q, "bad define - repeated argument");
			result = false;
		}

		bool _r = kw_match_tail(vm, ptr, CDR(tail));
		if (result)
			result = _r;

		if (result) {
			// transform define to normal form
			q = CDR(CAR(tail)); // argumnts
			p = CDR(tail); // body;
			q1 = new_id(vm, gc, SYMB_KW_LAMBDA);
			__protect(cons_scp,l, new_cons(vm, gc, new_cons(vm, gc, q1, new_cons(vm, gc, q, p) ), NIHIL));
			FIELD_SET(tail, car, CAR(CAR(tail)));
			FIELD_SET(tail, cdr, l);
		}
	} else {
		kw_error_asci(vm, ptr, p, "bad define syntax");
		result = false;
	}
ret: UNWIND_RSTK();
	return result;
}

bool kw_check_callcc(svm_t vm, __sbp(transformer_scp) ptr, __sbp(descr) cdat)
{
	bool result = true;
	SAVE_RSTK();
	__protect(cons_scp,tail, CDR(_DS(cdat)));

	if (cli_len(tail) < 1) {
		kw_error_asci(vm, ptr, _DS(cdat), "illegal call/cc syntax");
		result = false;
		goto ret;
	}

	descr p = CAR(tail);

	if(!datum_is_id(p) && !datum_is_carcdr(p)) {
		kw_error_asci(vm, ptr, _DS(cdat), "illegal call/cc syntax - literal can't be lambda");
		result = false;
		goto ret;
	}

	if(!kw_match_tail(vm, ptr, tail)) {
		result = false;
	}
ret:
	UNWIND_RSTK();
	return result;
}



bool kw_check_form(svm_t vm, __sbp(transformer_scp) ptr, __sbp(descr) cdat, int cx) {

	if (datum_is_carcdr(_DS(cdat))) {
		descr head = CAR(_DS(cdat));
		if (datum_is_id(head)) {
			descr kw = id_symbol(head);
			if (desc_is_spec(kw)) {
				ulong kwspec = desc_to_spec(kw);
				switch (kwspec) {
				case desc_to_spec(SYMB_KW_DEFINE):
					return kw_check_define(vm, ptr, cdat, cx);
				case desc_to_spec(SYMB_KW_LAMBDA):
					return kw_check_lambda(vm, ptr, cdat);
				case desc_to_spec(SYMB_KW_LAMBDAGREEK):
					return kw_check_lambda(vm, ptr, cdat);
				case desc_to_spec(SYMB_KW_IF):
					return kw_check_if(vm, ptr, cdat);
				case desc_to_spec(SYMB_KW_COND):
					return kw_check_cond(vm, ptr, cdat);
				case desc_to_spec(SYMB_KW_SET):
					return kw_check_set(vm, ptr, cdat);

				case desc_to_spec(SYMB_KW_LET):
					return kw_check_let(vm, ptr, (cons_scp*) cdat);
				case desc_to_spec(SYMB_KW_LET_STAR):
					return kw_check_let_star(vm, ptr, (cons_scp*) cdat);
				case desc_to_spec(SYMB_KW_LETREC):
					return kw_check_let(vm, ptr, (cons_scp*) cdat);
				case desc_to_spec(SYMB_QUOTE):
					return kw_check_quote(vm, ptr, (cons_scp*) cdat);
				case desc_to_spec(SYMB_QUASI):
					return kw_check_quasi(vm, ptr, (cons_scp*) cdat);
				case desc_to_spec(SYMB_UNQUOTE):
					return kw_check_unquote(vm, ptr, (cons_scp*) cdat, cx);
				case desc_to_spec(SYMB_SPLICING):
					return kw_check_unquote(vm, ptr, (cons_scp*) cdat, cx);

				case desc_to_spec(SYMB_KW_BEGIN):
				case desc_to_spec(SYMB_KW_BEGIN0):
					return kw_check_begin(vm,ptr, cdat);

				case desc_to_spec(SYMB_KW_CALLCC):
					return kw_check_callcc(vm, ptr, cdat);

				default:
					return kw_check_expr(vm, ptr, (cons_scp*) cdat);
				}
			} else {
				return kw_check_expr(vm, ptr, (cons_scp*) cdat);
			}
		} else {
			return kw_check_expr(vm, ptr, (cons_scp*) cdat);
		}
	} else if (datum_is_id(_DS(cdat))) {
		if (symbol_reserved(id_symbol(_DS(cdat)))) {
			kw_error_asci(vm, ptr, _DS(cdat), "keyword used as an identifier");
			return false;
		}
	}
	return true;
}



cons_scp lcod_new_mark(svm_t vm, __arg cons_scp markslist)
{
	decl_gc_of_vm();
	SAVE_RSTK();
	PUSH_RSTK(markslist);
	__ret cons_scp r = CONS(SYMB_VM_MARK, CONSCAR(markslist));
	UNWIND_RSTK();
	return r;
}

cons_scp lcod_new_set(svm_t vm, __arg descr id, __arg descr expr)
{
	assert(desc_is_var(id) && "lcode set requires variable" );
	decl_gc_of_vm();
	SAVE_RSTK();
	PUSH_RSTK2(id, expr);
	__ret cons_scp r = CONS(SYMB_VM_SET, CONS(id, CONSCAR(expr)));
	UNWIND_RSTK();
	return r;
}

cons_scp lcod_new_def(svm_t vm, __arg descr id, __arg descr expr)
{
	assert(desc_is_var(id) && "lcode def requires variable" );
	decl_gc_of_vm();
	SAVE_RSTK();
	PUSH_RSTK2(id, expr);
	__ret cons_scp r = CONS(SYMB_VM_DEF, CONS(id, CONSCAR(expr)));
	UNWIND_RSTK();
	return r;
}

cons_scp lcod_new_if(svm_t vm, __arg descr cond, __arg descr jmp)
{

	decl_gc_of_vm();
	SAVE_RSTK();
	PUSH_RSTK2(cond, jmp);
	__ret cons_scp r = CONS(SYMB_VM_IF, CONS(cond, jmp));
	UNWIND_RSTK();
	return r;
}




cons_scp lcod_new_jmp(svm_t vm, __arg cons_scp dest)
{
	decl_gc_of_vm();
	SAVE_RSTK();
	PUSH_RSTK(dest);
	__ret cons_scp r = CONS(SYMB_VM_JMP, CONSCAR(dest));
	UNWIND_RSTK();
	return r;
}


cons_scp lcod_new_ret(svm_t vm, __arg cons_scp expr)
{
	decl_gc_of_vm();
	SAVE_RSTK();
	PUSH_RSTK(expr);
	__ret cons_scp r = CONS(SYMB_VM_RET, CONSCAR(expr));
	UNWIND_RSTK();
	return r;
}



cons_scp lcod_new_call(svm_t vm, __arg cons_scp expr)
{
	assert(datum_is_carcdr(expr));
	decl_gc_of_vm();
	SAVE_RSTK();
	PUSH_RSTK(expr);
	 __protect(cons_scp, p, CDR(expr));
	 __protect(cons_scp, proc, CAR(expr));
	 __protect(vec_scp, argv, cl_to_vec(vm,gc, &p ) );
	p = CONS( proc, argv);
	__ret cons_scp r = (cons_scp) new_scons_p(vm, gc, SYMB_VM_CALL, p , syn_getpos(expr));
	UNWIND_RSTK();
	return r;
}

static ulong Fcode_uniq_num = 1;


fcode_scp new_fcode(svm_t vm, __sbp(transformer_scp) ptr, __arg fcode_scp parent, __arg env_scp env,
		__arg cl_scp args) {
	decl_gc_of_vm();
	SAVE_RSTK();
	PUSH_RSTK3(env, parent, args);
	__protect(cl_scp,p, NIHIL);
	__protect(cons_scp, lc_begin, CONSCAR(lcod_new_mark(vm, NIHIL)));
	__protect( fcode_scp, fc, (fcode_scp) inew_inst(gc, TYPEIX_CODEBLOCK));

	fc->children = NIHIL;
	fc->lcod = fc->lcod_end = lc_begin;
	fc->parent = parent;
	if(parent) {
		p = CONS(fc, parent->children);
		FIELD_SET(parent, children, p);
	}
	fc->argc = 0;
	fc->bits = 0;
	if(!env) {
		env = _DF(ptr, env);
	}
	env = new_env(vm, gc, env);
	p = args;
	while (datum_is_carcdr(p)) {
		if (!datum_is_id(CAR(p))) {
			_mthrow_(EXCEP_TRANSFORM_FATAL, "arguments list expected");
		}
		env_put(vm, env, id_symbol(CAR(p)), desc_from_local(0, fc->argc) );
		fc->argc ++;
		p = CDR(p);
	}
	if(!cl_end(p)) {
		if(!datum_is_id(p)) {
			_mthrow_(EXCEP_TRANSFORM_FATAL, "arguments list expected");
		}
		env_put(vm, env, id_symbol(p), desc_from_local(0, fc->argc) );
		fc->argc ++;
		fc->bits |= FCODE_BITM_MULTIARG;
	}

	FIELD_SET(fc, defenv  , env);
	fc->fvc = fc->argc;
	fc->frmax = 0;
	fc->uniq = Fcode_uniq_num ++;
	fc->ccode = fcod_entry_point(fc->uniq);

	UNWIND_RSTK();

	return fc;
}

void fcode_print(svm_t vm, oport_scp op, fcode_scp fc)
{
	__loc cons_scp p = CDR(fc->lcod);


	oport_write_asci(op, "\nicode  ");
	oport_print_int(op, fc->uniq);
	oport_write_asci(op, ":\n");

	if(cl_end(p)) {
		oport_write_asci(op, " <empty> \n");
		return;
	}
	until(cl_end(p))
	{
		descr cmd = CAR(p);
		if(!datum_is_carcdr(cmd)) {
			oport_print_int(op, cmd);
			return;

		}
		descr tag = CAR(cmd);

		switch(desc_to_spec(tag))
		{
		case desc_to_spec(SYMB_VM_IF):
			cmd = CDR(cmd);
			oport_write_asci(op, "(!IF ");
			oport_print_datum(vm, op, CAR(cmd));
			oport_write_asci(op, " .  mark:");
			oport_print_int(op, desc_to_fixnum( CAR(CDR(CAR(CDR(cmd))))));
			oport_write_asci(op, ")");
			break;


		case desc_to_spec(SYMB_VM_JMP):
			cmd = CDR(cmd);
			oport_write_asci(op, "(!JMP mark:");
			oport_print_int(op, desc_to_fixnum( CAR(CDR(CAR(CAR(cmd))))));
			oport_write_asci(op, ")");
			break;

		case desc_to_spec(SYMB_VM_MARK):
			cmd = CDR(cmd);
			oport_write_asci(op, "[->mark ");
			oport_print_int(op, desc_to_fixnum( CAR(cmd)));
			oport_write_asci(op, "]");
			break;
		default:
			oport_print_datum(vm, op, cmd);
		}
		oport_write_asci(op, "\n");

		p = CDR(p);
	}
}


inline descr lcod_sym(descr d)
{
	if(!datum_is_carcdr(d)) {
		return NIHIL;
	}

	switch(desc_to_spec(CAR(d))) {
	case desc_to_spec(SYMB_VM_IF): return SYMB_VM_IF;
	case desc_to_spec(SYMB_VM_CALL): return SYMB_VM_CALL;
	case desc_to_spec(SYMB_VM_SET):return SYMB_VM_SET;
	case desc_to_spec(SYMB_VM_JMP):return SYMB_VM_JMP;
	case desc_to_spec(SYMB_VM_LAMBDA):return SYMB_VM_LAMBDA;
	case desc_to_spec(SYMB_VM_MARK):return SYMB_VM_MARK;
	case desc_to_spec(SYMB_VM_RET):return SYMB_VM_RET;
	case desc_to_spec(SYMB_VM_CONT):return SYMB_VM_CONT;
	case desc_to_spec(SYMB_VM_CALLCC):return SYMB_VM_CALLCC;
	}
	return NIHIL;

}

void fcod_add_codelist(svm_t vm, __sbp(fcode_scp) pfc, __arg cons_scp p )
{
	SAVE_RSTK();
	PUSH_RSTK(p);
	__protect(cons_scp, q, _DF(pfc, lcod_end));
	FIELD_SET(q, cdr, p);
	cons_scp last = cl_last(p);
	assert(last);
	FIELD_DSET(pfc, lcod_end, last);
	UNWIND_RSTK();
}

void fcod_add_code(svm_t vm, __sbp(fcode_scp) pfc, __arg descr code ) {
	decl_gc_of_vm();
	fcod_add_codelist(vm, pfc, CONSCAR(code));
}

inline closure_scp new_closure(svm_t vm, __arg fcode_scp fc)
{
	if(fc->closure) {
		return fc->closure;
	}
	decl_gc_of_vm();
	SAVE_RSTK();
	PUSH_RSTK(fc);
	__ret closure_scp cls = (closure_scp) inew_array(gc, TYPEIX_CLOSURE, fcod_framemax(fc));
	/*cache*/
	cls->argc =  fcod_argc(fc);
	 cls->bits = 0;
	if(fcodp_multiarg(fc) ) { cls->bits |= CLOSR_MULTI_ARGS; }
	cls->bin = NULL;
	cls->fcod = fc;
	cls->category = CLOSR_CAT_INTERP;
	UNWIND_RSTK();
	return cls;
}



descr __pure fcod_pop_local(transformer_scp ptr, fcode_scp fc) {
	arlist_scp ids = ptr-> arl;
	if (!arlist_is_empty(ids)) {
		return arlist_pop(ids);
	}
	return desc_from_local(0, fc->fvc++);
}

void fcod_push_local(svm_t vm, __sbp(transformer_scp) ptr, __arg descr loc) {
	decl_gc_of_vm();
	assert(desc_is_local(loc));
	arlist_push(vm, gc, _DF(ptr, arl), loc);
}

void fcod_cache_closure(svm_t vm, __sbp(fcode_scp) pfc)
{
	if(fcod_framemax(_DS(pfc)) == 0 && fcod_clsrcache(_DS(pfc)) == NIHIL) {
		SAVE_RSTK();
		__protect(closure_scp, c, new_closure(vm, _DS(pfc)));
		FIELD_DSET(pfc, closure, c);
		UNWIND_RSTK();
	}
}

descr anf_resolv_var(svm_t vm, __sbp(transformer_scp) ptr,  __sbp(fcode_scp) pfc, __arg env_scp env, __arg descr id)
{
	assert(datum_is_id(id) && "precond - mst be id");
	int fr = 0;
	descr r = env_lookup_d(vm, env, id_symbol(id), &fr);
	descr ret = NIHIL;
	if(desc_is_global(r)) {
		ret = r;
	} else if(desc_is_local(r)) {
		_DS(pfc)->frmax = max(_DS(pfc)->frmax, fr);
		ret =  desc_from_local(fr, desc_loc_ix(r));
	} else {
		assert(r == NIHIL);
	}
	if(ret) {
		/*bound case*/
		((ident_scp)id)->var = ret;
	} else {
		SAVE_RSTK();
		PUSH_RSTK(id);
		__protect(descr, g, desc_from_glob(global_reserve(vm)));
		env_put(vm, _DF(ptr, env ), id_symbol(id), g );
		UNWIND_RSTK();
		((ident_scp)id)->var = g;
	}
	return id;
}

#define trf_globfcode(tr)   (tr)->gfc








descr anf_rid(svm_t vm,  __sbp(transformer_scp) ptr, __sbp(fcode_scp) pfc,
		__arg descr rid, __arg descr expr)
{
	SAVE_RSTK();
	PUSH_RSTK2(expr, rid);
	__protect( descr, var,  NIHIL);
	if(rid == SYMB_UNDEF) {
		// temporary set
		if(!lcod_sym(expr)) {
			var = expr;
		} else {
			var = fcod_pop_local(_DS(ptr), _DS(pfc));
			fcod_add_code(vm, pfc,
						lcod_new_set(vm,
								var,	expr));
		}
	} else if(rid == SYMB_VM_RET) {
		// ret
		fcod_add_code(vm, pfc,
							lcod_new_ret(vm, expr));
	} else if(desc_is_var(rid)) {
		// set!
		var = rid;
		fcod_add_code(vm, pfc,
					lcod_new_set(vm,
							rid,	expr));
	} else if(datum_is_box(rid)) {
		// define
		var = unbox(rid);
		fcod_add_code(vm, pfc,
					lcod_new_def(vm,
							var,	expr));
	} else if(rid == NIHIL) {
		if(lcod_sym(expr)) {
			fcod_add_code(vm, pfc, expr);
		}
	} else {
		assert(0 && "never here");
	}
	UNWIND_RSTK();
	return var;
}


descr anf_match_define(svm_t vm, __sbp(transformer_scp) ptr,  __sbp(fcode_scp) pfc, __sbp(env_scp) penv,
		__arg descr rid, __arg descr p )
{
	decl_gc_of_vm();
	SAVE_RSTK();
	PUSH_RSTK2(p, rid);
	p = CDR(p);
	__protect(descr, id, CAR(p));
	if(!datum_is_id(id)) {
		_mthrow_(EXCEP_TRANSFORM_FATAL, "var expected in define clause");
	}
	anf_resolv_var(vm, ptr, pfc, _DS(penv), id);
	if(!desc_is_global(id_var(id))) {
		_mthrow_(EXCEP_TRANSFORM_FATAL, "global var expected in define clause");
	}

	p = CDR(p);
	anf_match_form(vm, ptr, pfc, penv, MAKEBOX(id),  CAR(p) );
	p = anf_rid(vm, ptr, pfc, rid, SYMB_VOID);
	UNWIND_RSTK();
	return p;
}


descr anf_match_expr(svm_t vm, __sbp(transformer_scp) ptr, __sbp(fcode_scp) pfc,  __sbp(env_scp) penv,
		__arg descr rid, __arg descr frm)
{

	if(!datum_is_id(CAR(frm)) && !datum_is_carcdr(CAR(frm))) {
		kw_error_asci(vm, ptr, frm, "bad application");
		_mthrow_(EXCEP_TRANSFORM_FATAL, "bad application");
	}

	SAVE_RSTK();
	PUSH_RSTK2(frm, rid);
	__protect(cons_scp, q, NIHIL);
	__protect(cons_scp, p, frm);

	until(cl_end(p)) {
		q = anf_match_form(vm, ptr, pfc, penv, SYMB_UNDEF, CAR(p));
		FIELD_SET(p, car, q);
		p = CDR(p);
	}

	p = frm;
	until(cl_end(p)) {
		if(desc_is_local(CAR(p))) {
			fcod_push_local(vm, ptr, CAR(p));
		}
		p = CDR(p);
	}
	p = anf_rid(vm, ptr, pfc, rid, lcod_new_call(vm, frm));
	UNWIND_RSTK();
	return p;
}

descr anf_match_quote(svm_t vm, __sbp(transformer_scp) ptr, __sbp(fcode_scp) pfc,  __sbp(env_scp) penv,
		__arg descr rid, __arg descr frm)
{
	SAVE_RSTK();
	PUSH_RSTK2(frm, rid);
	__protect(cons_scp, p, frm);
	__loc descr q = anf_match_form(vm, ptr, pfc, penv, SYMB_UNDEF, CAR(p));
	FIELD_SET(p, car, q);
	q = anf_rid(vm, ptr, pfc, rid, lcod_new_call(vm, frm));
	UNWIND_RSTK();
	return q;
}



descr anf_match_callcc(svm_t vm, __sbp(transformer_scp) ptr, __sbp(fcode_scp) pfc,  __sbp(env_scp) penv,
		__arg descr rid, __arg descr frm)
{
	SAVE_RSTK();
	decl_gc_of_vm();
	PUSH_RSTK2(frm, rid);
	__protect(cons_scp, p, CDR(frm));
	__loc descr q = anf_match_form(vm, ptr, pfc, penv, SYMB_UNDEF, CAR(p));
	__protect(descr, r, CONS(SYMB_VM_CALLCC, CONSCAR(q)));
	q = anf_rid(vm, ptr, pfc, rid, r);
	UNWIND_RSTK();
	return q;
}


descr anf_match_body(svm_t vm, __sbp(transformer_scp) ptr, __sbp(fcode_scp) pfc,  __sbp(env_scp) penv,
		__arg descr rid, __arg descr frm)
{
	SAVE_RSTK();
	PUSH_RSTK2(frm, rid);
	__protect(cons_scp, p, frm);
	assert(datum_is_cl(p) && "match body");
	if(cl_end(p)) {
		p = anf_rid(vm, ptr, pfc, rid, SYMB_VOID);
		UNWIND_RSTK();
		return p;
	}

	until(cl_end(CDR(p))) {
		anf_match_form(vm, ptr, pfc, penv, NIHIL, CAR(p));
		p = CDR(p);
	}

	p = anf_match_form(vm, ptr, pfc, penv, rid, CAR(p));
	UNWIND_RSTK();
	return p;
}



descr anf_match_if(svm_t vm, __sbp(transformer_scp) ptr, __sbp(fcode_scp) pfc,  __sbp(env_scp) penv,
		__arg descr rid, __arg descr frm)
{
	SAVE_RSTK();
	decl_gc_of_vm();
	PUSH_RSTK2(frm, rid);
	frm = CDR(frm);
	__protect(cons_scp, p, frm);
	__protect(descr, cond, anf_match_form(vm, ptr, pfc, penv, SYMB_UNDEF, CAR(p)));

	if(!desc_is_var(cond))
	{
		if(cond == SYMB_F) {
			p = CDR(CDR(p));
			p = cl_end(p) ? SYMB_VOID : CAR(p);
			p = anf_match_form(vm, ptr, pfc, penv, rid, p);
		} else {
			p = anf_match_form(vm, ptr, pfc, penv, rid, CAR(CDR(p)));
		}
	} else {

		__protect(cons_scp, q, CONSCAR(CONS(SYMB_VM_MARK, CONSCAR(desc_from_fixnum(anf_get_mark())))));

		fcod_add_code(vm, pfc, CONS(SYMB_VM_IF, CONS(cond, q)) );
		if(rid == SYMB_UNDEF) { // if in context of expression
			rid = fcod_pop_local(_DS(ptr), _DS(pfc));
		}

		p = CDR(p);
		anf_match_form(vm, ptr, pfc, penv, rid, CAR(p));
		p = CDR(p);
		unless(rid == SYMB_VM_RET) {
			frm = CONSCAR(CONS(SYMB_VM_MARK, CONSCAR(desc_from_fixnum(anf_get_mark()))));
			fcod_add_code(vm, pfc, CONS(SYMB_VM_JMP, CONSCAR(frm)));
		}
		fcod_add_codelist(vm, pfc, q);
	//	unless(rid == NIHIL) {
			p = cl_end(p) ? SYMB_VOID : CAR(p);
			anf_match_form(vm, ptr, pfc, penv, rid, p);
		unless(rid == SYMB_VM_RET) {
			fcod_add_codelist(vm, pfc, frm);
		}
	//	}
		p = rid;

	}
	UNWIND_RSTK();
	return p;
}

descr anf_match_set(svm_t vm, __sbp(transformer_scp) ptr, __sbp(fcode_scp) pfc,  __sbp(env_scp) penv,
		__arg descr rid, __arg cons_scp p)
{
	SAVE_RSTK();
	PUSH_RSTK2(p,rid);
	p = CDR(p);
	__protect(descr, id, CAR(p));
	if(!datum_is_id(id)) {
		_mthrow_(EXCEP_TRANSFORM_FATAL, "var expected in define clause");
	}
	anf_resolv_var(vm, ptr, pfc, _DS(penv), id);

	//vm_stk_printall(vm, Out_console_port);
	// gc_collect(vm);
	//vm_stk_printall(vm, Out_console_port);
	p = CDR(p);
	anf_match_form(vm, ptr, pfc, penv, id,  CAR(p));
	p = anf_rid(vm, ptr, pfc, rid, SYMB_VOID);
	UNWIND_RSTK();
	return p;
}



descr anf_match_let(svm_t vm, __sbp(transformer_scp) ptr, __sbp(fcode_scp) pfc,  __sbp(env_scp) penv,
		__arg descr rid, __arg descr frm, bool rec)
{
	SAVE_RSTK();
	PUSH_RSTK2(frm, rid);

	frm = CDR(frm);

	__protect(env_scp, env, _DS(penv));
	__protect(cons_scp, p, CAR(frm));
	__protect(descr, id, NIHIL);


	until(cl_end(p)) {
		id = CAR(CAR(p));
		_mthrow_assert_(datum_is_id(id), EXCEP_TRANSFORM_FATAL, "bad let - var expected");
		id_bind(id,  fcod_pop_local(_DS(ptr), _DS(pfc)));
		env = env_extend(vm, env, id_symbol(id), id_var(id));
		if(!rec) {
			anf_match_form(vm, ptr, pfc, penv, id, CAR(CDR(CAR(p))));
		}
		p = CDR(p);
	}
	if(rec) {
		p = CAR(frm);
		until(cl_end(p)) {
			id = CAR(CAR(p));
			anf_match_form(vm, ptr, pfc, penv, id, SYMB_UNDEF);
			p = CDR(p);
		}

		p = CAR(frm);
		until(cl_end(p)) {
			id = CAR(CAR(p));
			anf_match_form(vm, ptr, pfc, &env, id, CAR(CDR(CAR(p))));
			p = CDR(p);
		}
	}

	p = CDR(frm);
	p = anf_match_body(vm, ptr, pfc, &env, rid, p);
	UNWIND_RSTK();
	return p;
}

descr anf_match_lambda(svm_t vm, __sbp(transformer_scp) ptr, __sbp(fcode_scp) pfc,  __sbp(env_scp) penv,
		__arg descr rid, __arg descr frm)
{
	SAVE_RSTK();
	PUSH_RSTK2(frm, rid);
	frm = CDR(frm);
	__protect(descr, p, SYMB_VOID);
	if(rid != NIHIL)
	{
		decl_gc_of_vm();
		__protect(fcode_scp, fc,  new_fcode(vm, ptr, _DS(pfc), _DS(penv), CAR(frm)));
		frm = CDR(frm);
		FIELD_SET(fc, synbody, frm);
		p = CONS(SYMB_VM_LAMBDA, CONSCAR(fc));
		p = anf_rid(vm, ptr, pfc, rid, p);
	}
	UNWIND_RSTK();
	return p;
}




descr anf_match_begin (svm_t vm, __sbp(transformer_scp) ptr, __sbp(fcode_scp) pfc,  __sbp(env_scp) penv,
		__arg descr rid, __arg descr frm)
{
	return anf_match_body(vm, ptr, pfc, penv, rid, CDR(frm));
}


descr anf_match_quasiquotes(svm_t vm, __sbp(transformer_scp) ptr, __sbp(fcode_scp) pfc,  __sbp(env_scp) penv,
		__arg descr rid, __arg descr frm)
{
	SAVE_RSTK();
	PUSH_RSTK2(frm, rid);
	decl_gc_of_vm();
	__protect(cons_scp, p, frm);
	__protect(cons_scp, r, NIHIL);
	__protect(cons_scp, q, NIHIL);
	__protect(cons_scp, r1, NIHIL);
	__protect(cons_scp, q1, NIHIL);
	__protect(descr, u, NIHIL);
	__protect(ident_scp, qqid, new_id(vm, gc, symbol_from_string(vm, new_string_asci(vm, gc, "list-cat"))));
	qqid->pos = syn_getpos(frm);
	q = r = CONSCAR(qqid);

	p = CAR(CDR(p));


	until(cl_end(p)) {
		 u = CAR(p);

		if(datum_is_carcdr(u)) {
			u = CAR(u);
			if(datum_is_id(u) && desc_is_spec(id_symbol(u))) {
				descr sym = id_symbol(u);
				switch(desc_to_spec(sym)) {
				case desc_to_spec(SYMB_UNQUOTE):
				case desc_to_spec(SYMB_SPLICING):
					if(r1) {
						FIELD_SET(q1, cdr, NIHIL);
						qqid = new_id(vm, gc, SYMB_QUOTE);
						r1 =  CONSCAR(CONS(qqid, CONSCAR(r1)));
						FIELD_SET(q, cdr, r1); q = r1; r1 = NIHIL;
					}
					qqid = new_id(vm, gc, sym);
					u = anf_match_form(vm, ptr, pfc, penv, SYMB_UNDEF,CONS(qqid, CONSCAR(CAR(CDR(CAR(p))))));
					u = CONSCAR(u);
					FIELD_SET(q, cdr, u); q = u;
					goto loop;
				}
			}
		}

		if(r1 == NIHIL) {
			r1 = q1 = p;
		} else {
			q1 = p; // q1 = is previous
		}

loop:
		p = CDR(p);
	}

	if(r1) {
		FIELD_SET(q1, cdr, NIHIL);
		qqid = new_id(vm, gc, SYMB_QUOTE);
		r1 =  CONSCAR(CONS(qqid, CONSCAR(r1)));
		FIELD_SET(q, cdr, r1); q = r1; r1 = NIHIL;
	}

	q = anf_match_form(vm, ptr, pfc, penv, rid, r);
	UNWIND_RSTK();
	return q;
}


// returns replacement for the form.
descr anf_match_form(svm_t vm, __sbp(transformer_scp) ptr, __sbp(fcode_scp) pfc, __sbp(env_scp) penv,
		__arg descr rid, __arg descr frm)
{
	if (datum_is_carcdr(frm)) {
		descr head = CAR(frm);
		if (datum_is_id(head)) {
			descr kw = id_symbol(head);
			if (desc_is_spec(kw)) {
				ulong kwspec = desc_to_spec(kw);
				switch (kwspec) {
				case desc_to_spec(SYMB_KW_DEFINE):
					return anf_match_define(vm, ptr, pfc, penv, rid, frm);
				case desc_to_spec(SYMB_KW_BEGIN):
					return anf_match_begin(vm, ptr, pfc, penv, rid, frm);
				case desc_to_spec(SYMB_KW_IF):
					return anf_match_if(vm, ptr, pfc, penv, rid, frm);
				case desc_to_spec(SYMB_QUOTE):
					return anf_match_quote(vm, ptr, pfc, penv, rid, frm);
				case desc_to_spec(SYMB_KW_LET):
					return anf_match_let(vm, ptr, pfc, penv, rid, frm, false);
				case desc_to_spec(SYMB_KW_LETREC):
					return anf_match_let(vm, ptr, pfc, penv, rid, frm, true);
				case desc_to_spec(SYMB_KW_SET):
					return anf_match_set(vm, ptr, pfc, penv, rid, frm);
				case desc_to_spec(SYMB_KW_LAMBDA):
				case desc_to_spec(SYMB_KW_LAMBDAGREEK):
					return anf_match_lambda(vm, ptr, pfc, penv, rid, frm);
				case desc_to_spec(SYMB_KW_CALLCC):
					return anf_match_callcc(vm, ptr, pfc, penv, rid, frm);
				case desc_to_spec(SYMB_QUASI):
					return anf_match_quasiquotes(vm, ptr, pfc, penv, rid, frm);
			//	case desc_to_spec(SYMB_UNQUOTE):
			//	case desc_to_spec(SYMB_SPLICING):

				default:
					return anf_match_expr(vm, ptr, pfc, penv, rid, frm);
				}
			} else {
				return anf_match_expr(vm, ptr, pfc, penv, rid, frm);

			}
		} else {
			return anf_match_expr(vm, ptr, pfc, penv, rid, frm);
		}
	} else  {
		/*atomic term case*/

		if(datum_is_id(frm)) {
			anf_resolv_var(vm, ptr, pfc, _DS(penv), frm);
		}

		SAVE_RSTK(); PUSH_RSTK(frm);
		anf_rid(vm, ptr, pfc, rid, frm);
		UNWIND_RSTK();
		return frm;
	}
	assert(0 && "never here");
	return 0;

}


#include "interpret.h"

void icod_create(svm_t vm, __arg fcode_scp fc);
void icod_print(svm_t vm,  fcode_scp fc, oport_scp op);


bool Anf_verbose = false;

void anf_finalize_fcode(svm_t vm, __sbp(transformer_scp) ptr, __arg fcode_scp fc)
{
	SAVE_RSTK();
	PUSH_RSTK(fc);
	__protect(env_scp, env, fc->defenv);


	arlist_clear(_DF(ptr, arl));

	anf_match_body(vm, ptr, _RS(fc), &env, SYMB_VM_RET, fc->synbody);

	fcod_cache_closure(vm, _RS(fc));

	if(Anf_verbose) {
		fcode_print(vm, Out_console_port, fc);
	}

	icod_create(vm, fc);
	if(Anf_verbose) {
		icod_print(vm, fc, Out_console_port);
	}



	__protect(cons_scp, p, fc->children);

	until(cl_end(p)) {
		anf_finalize_fcode(vm, ptr, CAR(p));
		p = CDR(p);
	}
	UNWIND_RSTK();
}


void trf_prepare(svm_t vm, __sbp(transformer_scp) ptr, __arg descr datum)
{
	SAVE_RSTK();
	_DF(ptr, bits) &= ~TRANSF_BITM_ERR_KWSYN;
	PUSH_RSTK(datum);
	__loc fcode_scp fc = new_fcode(vm, ptr, NIHIL, NIHIL, SYMB_NIL);
	fc->synbody = datum;
	FIELD_DSET(ptr, gfc, fc);

	UNWIND_RSTK();
}

bool trf_transform_to_anf(svm_t vm, __sbp(transformer_scp) ptr, __arg descr datum)
{
	bool result = true;
	SAVE_RSTK();
	_try_
		trf_prepare(vm, ptr, datum);

		anf_finalize_fcode(vm, ptr,  _DF(ptr, gfc));
	_except_
	_catch_(EXCEP_TRANSFORM_FATAL)
		log_excep_error(Sys_log);
		printf("\n Error[TRANS]: %s ", excep_msg() );
		UNWIND_RSTK();
		(*ptr)->bits |= TRANSF_BITM_ERR_KWSYN;
		result  = false;
	_end_try_

	UNWIND_RSTK();
	return result;
}












void syn_test(svm_t vm, __sbp(parser_scp) ppr, __sbp(oport_scp) pwp) {
	decl_gc_of_vm();
	SAVE_RSTK();
	__protect(descr,d, NIHIL);
	__protect(transformer_scp,tr, new_transformer(vm));
	__protect(fcode_scp, gfc, trf_globfcode(tr));
	__protect(env_scp, genv, tr->env);
	_try_
		while (!pars_eof(_DS(ppr))) {

			d = pars_read_datum(vm, ppr);

			//_mthrow_(EXCEP_TRANSFORM_FATAL, "hahha");

			if (!d) {
				oport_write_asci(_DS(pwp), "\n");
				break;
			}
			if (!parsp_error(_DS(ppr))) {
				trf_check_syntax(vm, _RS(tr), _RS(d) );
			}
			oport_write_asci(_DS(pwp), "\n");
			oport_print_datum(vm, _DS(pwp), d);

			if(!trfp_error(tr) ) {
				//arlist_clear(tr->arl);
			//	anf_match_form(vm, &tr, &gfc, &genv, NIHIL, d);
				trf_transform_to_anf(vm, &tr, CONSCAR(d));
				//oport_print_datum(vm, _DS(pwp), tr->gfc->lcod);
				//fcode_print(vm, _DS(pwp), tr->gfc);
			}
		}
		oport_write_asci(_DS(pwp), "\n");
	_except_
	_catchall_
		UNWIND_RSTK();
		fprintf(stderr, "error - %s\n", excep_msg());
		_rethrow_;
	_end_try_

	UNWIND_RSTK();
}

