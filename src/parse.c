/* parse.c
 * musc- scheme translator & interpreter
 * author: denis.lepekhin@gmail.com
 * created on: 2009
 */


#include "loggers.h"
#include "opcore.h"
#include <malloc.h>
#include <ustdio.h>
#include <string.h>
#include <uchar.h>
#include "sysexcep.h"
#include "charcat.h"
#include "parse.h"
#include "opsys.h"

#define chrp_eofdelim(ch)  (((ch) == EOF_CHAR) || chrp_delim(ch))
inline void    __pure readch_first_notwhsp(svm_t vm, parser_scp par);
inline bigchar __pure readch(svm_t vm, parser_scp par);
inline bigchar __pure readp(svm_t vm, parser_scp par);
inline void readch_first_notwhsp(svm_t vm, parser_scp par);
inline bool readch_precise_asci(svm_t vm,  parser_scp par,  const char* asci);
inline void readch_first_delim(svm_t vm, parser_scp par);
int64_t lex_read_digits(svm_t vm, parser_scp pr, byte radix);
int ss_read_digits(svm_t vm, const UChar* buf, byte radix, uint64_t *n);
void __pure set_readp_ix(parser_scp pr, int ix);

inline void skip_comment_line(svm_t vm,    parser_scp par);
inline void skip_comment_nested(svm_t vm,  __sbp(parser_scp) par);



descr lex_read_identifier(svm_t vm,  __sbp(parser_scp) ppr);
descr lex_read_number(svm_t vm, __sbp(parser_scp) ppr);
descr lex_read_string (svm_t vm,     __sbp(parser_scp) ppr);
descr	lex_read_char(svm_t vm,        __sbp(parser_scp) ppr);

descr bin_print_identifier(svm_t vm, uint argc, __binargv(argv));


descr syn_match_datum(svm_t vm, __sbp(parser_scp) ppr);
descr syn_read_datum(svm_t vm, __sbp(parser_scp) ppr);
descr syn_read_compound(svm_t vm, __sbp(parser_scp) ppr, descr sort);

static os_thread_key_t Bufparse_key;


void parser_init(svm_t vm)
{
	global_once_preamble();

	// requires module:
	ioport_init(vm);
	type_scp t = type_reserve(vm, TYPEIX_SYNTAX_CONS, "<syntax-pair>",  TYPE_CATEGORY_NONTERM | TYPE_CATEGORY_CARCDR | TYPE_CATEGORY_SYNTAX,
				sizeof(struct ConsSyntaxDatum), 0, (ulong) &((scons_scp)(0))->car, _SLOTC_SYNTAX_PAIR);
	t->printer = new_bin(2, bin_print_cons);


	t = type_reserve(vm, TYPEIX_SYNTAX_IDENT, "<identifier>", TYPE_CATEGORY_NONTERM | TYPE_CATEGORY_SYNTAX,
					sizeof(struct IdentifierDatum), 0, (ulong) &((ident_scp)(0))->pos, _SLOTC_IDENTIFIER);
	t->printer = new_bin(2, bin_print_identifier);
	type_reserve(vm, TYPEIX_PARSER, "<scm-parser>", TYPE_CATEGORY_NONTERM,
					sizeof(struct ParserDatum), 0, (ulong) &((parser_scp)(0))->iport, _SLOTC_PARSER_SCM);
	type_setprint(type_from_ix(TYPEIX_PARSER),  Bin_print_defshort);
	type_reserve(vm, TYPEIX_TEXTPOS, "<text-pos>", TYPE_CATEGORY_CONST,
					sizeof(struct TextPosDatum), 0, 0, 0);
	os_thread_key_create(&Bufparse_key);
	os_thread_key_set(Bufparse_key, new_string(vm, Gc_malloc, 4096));
	log_info(Sys_log, "module parser init ok.");
}



descr bin_print_identifier(svm_t vm, uint argc, __binargv(argv))
{

	SAVE_RSTK();
	__protect(ident_scp, datum , (ident_scp) argv[1]);
	__protect(descr, d, datum->symbol);
	oport_print_datum(vm, argv[0], d);
	if(id_var(datum) != SYMB_UNDEF) {
		oport_print_datum(vm, argv[0], id_var(datum)  );
	}
	UNWIND_RSTK();
	return SYMB_VOID;
}


inline txtpos_scp syn_getpos(descr syntaxobj)
{
	if(datum_is_instof_ix(syntaxobj, TYPEIX_SYNTAX_IDENT)) {
		return ((ident_scp)syntaxobj)->pos;
	} else if(datum_is_instof_ix(syntaxobj, TYPEIX_SYNTAX_CONS)) {
		return ((scons_scp)syntaxobj)->pos;
	} else {
		return NULL;
	}
}




#define txp_set(t, r1, c1, r2, c2) \
	(t)->row_begin = r1 ; (t)->col_begin = c1; \
	(t)->row_end = r2; (t)->col_end = c2

#define txp_oset(t, o)   txp_set(t, (o)->row_begin, (o)->col_begin, (o)->row_end, (o)->col_end)


#define txp_set_begin(t, r1, c1) \
	(t)->row_begin = r1 ; (t)->col_begin = c1
#define txp_set_end(t, r2, c2) \
	(t)->row_end = r2; (t)->col_end = c2
#define txp_set_relend(t, dr, dc) \
	(t)->row_end = (t)->row_begin + (dr) ; \
	(t)->col_end =  (t)->col_begin + (dc)

#define txp_set_relbegin(t, dr, dc) \
	(t)->row_begin = (t)->row_end + (dr) ; \
	(t)->col_begin =  (t)->col_end + (dc)


txtpos_scp new_txtpos(svm_t vm) {
	decl_gc_of_vm();
	txtpos_scp t = (txtpos_scp) inew_inst(gc, TYPEIX_TEXTPOS);

	return t;
}

txtpos_scp new_txtpos_lex(svm_t vm, __sbp(parser_scp) ppr)
{
	txtpos_scp t = new_txtpos(vm);
	txtpos_scp p = &_DS(ppr)->poslex;
	txp_set(t, p->row_begin, p->col_begin, p->row_end, p->col_end);
	return t;
}


inline void lex_error_u(svm_t vm, __sbp(parser_scp) ppr, int excep, struct TextPosDatum* p, const UChar* msg)
{


	const char *fmt;
	switch(excep) {

	case EXCEP_PARSE_LEX_BADLEX: case EXCEP_PARSE_LEX_STOP: case PARS_LEX_WARNING:
		readch_first_delim(vm, _DS(ppr));
		fmt = (excep == 0) ?  "Warning[LEX] %i:%i-%i:%i - %S\n" : "Error[LEX] %i:%i-%i:%i - %S\n";
		u_fprintf(ustderr, fmt,
				p->row_begin, p->col_begin,
				_DF(ppr, linecur), _DF(ppr, colcur), msg);

		_DS(ppr)->bits |= PARS_BITM_ERR_LEX;
		if((_DF(ppr, bits) & PARS_BITM_ALWAYSFATAL) && excep==EXCEP_PARSE_LEX_BADLEX ) {
			excep = EXCEP_PARSE_LEX_STOP;
		}
		break;
	case EXCEP_PARSE_DAT_STOP: case EXCEP_PARSE_DAT: case PARS_DAT_ERROR:
		fmt = "Error[DATUM] %i:%i-%i:%i - %S\n";
				u_fprintf(ustderr, fmt,
						p->row_begin, p->col_begin,
						p->row_end, p->col_end, msg);
				_DS(ppr)->bits |= PARS_BITM_ERR_DAT;
		break;
	}

	if(excep > 0) {
		_throw_(excep);
	}
}


#define lex_error_f(vm, ppr, ex, p, format, ...) \
{\
		UChar _u[256]; u_sprintf(_u, format, __VA_ARGS__);\
		lex_error_u(vm, ppr, ex, p, _u); \
}

#define lex_error_asci(vm, ppr, ex, p,asci)    lex_error_f(vm,ppr, ex,p, "%s", asci)

ident_scp new_id(svm_t vm, gc_t gc, __arg symbol_d sym)
{
	SAVE_RSTK();
	PUSH_RSTK(sym);
	__ret ident_scp id = (ident_scp)inew_inst(gc, TYPEIX_SYNTAX_IDENT);
	id->symbol = sym;
	id->pos = NIHIL;
	id->var = SYMB_UNDEF;
	UNWIND_RSTK();
	return id;
}

inline bool id_eq(descr id1, descr id2)
{
	return id_symbol(id1) == id_symbol(id2);
}

ident_scp new_id_lex(svm_t vm, __sbp(parser_scp) ppr)
{
	SAVE_RSTK();
	decl_gc_of_vm();
	__protect(string_scp, n,  _DF(ppr,  buflex));
	ss_clr_hash(n);
	if(chrp_delim(ss_ucs(n)[ss_len(n)-1])) {
		ss_dec(n , 1);
	}
	__protect(txtpos_scp, pos, new_txtpos_lex(vm, ppr) );
	__protect( symbol_d, sym, symbol_from_string(vm, n));
	__ret ident_scp id = (ident_scp)inew_inst(gc, TYPEIX_SYNTAX_IDENT);
	id->pos = pos;
	txp_set_relend((id->pos), 0,  ss_len(n));
	id->symbol = sym;
	id->var = SYMB_UNDEF; // unbound;
	UNWIND_RSTK();
	return id;
}



scons_scp new_scons(svm_t vm, gc_t gc, __arg descr car, __arg descr cdr)
{
	SAVE_RSTK();
	PUSH_RSTK2(car,cdr);
	__ret scons_scp result = (scons_scp) inew_inst(gc, TYPEIX_SYNTAX_CONS);
	result->car = car;
	result->cdr = cdr;
	UNWIND_RSTK();
	return result;
}

scons_scp new_scons_p(svm_t vm, gc_t gc, __arg descr car, __arg descr cdr, __arg txtpos_scp pos)
{
	SAVE_RSTK();
	PUSH_RSTK3(car,cdr,pos);
	__ret scons_scp result = (scons_scp) inew_inst(gc, TYPEIX_SYNTAX_CONS);
	result->car = car;
	result->cdr = cdr;
	result->pos = pos;
	UNWIND_RSTK();
	return result;
}





string_scp new_string_lex(svm_t vm, __sbp(parser_scp) ppr)
{
	decl_gc_of_vm();
	SAVE_RSTK();
	__protect(string_scp, result, new_string(vm, gc, ss_len(_DF(ppr, buflex))));
	parser_scp pr = _DS(ppr);
	string_scp src = pr->buflex;
//	ss_dec(src, 1);

	ulong i  = 1;
	ulong count = ss_len(src);
	bigchar ch;
	UChar *srcbuf = ss_ucs(src);

	UTF_NEXT_CHAR_UNSAFE(srcbuf, i, ch );

	while(i < count) {
		switch(ch) {
		case CHR_RET:
			UTF_NEXT_CHAR_UNSAFE(srcbuf, i, ch );
			if(ch == CHR_LINEFEED) {
				ss_add(vm, NULL, &result, ch);
				UTF_NEXT_CHAR_UNSAFE(srcbuf, i, ch );
			} else {
				ss_add(vm, NULL, &result, CHR_RET);
			}
			break;
		case CHR_SLASH: {
			UTF_NEXT_CHAR_UNSAFE(srcbuf, i, ch );
			switch(ch) {
			case 'x': {


				uint64_t n;
				int c = ss_read_digits(vm, srcbuf + i, 16, &n);
				bigchar x = (bigchar) n;
				if(c == 0) {
						lex_error_f(vm, ppr, EXCEP_PARSE_LEX_BADLEX, &pr->poslex,
											"after \\x unicode hex code point expected in string", x);
				}
				i +=  c;
				UTF_NEXT_CHAR_UNSAFE(srcbuf, i, ch );
				if(ch != ';') {
						lex_error_f(vm, ppr, EXCEP_PARSE_LEX_BADLEX, &pr->poslex,
											"\\x<num> not terminated by semicolon ; in string", x);
				}
				UTF_NEXT_CHAR_UNSAFE(srcbuf, i, ch );
				if(!U_IS_UNICODE_CHAR(x)) {
					lex_error_f(vm, ppr, EXCEP_PARSE_LEX_BADLEX, &pr->poslex,
										"incorrect unicode code point in string: \\x%x", x);
				}
				ss_add(vm, NULL, &result, x);
			}break;
			case CHR_RET:
				UTF_NEXT_CHAR_UNSAFE(srcbuf, i, ch );
				if(ch == CHR_LINEFEED) {
					UTF_NEXT_CHAR_UNSAFE(srcbuf, i, ch );
				}
				break;
			case CHR_LINEFEED:
				UTF_NEXT_CHAR_UNSAFE(srcbuf, i, ch );
				break;
			default: {
				bigchar x = chr_from_esc(ch);
				if(x == -1) {
					lex_error_f(vm, ppr, EXCEP_PARSE_LEX_BADLEX, &pr->poslex,
															"incorrect escape sequence in string: \\%C", ch);
				}
				ss_add(vm, NULL, &result, x);
				UTF_NEXT_CHAR_UNSAFE(srcbuf, i, ch );
			}
			}
		} //endof slash!
		break;

		case CHR_DBLQUOT:
			goto loop_exit;
		default:
			ss_add(vm, NULL, &result, ch);
			UTF_NEXT_CHAR_UNSAFE(srcbuf, i, ch );
		}
	}
loop_exit:
	UNWIND_RSTK();
	return result;
}


parser_scp new_parser(svm_t vm, __arg iport_scp in_port)
{
	SAVE_RSTK();
	decl_gc_of_vm();
	PUSH_RSTK(in_port);
	//__protect(string_scp, s,  new_string(vm, gc, BUFLEX_INITSZ));
	__ret parser_scp pr = (parser_scp) inew_inst(gc, TYPEIX_PARSER);
	pr->buflex = (string_scp) os_thread_key_get(Bufparse_key);
	pr->linecur = 1; pr->colcur = 0;
	pr->iport = in_port;
	pr->charcur = _CHR_(' ');
	UNWIND_RSTK();
	return pr;
}



descr pars_read_lexeme(svm_t vm, __sbp(parser_scp) ppr)
{

	parser_scp pr = _DS(ppr);
start: // we return here from comment branches only
	if(pars_eof(pr)) {
		return NIHIL;
	}
	ss_clear(pr->buflex);
	pr->bits &= ~PARS_BITM_BUFREAD;
	readch_first_notwhsp(vm, pr);
	if(pars_eof(pr)) {
			return NIHIL;
	}
	ss_add(vm, NULL, &pr->buflex, pr->charcur);
	pr->bits |= PARS_BITM_BUFREAD;
	set_readp_ix(pr, 0);

	if(pars_eof(pr)) return NIHIL;
	txp_set(&pr->poslex, pr->linecur, pr->colcur, 0, 0);
	if (chrp_digit(pr->charcur)) {
		return lex_read_number(vm, ppr);
	} else {
		switch(pr->charcur) {
		/* special id's or numers */
		case _CHR_('+'):
			readch(vm, pr);
			if(chrp_num_aftersign(pr->charcur)) {
				return lex_read_number(vm, ppr);
			} else {
				return lex_read_identifier(vm, ppr);
			}
		case _CHR_('-'):
			readch(vm, pr);
			if(chrp_num_aftersign(pr->charcur)) {
				return lex_read_number(vm, ppr);
			} else {
				return lex_read_identifier(vm, ppr);
			}
		case _CHR_('.'):
			// TODO
			readch(vm, pr);
			if(chrp_eofdelim(pr->charcur)) {
				return SYMB_SGN_POINT;
			} else if(chrp_digit(pr->charcur)) {
				return lex_read_number(vm, ppr);
			}else  {
				return lex_read_identifier(vm, ppr);
			}
		case CHR_SEMICOL: // comment line
			pr->bits &= (~PARS_BITM_BUFREAD);
			skip_comment_line(vm, pr);
			goto start;

		case _CHR_('('):
			readch(vm, pr);
			txp_set_relend(&pr->poslex, 0, 1);
			return SYMB_OPEN_RO;
		case _CHR_(')'):
			readch(vm, pr);
			txp_set_relend(&pr->poslex, 0, 1);
			return SYMB_CLOSE_RO;
		case _CHR_('['):
			readch(vm, pr);
			txp_set_relend(&pr->poslex, 0, 1);
			return SYMB_OPEN_SQ;
		case _CHR_(']'):
			readch(vm, pr);
			txp_set_relend(&pr->poslex, 0, 1);
			return SYMB_CLOSE_SQ;
		case _CHR_('{'): case _CHR_('}'):
			readch(vm, pr);
			lex_error_asci(vm, ppr, EXCEP_PARSE_LEX_STOP, &pr->poslex,
					"curly brackets { } are reserved for the future");
		case CHR_DBLQUOT: // string
			return lex_read_string(vm, ppr);
   // quotation
		case CHR_QUOTE: readch(vm, pr); txp_set_relend(&pr->poslex, 0, 1); return SYMB_SGN_QUOTE;
		case CHR_QUASI: readch(vm, pr); txp_set_relend(&pr->poslex, 0, 1); return SYMB_SGN_QUASI;
		case CHR_UNQUOTE:
			readch(vm, pr);
			if(pr->charcur == _CHR_('@')) {
				readch(vm, pr);
				txp_set_relend(&pr->poslex, 0, 2);
				return SYMB_SGN_SPLICING;
			}
			txp_set_relend(&pr->poslex, 0, 1);
			return SYMB_SGN_UNQUOTE;
   // #
		case CHR_HASH: {
			readch(vm, pr);
			if(chrp_whitespace(pr->charcur)) {
				txp_set_relend(&pr->poslex, 0, 1);
				lex_error_asci(vm, ppr, EXCEP_PARSE_LEX_BADLEX,& pr->poslex,
						"bad lexeme: single # char" );
			} else if(chrp_num_afterhash(pr->charcur)) {
				return lex_read_number(vm, ppr);
			}
			switch(pr->charcur) {
			case CHR_QUOTE:
			case CHR_QUASI:
			case CHR_UNQUOTE:
				readch(vm, pr);
				txp_set_relend(&pr->poslex, 0, 2);
				lex_error_asci(vm, ppr, EXCEP_PARSE_LEX_BADLEX,& pr->poslex,
						"no support for syntax quoting #' #` #, #,@");
	// read numbers
			case _CHR_('('):
				readch(vm, pr);
				txp_set_relend(&pr->poslex, 0, 2);
				return SYMB_OPEN_VEC;
			case _CHR_('v'):
				//readch(vm, pr);
				if(readch_precise_asci(vm, pr, "u8(")) {
					txp_set_relend(&pr->poslex, 0, 4);
					readch(vm, pr);
					return SYMB_OPEN_VU;
				}
				lex_error_asci(vm, ppr, EXCEP_PARSE_LEX_BADLEX, &pr->poslex, "#vu8( expected");
			case _CHR_('t'): case _CHR_('T'):
				readch(vm, pr);
				txp_set_relend(&pr->poslex, 0, 2);
				return SYMB_T;
			case _CHR_('f'): case _CHR_('F'):
				readch(vm, pr);
				txp_set_relend(&pr->poslex, 0, 2);
				return SYMB_F;
			case CHR_SLASH:
				return lex_read_char(vm, ppr);
			case CHR_BAR_VER:
				// netsed comment
				pr->bits &= (~PARS_BITM_BUFREAD);
				skip_comment_nested(vm, ppr);
				goto start;
			case CHR_SEMICOL:
				readch(vm, pr);
				return SYMB_COMMENT;
			default: {
				bigchar ch = pr->charcur;
				readch(vm, pr);
				lex_error_f(vm,ppr, EXCEP_PARSE_LEX_BADLEX, &pr->poslex,
						"illegal char after # - '%C' codepoint %x (-1 means eof) ", ch, ch);
				}
			} // endof
		}break; // end #
		default: {
			if(chrp_initial(pr->charcur)) {
					return lex_read_identifier(vm, ppr);
			} else {
				bigchar ch = pr->charcur;
				readch(vm, pr);
				lex_error_f(vm,ppr, EXCEP_PARSE_LEX_BADLEX, &pr->poslex,
										"bad lexeme first character: '%C' codepoint: 0x%x", ch, ch);
			}
		}
		}// endof char switch
	}
	assert(0 && "never here");
}


// always read
inline bigchar __pure readch(svm_t vm, parser_scp par)
{
	if(par->charcur == EOF_CHAR) return EOF_CHAR;


	if(chrp_line_ending(par->charcur)) {
		par->colcur = 0;
		par->linecur++;

	}
	par->charcur = iport_read_char(par->iport);
	if(par->charcur == EOF_CHAR) return EOF_CHAR;
	if(par->bits & PARS_BITM_BUFREAD) {
		ss_add(vm, Gc_malloc, &par->buflex, par->charcur);
	}

	par->colcur ++;
	return par->charcur;
}
/* read prefix (or if prefix is ended - call readch) */
inline bigchar __pure readp(svm_t vm, parser_scp par)
{
	if(par->bits & PARS_BITM_LEXPREFIX) {
		bigchar c;

		if(par->ixpref >= ss_len(par->buflex) ) {
			if(readch(vm, par) == EOF_CHAR) {
				return EOF_CHAR;
			}
		}

		UChar *ucs = ss_ucs(par->buflex);
		UTF_NEXT_CHAR_UNSAFE(ucs, par->ixpref, c);
		par->charcur = c;
		return c;

	} else {
		return readch(vm, par);
	}
}

void __pure set_readp_ix(parser_scp pr, int ix)
{
	pr->bits |= PARS_BITM_LEXPREFIX;
	if(ix >= 0) {
		pr->ixpref = ix;
	} else {
		pr->ixpref = ss_len(pr->buflex) + ix;
	}
}


// read if whitespace char.
inline void __pure readch_first_notwhsp(svm_t vm, parser_scp par)
{
	until( pars_eof(par) || !chrp_whitespace(par->charcur)) {
		readch(vm, par);
	}
}

inline void __pure readch_first_delim(svm_t vm, parser_scp par)
{
	until( pars_eof(par) || chrp_delim(par->charcur)) {
			readch(vm, par);
	}
}

inline bool __pure readch_precise_asci(svm_t vm,  parser_scp pr,  const char* asci)
{
	bool r = true;
	until(pars_eof(pr) || !r || !*asci)	{
		r = (readch(vm, pr) == *(asci++));
	}
	return r;
}

inline bool __pure readp_precise_asci(svm_t vm,  parser_scp pr,  const char* asci)
{
	bool r = true;
	until(pars_eof(pr) || !*asci || !r)	{
		r = (readp(vm, pr) == *(asci++));
	}
	if(r) {
		readp(vm, pr);
	}
	return r;
}



/**
<comment> → ; 〈all subsequent characters up to a
         <line ending> or <paragraph separator>〉
         | <nested comment>
         | #; <interlexeme space> <datum>
         | #!r6rs
<nested comment> → #| <comment text>
         <comment cont>* |#
<comment text> → 〈character sequence not containing
         #| or |#〉
<comment cont> → <nested comment> <comment text>
 */
inline void skip_comment_line(svm_t vm, parser_scp par)
{
	//;....
	bigchar ch;
	do {
		ch = readch(vm, par);
	} until(pars_eof(par) || chrp_line_ending(ch) || ch == CHR_PARASEP );
	readch(vm, par);
}

inline void skip_comment_nested(svm_t vm, __sbp(parser_scp) ppr)
{
	// #| .... |#
	bigchar cur = 0, prev = EOF_CHAR;
	uint balance = 1;
	parser_scp par = _DS(ppr);
	until(pars_eof(par))
	{
		cur = readch(vm, par);
		// #|
		if(prev == CHR_HASH && cur == CHR_BAR_VER) {
			balance ++;
		}
		if(prev == CHR_BAR_VER && cur == CHR_HASH) {
			balance --;
			if(!balance) {
				break;
			}
		}
		prev = cur;
	}
	readch(vm, par);
	if(pars_eof(par) && balance) {
		txp_set_relend(&par->poslex, 0, 2);
		lex_error_asci(vm, ppr, EXCEP_PARSE_LEX_STOP,& par->poslex,
				"end of file in #| comment");
	}
}

descr lex_read_identifier(svm_t vm,  __sbp(parser_scp) ppr)
{
	/* <identifier> → <initial> <subsequent>*
         | <peculiar identifier> */

	parser_scp pr = _DS(ppr);
	bigchar ch;
	bool checkini = true;
	bool special = false;
	ch = readp(vm, pr);
	// special identifiers + - -> ...
	if(ch == '+') {
		special = true;
		if(!chrp_delim(readp(vm, pr))) {
			lex_error_asci(vm, ppr, EXCEP_PARSE_LEX_BADLEX, &pr->poslex,
					"special identifier expected: +");
		}
	} else if(ch == '-') {
		ch = readp(vm, pr);
		if(ch == '>') {
			checkini = false;
		} else if(!chrp_eofdelim(ch)) {
			lex_error_asci(vm, ppr, EXCEP_PARSE_LEX_BADLEX, &pr->poslex,
					"special identifier expected: - or ->xxx..");
		} else {
			special = true;
		}
	} else if(ch == '.') {
		special = true;
		ch = readp(vm, pr);
		if(ch == '.') {
			ch = readp(vm, pr);
			if(ch == '.') {
				ch = readp(vm, pr);
			}
		}
		if(!chrp_delim(ch)) {
				lex_error_asci(vm, ppr, EXCEP_PARSE_LEX_BADLEX, &pr->poslex,
						"special identifier expected: ...");
		}
	}

	if(!special) {
		// the rest of identifiers
		if(checkini && !chrp_initial(ch)) {
			lex_error_f(vm, ppr, EXCEP_PARSE_LEX_BADLEX, &pr->poslex,
					"wrong identifier syntax. initial char code %i ", ch);
		}
		ch = readp(vm, pr);
		until(chrp_eofdelim(ch)) {
			if(!chrp_subsequent(ch)) {
				lex_error_f(vm, ppr, EXCEP_PARSE_LEX_BADLEX, &pr->poslex,
						"wrong identifier syntax. subsequient char code %i ", ch);
			}
			ch = readp(vm, pr);
		}
	}

	return new_id_lex(vm, ppr);
}



int64_t lex_read_digits(svm_t vm, parser_scp pr, byte radix)
{

	bigchar ch = readp(vm, pr);
	int64_t result  = 0;
	pr->charcount = 0;
	until(pars_eof(pr)){
		int dig =  chr_to_digit(ch, radix);
		if(dig == -1) {
			break;
		}
		pr->charcount ++;
		result = result * radix + dig;
		ch = readp(vm, pr);
	}
	return result;
}

int ss_read_digits(svm_t vm, const UChar* buf, byte radix, uint64_t *n)
{
	*n = 0;
	int c = 0, dig;
	bigchar ch;
	while(true) {
		int pc = c;
		UTF_NEXT_CHAR_UNSAFE(buf, c, ch);
		dig = chr_to_digit(ch, radix );
		if(dig == -1) {
			c = pc;
			break;
		}
		*n = *n * radix + dig;
	}
	return c;
}

int64_t lex_read_sdigits(svm_t vm, parser_scp pr, byte radix)
{
	bigchar ch = readp(vm, pr);
	bool neg = false;
	switch(ch) {
	case '-':
		neg = true;
	case '+':
		ch = readp(vm, pr);
	}
	set_readp_ix(pr, -1);
	int64_t r = (int64_t) lex_read_digits(vm, pr, radix);
	return neg ? -r: r;
}

double lex_read_fract(svm_t vm, parser_scp pr, byte radix)
{

	bigchar ch = readp(vm, pr);
	double result  = 0.0;
	pr->charcount = 0;
	until(pars_eof(pr)){
		int dig =  chr_to_digit(ch, radix);
		if(dig == -1) {
			break;
		}
		pr->charcount ++;
		result = result  + dig * pow(radix, - pr->charcount);
		ch = readp(vm, pr);
	}
	return result;
}



double tofract(double num)
{
	return num == 0? 0 : num / pow(10, floor(log10(num)));
}

descr lex_read_number(svm_t vm, __sbp(parser_scp) ppr)
{
	/*
	<number> → <num 2> | <num 8>
	         | <num 10> | <num 16>
	<num R> → <prefix R> <complex R>

	<real R> → <sign> <ureal R>
	         | + <naninf> | - <naninf>
	<naninf> → nan.0 | inf.0
	<ureal R> → <uinteger R>
	         | <uinteger R> / <uinteger R>
	         | <decimal R> <mantissa width>
	<decimal 10> → <uinteger 10> <suffix>
	         | . <digit 10>+ <suffix>
	         | <digit 10>+ . <digit 10>* <suffix>
	         | <digit 10>+ . <suffix>
	<uinteger R> → <digit R>+
	<prefix R> → <radix R> <exactness>
	         | <exactness> <radix R>
	<suffix> → <empty>
	         | <exponent marker> <sign> <digit 10> +
	 */

	decl_gc_of_vm();
	parser_scp pr = _DS(ppr);
	bool exact   = true;
	bool wasprec = false;
	byte radix = 0;
	bigchar ch = readp(vm, pr);
	// reading prefix
	if(ch == _CHR_('#')){
		ch = readp(vm, pr);
		if(!chrp_num_afterhash(ch)) {
			lex_error_asci(vm, ppr, EXCEP_PARSE_LEX_BADLEX, &pr->poslex,
									"wrong number syntax: after #");
		}
		if(!chrp_num_precmarker(ch)) {
			radix = chr_to_radix(ch);
		} else {
			exact = ch == 'e';
			wasprec = true;
		}

		ch = readp(vm, pr);
		if(ch == '#') {
			ch = readp(vm, pr);
			if(!chrp_num_afterhash(ch)) {
				lex_error_asci(vm, ppr, EXCEP_PARSE_LEX_BADLEX, &pr->poslex,
						"wrong number syntax: after second #");
			}
			if(!chrp_num_precmarker(ch)) {
				if(radix == 0) {
					radix = chr_to_radix(ch);
				} else {
					lex_error_asci(vm, ppr, EXCEP_PARSE_LEX_BADLEX, &pr->poslex,
											"wrong number syntax: respecifying radix");
				}
			} else {
				if(wasprec) {
					lex_error_asci(vm, ppr, EXCEP_PARSE_LEX_BADLEX, &pr->poslex,
											"wrong number syntax: respecifying radix precision");
				} else {
					exact = ch == 'e';
					wasprec = true;
				}
			}
			ch = readp(vm, pr);
		}
	}
	int sign = 1;
	if(radix == 0) radix = 10;
	if(ch == '+') {
		ch = readp(vm, pr);
	} else if(ch == '-') {
		ch = readp(vm, pr);
		sign = -1;
	}

	switch(ch) {
	case _CHR_('n'):
		if(!readp_precise_asci(vm, pr, "an.0") || !chrp_delim(pr->charcur)) {
			lex_error_asci(vm, ppr, EXCEP_PARSE_LEX_BADLEX, &pr->poslex,
														"wrong number syntax: [+,-]nan.0 expected");
		}
		return new_real(gc, sign * NAN);
	case _CHR_('i'):
		if(!readp_precise_asci(vm, pr, "nf.0") || !chrp_delim(pr->charcur)) {
					lex_error_asci(vm, ppr, EXCEP_PARSE_LEX_BADLEX, &pr->poslex,
																"wrong number syntax: [+,-]inf.0 expected");
		}
		return new_real(gc, sign * INFINITY) ;
		break;
	}


	set_readp_ix(pr, pr->ixpref-1);
	int64_t mnt = lex_read_digits(vm, pr, radix);
	bool wasmnt = pr->charcount != 0 ;
	double frac = 0;
	bool wasfrac = false;
	uint64_t den = 0;
	bool wasden = false;
	int64_t ex = 0;
	bool wasex = false;




	if(pr->charcur == '.') {
		frac = lex_read_fract(vm, pr, radix);
		wasfrac = pr->charcount != 0;
		/*
		if(!wasfrac) {
			lex_error_asci(vm, ppr, EXCEP_PARSE_LEX_BADLEX, &pr->poslex,
											"wrong number syntax: digits after point are missing")
		}*/

	} else if(pr->charcur == '/') {
		if(!wasmnt) {
			lex_error_asci(vm, ppr, EXCEP_PARSE_LEX_BADLEX, &pr->poslex,
														"wrong number syntax: no numerator");
		}
		den = lex_read_digits(vm, pr, radix);
		if(pr->charcount == 0) {
					lex_error_asci(vm, ppr, EXCEP_PARSE_LEX_BADLEX, &pr->poslex,
													"wrong number syntax: denominator part expected")
		}
		wasden = true;
	}
	if(!(wasmnt || wasfrac)) {
		lex_error_asci(vm, ppr, EXCEP_PARSE_LEX_BADLEX, &pr->poslex,
										"wrong number syntax: no mantissa specified");
	}
	if(chrp_expmarker(pr->charcur)  ) {

		ex = lex_read_sdigits(vm, pr, radix);
		if(pr->charcount == 0) {
			lex_error_asci(vm, ppr, EXCEP_PARSE_LEX_BADLEX, &pr->poslex,
											"wrong number syntax: exponenta part expected")
		}
		wasex = true;
	}

	if(pr->charcur=='+' || pr->charcur == '-'|| pr->charcur == '@') {
		lex_error_asci(vm, ppr, EXCEP_PARSE_LEX_BADLEX, &pr->poslex,
													"wrong number syntax or no complex support!");
	}

	if(!chrp_eofdelim(pr->charcur)) {
		lex_error_asci(vm, ppr, EXCEP_PARSE_LEX_BADLEX, &pr->poslex,
															"wrong number syntax");
	}

	txp_set_end(&pr->poslex, pr->linecur, pr->colcur - 1);

	if(wasmnt && !wasfrac && !wasex && !wasden && exact) {
		mnt *= sign;
		return desc_from_fixnum(mnt);
	}
	double result = 0;
	if(!wasden) {
		result = (double) mnt + frac;
	} else {
		result = (double) mnt / (double) den;
	}
	result *= sign;
	result *= pow(radix, ex);
	/*
	if(exact && (result == ceil(result))) {
		long i = ceil(result);
		return desc_from_fixnum(i);
	}
	*/
	return new_real(gc, result);
}

descr lex_read_namedchar(svm_t vm, __sbp(parser_scp) ppr, const char* name, bigchar ch )
{
	if(!readp_precise_asci(vm, _DS(ppr), name) || !chrp_eofdelim(_DF(ppr,charcur))) {
		lex_error_f(vm, ppr, EXCEP_PARSE_LEX_BADLEX, &_DF(ppr,poslex),
											"char named %s expected", name);
	}
	parser_scp pr = _DS(ppr);
	txp_set_end(&_DF(ppr,poslex), pr->linecur, pr->colcur - 1);
	return desc_from_char(ch);
}

void __pure oport_print_char(svm_t vm, oport_scp op, bigchar ch)
{
	switch(ch) {
	case 0: oport_write_asci(op, "#\\nul"); return;
	case CHR_LINEFEED:oport_write_asci(op, "#\\newline"); return;
	case CHR_ALARM:oport_write_asci(op, "#\\alarm"); return;
	case CHR_BKSP:oport_write_asci(op, "#\\backspace"); return;
	case CHR_TAB:oport_write_asci(op, "#\\tab"); return;
	case CHR_VTAB:oport_write_asci(op, "#\\vtab"); return;
	case CHR_PAGE:oport_write_asci(op, "#\\page"); return;
	case CHR_RET:oport_write_asci(op, "#\\return"); return;
	case CHR_ESC:oport_write_asci(op, "#\\esc"); return;
	case CHR_SPACE:oport_write_asci(op, "#\\space"); return;
	case CHR_DEL:oport_write_asci(op, "#\\delete"); return;
	default:
		if(chrp_printable(ch)) {
			oport_write_asci(op, "#\\");
			oport_write_char(op, ch);
		} else {
			oport_write_asci(op, "#\\");
			oport_print_hex(op, ch);
		}
	}
}

descr	lex_read_char(svm_t vm,__sbp(parser_scp) ppr )
{
/*	<character> → #\<any character>
	         | #\<character name>
	         | #\x<hex scalar value> */

	parser_scp pr = _DS(ppr);
	assert(readp(vm, pr ) == '#');
	assert(readp(vm, pr ) == CHR_SLASH);

	bigchar ch = readp(vm, pr);
	bigchar ch1 = readp(vm, pr);
	txp_set_end(&pr->poslex, pr->linecur, pr->colcur - 1);
	if(chrp_eofdelim(ch1)) {
		return desc_from_char(ch);
	} else if(ch == 'x') {
		set_readp_ix(pr, -1);
		uint64_t x = lex_read_digits(vm, pr, 16);
		if(pr->charcount == 0 || !chrp_eofdelim(pr->charcur)) {
			lex_error_asci(vm, ppr, EXCEP_PARSE_LEX_BADLEX, &pr->poslex,
						"wrong hex character code syntax");
		}
		if(!U_IS_UNICODE_CHAR(x)) {
			lex_error_asci(vm, ppr, EXCEP_PARSE_LEX_BADLEX, &pr->poslex,
									"wrong unicode code point");
		}
		txp_set_end(&pr->poslex, pr->linecur, pr->colcur - 1);
		return desc_from_char(x);
	} else {
		set_readp_ix(pr, -2);
		switch(ch) {
		case 'n': {
			switch(ch1) {
			case 'u': return lex_read_namedchar(vm, ppr, "nul", 0);
			case 'e': return lex_read_namedchar(vm, ppr, "newline", CHR_LINEFEED);
			default:
				lex_error_asci(vm, ppr, EXCEP_PARSE_LEX_BADLEX, &pr->poslex,
						"#\newline or #\nul expected");
			}

		}
		case 'a': return lex_read_namedchar(vm, ppr, "alarm", CHR_ALARM );
		case 'b': return lex_read_namedchar(vm, ppr, "backspace", CHR_BKSP );
		case 't': return lex_read_namedchar(vm, ppr, "tab", CHR_TAB );
		case 'l': return lex_read_namedchar(vm, ppr, "linefeed", CHR_LINEFEED );
		case 'v': return lex_read_namedchar(vm, ppr, "vtab", CHR_VTAB );
		case 'p': return lex_read_namedchar(vm, ppr, "page", CHR_PAGE );
		case 'r': return lex_read_namedchar(vm, ppr, "return", CHR_CARRETRN );
		case 'e': return lex_read_namedchar(vm, ppr, "esc", CHR_ESC);
		case 's': return lex_read_namedchar(vm, ppr, "space", CHR_SPACE );
		case 'd': return lex_read_namedchar(vm, ppr, "delete", CHR_DEL );
		default:
			lex_error_asci(vm, ppr, EXCEP_PARSE_LEX_BADLEX, &pr->poslex,
									"named char expected: #\\<name>");
		}
	}

	assert(0 && "never here bad char");
	return 0;
}


descr lex_read_string (svm_t vm, __sbp(parser_scp) ppr)
{
	parser_scp pr = _DS(ppr);
	assert(readp(vm, pr ) == '"');
	bigchar ch, prev;
	prev = '"';
	bool closed = false;
	until(pars_eof(pr) || closed) {
		ch = readp(vm, pr);
		if(ch == CHR_DBLQUOT && prev != CHR_SLASH ) {
			closed = true;
		}
		if(ch == CHR_SLASH && prev == CHR_SLASH) {
			prev = '"';
		} else {
			prev = ch;
		}
	}
	if(!closed) {
		lex_error_asci(vm, ppr, EXCEP_PARSE_LEX_BADLEX, &pr->poslex,
						"unclosed string");
	}
	readp(vm, pr);
	txp_set_end(&pr->poslex, pr->linecur, pr->colcur - 1);
	return new_string_lex(vm, ppr);
}


descr pars_next_lex(svm_t vm, __sbp(parser_scp) ppr)
{
	SAVE_RSTK();
	_try_
		__protect(descr, l, pars_read_lexeme(vm, ppr));
		FIELD_DSET(ppr, lexeme,  l);
	_except_
	_catch_(EXCEP_PARSE_LEX_BADLEX)
		_retry_;
	_catchall_
		FIELD_DSET(ppr, lexeme, NIHIL);
		UNWIND_RSTK();
		_rethrow_;
	_end_try_
	UNWIND_RSTK();
	return _DF(ppr, lexeme);
}

descr pars_read_datum(svm_t vm, __sbp(parser_scp) ppr)
{
	SAVE_RSTK();
	(_DF(ppr, bits) &= ~PARS_BITM_ERR_LEX);
	(_DF(ppr, bits) &= ~PARS_BITM_ERR_DAT);
	__loc descr result = NIHIL;
	_try_
		result =  syn_read_datum(vm, ppr);
	_except_
	_catch3_(EXCEP_PARSE_DAT_STOP, EXCEP_PARSE_DAT, EXCEP_PARSE_LEX_STOP )
		UNWIND_RSTK();
		return NIHIL;
	_chain_try_
	UNWIND_RSTK();

	return result;
}

descr syn_read_datum(svm_t vm, __sbp(parser_scp) ppr)
{
	if(!pars_next_lex(vm, ppr)) {
		return NIHIL;
	}
	return syn_match_datum(vm, ppr);
}



// sy
descr syn_match_datum(svm_t vm, __sbp(parser_scp) ppr)
{
	decl_gc_of_vm();
	descr lexeme = _DF(ppr, lexeme);
	if(desc_is_spec(lexeme)) {
		ulong spec = desc_to_spec(lexeme);
		__loc descr q = NIHIL;
		__loc descr csort = NIHIL;
		switch(spec) {
		case desc_to_spec(SYMB_T):
		case desc_to_spec(SYMB_F):
		case desc_to_spec(SYMB_EOF):
		case desc_to_spec(SYMB_VOID):
		case desc_to_spec(SYMB_UNDEF):
			return lexeme;
		// compound datum beginnings
		case desc_to_spec(SYMB_OPEN_RO): csort = SYMB_OPEN_RO; break;
		case desc_to_spec(SYMB_OPEN_SQ): csort = SYMB_OPEN_SQ; break;
		case desc_to_spec(SYMB_OPEN_VEC): csort = SYMB_LIT_VEC; break;
		case desc_to_spec(SYMB_OPEN_VU): csort = SYMB_LIT_VU; break;
		// quotation lexemes
		case desc_to_spec(SYMB_SGN_QUOTE): q = SYMB_QUOTE; break;
		case desc_to_spec(SYMB_SGN_QUASI): q = SYMB_QUASI; break;
		case desc_to_spec(SYMB_SGN_UNQUOTE):  q = SYMB_UNQUOTE; break;
		case desc_to_spec(SYMB_SGN_SPLICING):  q = SYMB_SPLICING; break;
		case desc_to_spec(SYMB_COMMENT):
			if(!syn_read_datum(vm, ppr)) {
				lex_error_asci(vm, ppr, EXCEP_PARSE_DAT_STOP, &_DF(ppr, poslex),
													"unexpected EOF: datum expected after #;");
			}
			return syn_read_datum(vm, ppr);
		default:
			lex_error_f(vm, ppr, EXCEP_PARSE_DAT_STOP, &_DF(ppr, poslex),
					"datum expected -  \"%S\" found", ss_ucs(symbol_getname(lexeme)));
		}
		if(csort) {
			return syn_read_compound(vm, ppr, csort);
		} else if(q) {
			SAVE_RSTK();
			decl_gc_of_vm();
			__protect(descr, quoted,   syn_read_datum(vm, ppr));
			if(!quoted) {
				lex_error_asci(vm, ppr, EXCEP_PARSE_DAT_STOP, &_DF(ppr, poslex),
									"unexpected EOF: datum expected after abbrev");
			}

			__protect(descr, id,   new_id(vm, gc, q));
			__protect(txtpos_scp, ps,   syn_getpos(quoted) );
			quoted = CONSCAR(quoted);
			__ret descr r = new_scons_p(vm, gc, id, quoted , ps);
			UNWIND_RSTK();
			return r;
		}
		assert(0 && "never here syn match");
	} else {
		return lexeme;
	}
	assert(0 && "never here -endfun- syn match");
}




#define __FREADCOMP_LIST  1
#define __FREADCOMP_ROUND 2
#define __FREADCOMP_PT    4

descr syn_read_compound(svm_t vm, __sbp(parser_scp) ppr, descr sort)
{
	SAVE_RSTK();
	decl_gc_of_vm();
	__protect(scons_scp, result, NIHIL);
	__protect(scons_scp, cur, NIHIL);
	__protect(scons_scp, prev, NIHIL);
	__protect(descr, lexem, NIHIL);
	__protect(descr, q1, NIHIL);
	__protect(scons_scp, cell, NIHIL);
	byte bits = 0;
	if(sort == SYMB_LIT_VEC || sort == SYMB_LIT_VU) {
		bits |= __FREADCOMP_ROUND;
		// cur=result = new_scons(vm,gc, sort, NIHIL);
	} else {
		bits |= __FREADCOMP_LIST;
		bits |= (sort == SYMB_OPEN_RO) ? __FREADCOMP_ROUND : 0;
	}
	txtpos_t pos = _DF(ppr, poslex);
	for(;;) {
		lexem = pars_next_lex(vm, ppr);
		if(lexem == SYMB_COMMENT) {
			syn_read_datum(vm, ppr);
			lexem = pars_next_lex(vm, ppr);
		}

		if(lexem == SYMB_SGN_POINT) {
			if(!(bits & __FREADCOMP_LIST)) {
				lex_error_asci(vm, ppr, EXCEP_PARSE_DAT_STOP, &_DF(ppr, poslex),
									"\".\" illegal in vector context");
			}

			if(!result) {
				lex_error_asci(vm, ppr, EXCEP_PARSE_DAT_STOP, &_DF(ppr, poslex),
													" \".\" without left side ");
			}
			bits |= __FREADCOMP_PT;
			lexem =  syn_read_datum(vm, ppr);
			FIELD_SET(cur, cdr, lexem);

			lexem = pars_next_lex(vm, ppr);
			if(lexem !=  SYMB_CLOSE_RO && lexem != SYMB_CLOSE_SQ) {
					lex_error_asci(vm, ppr, EXCEP_PARSE_DAT_STOP, &_DF(ppr, poslex),
															" \".\" right side has more than 1 datum ");
			}
		}
		if(lexem == SYMB_CLOSE_RO || lexem == SYMB_CLOSE_SQ) {
			if(lexem == SYMB_CLOSE_RO && !( bits & __FREADCOMP_ROUND )) {
				lex_error_asci(vm, ppr, EXCEP_PARSE_DAT, &_DF(ppr, poslex),
						"] expected, ) found");
			} else if(lexem == SYMB_CLOSE_SQ && ( bits & __FREADCOMP_ROUND )) {
				lex_error_asci(vm, ppr, EXCEP_PARSE_DAT, &_DF(ppr, poslex),
									") expected, ] found");
			} else {
				if(result) {
					q1 = new_txtpos(vm);
					FIELD_SET(result, pos, q1);
					txp_set_begin(result->pos, pos.row_begin, pos.col_begin);
					txp_set_end(result->pos, _DF(ppr, poslex).row_begin, _DF(ppr,poslex).col_begin+1);
				}
				break;
			}
		} else if(lexem == NIHIL) {
			lex_error_asci(vm, ppr, EXCEP_PARSE_DAT_STOP, &_DF(ppr, poslex),
											"unexpected end of input or file ) or ] expected.");
		} else {
			lexem = syn_match_datum(vm, ppr);
			if(lexem == NIHIL) {
				lex_error_asci(vm, ppr, EXCEP_PARSE_DAT_STOP, &_DF(ppr, poslex),
						"unexpected end of input or file ) or ] expected.");
			}

			cell  = new_scons(vm, gc, lexem, NIHIL);
			if(cur) {
				prev = cur;
				FIELD_SET(cur, cdr, cell);
				cur = cur->cdr;
			} else {
				result = cur = cell;
			}
		}
	}
	if(sort == SYMB_LIT_VEC ) {
		result = (scons_scp) cl_to_vec(vm, gc, (cons_scp*)_RS(result));
	}

	if(sort == SYMB_LIT_VU ) {
		result = (scons_scp) cl_to_vu8(vm, gc, (cons_scp*) _RS(result));
		if(!result) {
			lex_error_asci(vm, ppr, PARS_DAT_ERROR, &pos,
									"components of #vu8 byte vector are integer [0...255]");
		}
	}

	UNWIND_RSTK();
	if(!result)
		return SYMB_NIL;

	if(scp_valid(result)) {
		datum_set_const(result);
	}
	return result;
}


void lexer_test(svm_t vm, __sbp(parser_scp) ppr, __sbp(oport_scp) pwp)
{
	SAVE_RSTK();
	__protect(descr, d, NIHIL);
	_try_
		while(!pars_eof(_DS(ppr))) {
			d = pars_read_lexeme(vm, ppr);
			oport_write_asci(_DS(pwp), "\n");
			oport_print_datum(vm, pwp, _RS(d));
		}
		oport_write_asci(_DS(pwp), "\n");
	_except_
	_catch_(EXCEP_PARSE_LEX_BADLEX)
		_retry_;
	_catch_(EXCEP_PARSE_LEX_STOP)
		printf("fatal lexeme");
	_catchall_
		oport_write_asci(_DS(pwp), "\nlex errors detected!\n");
		UNWIND_RSTK();
		_rethrow_;
	_end_try_

	UNWIND_RSTK();
}

/*
void syn_test(svm_t vm, __sbp(parser_scp) ppr, __sbp(oport_scp) pwp)
{
	SAVE_RSTK();
	__protect(descr, d, NIHIL);
	_try_
		while(!pars_eof(_DS(ppr))) {
			d = pars_read_datum(vm, ppr);

			oport_write_asci(_DS(pwp), "\n");
			if(!d) {
							return;
						}
			oport_print_datum(vm, pwp, _RS(d));
		}
	oport_write_asci(_DS(pwp), "\n");
	_except_
	_catchall_
		oport_write_asci(_DS(pwp), "\nsyn errors detected!\n");
		UNWIND_RSTK();
		_rethrow_;
	_end_try_

	UNWIND_RSTK();
}

*/











