/* parse.h
 * musc- scheme translator & interpreter
 * author: denis.lepekhin@gmail.com
 * created on: 2009
 */

#ifndef PARSE_H_
#define PARSE_H_

#include "ioport.h"


#define EXCEP_PARSE_LEX_STOP         950
#define EXCEP_PARSE_LEX_BADLEX       951

#define EXCEP_PARSE_DAT              952
#define EXCEP_PARSE_DAT_STOP         953



#define PARS_LEX_WARNING 0
#define PARS_DAT_ERROR   -1
#define BUFLEX_INITSZ 2048

typedef struct TextPosDatum         *txtpos_scp;
typedef struct ConsSyntaxDatum    	*scons_scp;
typedef struct IdentifierDatum      *ident_scp;
typedef struct ParserDatum          *parser_scp;

typedef struct TextPosDatum {
	tag_t tag;
	uint row_begin,  col_begin;
	uint row_end,   col_end;
} __attribute__((packed)) txtpos_t;


// nodes of syntactic trees
struct ConsSyntaxDatum {
	tag_t tag;

	descr car;
	descr cdr;
	txtpos_scp pos;
};
#define _SLOTC_SYNTAX_PAIR 3

struct IdentifierDatum {
	tag_t tag;
	txtpos_scp pos;
	descr symbol;     // symbol
	descr var;        // var;

};
#define _SLOTC_IDENTIFIER 3

#define PARS_BITM_ERR_LEX  _ubit_(0)
#define PARS_BITM_ERR_DAT  _ubit_(1)
#define PARS_BITM_BUFREAD  _ubit_(2)
#define PARS_BITM_LEXPREFIX _ubit_(3)
#define PARS_BITM_ALWAYSFATAL _ubit_(4)

#define idp_bound(id)       (id_var(id) != SYMB_UNDEF)
#define id_bind(id, _var)     (id_var(id) = _var)
#define datum_is_id(dm)          (datum_is_instof_ix(dm, TYPEIX_SYNTAX_IDENT))

#define id_symbol(dm)            (((ident_scp)(dm))->symbol)

#define id_var(id)               (((ident_scp)(id))->var)

#define id_pos(id)                (((ident_scp)(id))->pos)

struct ParserDatum {
	tag_t tag;
	struct TextPosDatum poslex; // current lexeme pos
	uint linecur, colcur;
	ulong bits;
	int  ixpref;
	int  charcount;
	bigchar charcur;

	iport_scp  iport;
	string_scp buflex;
	descr      lexeme;
};
#define _SLOTC_PARSER_SCM 3

#define pars_eof(p)    ((p)->charcur == EOF_CHAR)
#define pars_set_always_fatal(p) ((p)->bits |= PARS_BITM_ALWAYSFATAL)
#define parsp_error(p)   ((p)->bits & (PARS_BITM_ERR_DAT | PARS_BITM_ERR_LEX))

scons_scp new_scons_p(svm_t vm, gc_t gc, __arg descr car, __arg descr cdr, __arg txtpos_scp pos);
ident_scp new_id(svm_t vm, gc_t gc, __arg symbol_d sym);

void parser_init();
void lexer_test(svm_t vm, __sbp(parser_scp) ppr, __sbp(oport_scp) pwp);
void syn_test(svm_t vm, __sbp(parser_scp) ppr, __sbp(oport_scp) pwp);

parser_scp new_parser(svm_t vm,  __arg iport_scp in_port);
descr pars_read_lexeme(svm_t vm, __sbp(parser_scp) pars);
descr pars_read_datum(svm_t vm, __sbp(parser_scp) ppr);
inline txtpos_scp syn_getpos(descr syntaxobj);





#endif /* PARSE_H_ */
