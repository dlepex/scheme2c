/* charcat.c
 * musc- scheme translator & interpreter
 * author: denis.lepekhin@gmail.com
 * created on: 2009
 */

#include "charcat.h"


// Character predicates chrp_xxx
inline bool chrp_whitespace(bigchar ch)
{
	/*
	<whitespace> → <character tabulation>
	         | <linefeed> | <line tabulation> | <form feed>
	         | <carriage return> | <next line>
	         | <any character whose category is Zs, Zl, or Zp>
	*/
	return u_isWhitespace(ch) ||ch == CHR_TAB_HOR || ch == CHR_TAB_VER ||
		ch == CHR_LINEFEED || ch == CHR_CARRETRN || ch == CHR_NEXTLINE;
}

inline bool chrp_line_ending(bigchar ch)
{
	/*
		<line ending> → <linefeed> | <carriage return>
		         | <carriage return> <linefeed> | <next line>
		         | <carriage return> <next line> | <line separator>
	*/
	return ch == CHR_LINEFEED || ch == CHR_NEXTLINE || ch == CHR_LINESEP;
}


inline bool chrp_u_line_ending(UChar *uc)
{
	return chrp_line_ending(uc[0]) ||
	(uc[0] == CHR_CARRETRN &&
			(uc[1]==CHR_LINEFEED || uc[1] == CHR_NEXTLINE));
}

inline bool chrp_open_bracket(bigchar ch)
{
	return ch == CHR_OPEN_CURL || ch == CHR_OPEN_RO || ch == CHR_OPEN_SQ;
}

inline bool chrp_close_bracket(bigchar ch)
{
	return ch == CHR_CLOSE_CURL || ch == CHR_CLOSE_RO || ch == CHR_CLOSE_SQ;
}

inline bool chrp_bracket(bigchar ch)
{
	return chrp_close_bracket(ch) || chrp_open_bracket(ch);
}




inline bool chrp_delim(bigchar ch)
{	/*
	<delimiter> → ( | ) | [ | ] | " | ; | #
         | <whitespace>
   */
	return chrp_bracket(ch) || chrp_whitespace(ch) ||
		ch == CHR_DBLQUOT || ch == CHR_SEMICOL || ch == CHR_HASH;
}

inline bool chrp_rdigit(byte radix, bigchar ch)
{
	switch(radix) {
	case 2:
		return ch == _CHR_('0') || ch == _CHR_('1');
	case 8:
		return ch >= _CHR_('0') && ch <= _CHR_('7');
	case 10:
		return u_isdigit(ch);
	case 16:
		return u_isxdigit(ch);
	default:
		assert(0 && "no such radix");
	}
}

inline bool chrp_digit(bigchar ch) {return u_isdigit(ch);}

inline bool chrp_letter(bigchar ch) {
	return ( ch >= _CHR_('a') && ch <= _CHR_('z') ) ||
	 ( ch >= _CHR_('A') && ch <= _CHR_('Z') );
}


inline bool chrp_constituent(bigchar ch)
{
	/*
	<constituent> → <letter>
	         | 〈any character whose Unicode scalar value is greater than
	             127, and whose category is Lu, Ll, Lt, Lm, Lo, Mn,
	             Nl, No, Pd, Pc, Po, Sc, Sm, Sk, So, or Co〉
	             */
	uint32_t m = U_GET_GC_MASK(ch);
	return chrp_letter(ch) || (m & U_GC_L_MASK) || (m & U_GC_S_MASK)
		|| (m & U_GC_NL_MASK) || (m & U_GC_NO_MASK) ||
		(m & U_GC_PD_MASK) || (m & U_GC_PC_MASK) || (m & U_GC_PO_MASK) ||
		(m & U_GC_MN_MASK);
}

inline bool chrp_spec_initial(bigchar ch)
{

	/* <special initial> → ! | $ | % | & | * | / | : | < | =
	         | > | ? | ^ | _ | ~    */
	return ch == _CHR_('!') ||  ch == _CHR_('$') ||  ch == _CHR_('%') || ch == _CHR_('&') ||
	 ch == _CHR_('*') || ch == _CHR_('/') || ch == _CHR_(':') || ch == _CHR_('<') ||
	 ch == _CHR_('=') || ch == _CHR_('>') || ch == _CHR_('?') ||  ch == _CHR_('^') ||
	 ch == _CHR_('_') ||  ch == _CHR_('~');
}

inline bool chrp_initial(bigchar ch)
{
	// <initial> → <constituent> | <special initial>
	return chrp_constituent(ch) || chrp_spec_initial(ch);
}

inline bool chrp_spec_subsequent(bigchar ch)
{
	// <special subsequent> → + | - | . | @
	return ch == _CHR_('+') ||  ch == _CHR_('-') ||  ch == _CHR_('.') || ch == _CHR_('@');
}

inline bool chrp_subsequent(bigchar ch)
{
	/* <subsequent> → <initial> | <digit>
	         | <any character whose category is Nd, Mc, or Me>
	         | <special subsequent>  */
	uint32_t m = U_GET_GC_MASK(ch);
	return chrp_initial(ch) || chrp_digit(ch) || chrp_spec_subsequent(ch) ||
	 (m & U_GC_ND_MASK) || (m & U_GC_MC_MASK) ||  (m & U_GC_ME_MASK);
}

inline bool chrp_expmarker(bigchar ch)
{
	/*<exponent marker> → e | E | s | S | f | F
         | d | D | l | L */
	return ch == _CHR_('e') || ch == _CHR_('E') || ch == _CHR_('s') || ch == _CHR_('S')  ||
	ch == _CHR_('d') || ch == _CHR_('D')  || ch == _CHR_('f') || ch == _CHR_('F') ||
	ch == _CHR_('l') || ch == _CHR_('L');
}

inline bool chrp_num_afterhash(bigchar ch)
{
	return chrp_num_precmarker(ch) || ch == _CHR_('D') || ch == _CHR_('d') || ch == _CHR_('X') || ch == _CHR_('x') ||
	ch == _CHR_('O') || ch == _CHR_('o') || ch == _CHR_('B') || ch == _CHR_('b');
}

inline bool chrp_num_precmarker(bigchar ch)
{
	return ch == _CHR_('i') || ch == _CHR_('e');
}

inline byte chr_to_radix(bigchar ch)
{
	switch(ch)
	{
	case 'D': case 'd':
		return 10;
	case 'X': case 'x':
			return 16;
	case 'O': case 'o':
			return 8;
	case 'B': case 'b':
			return 2;
	default:
		assert("never here" && 0);
	}
}

inline bigchar chr_from_esc(bigchar ch)
{
	/**

	 #
\a : alarm, U+0007
\b : backspace, U+0008
\t : character tabulation, U+0009
\n : linefeed, U+000A
\v : line tabulation, U+000B
\f : formfeed, U+000C
\r : return, U+000D
\" : doublequote, U+0022
\\ : backslash, U+005C


	 */
	switch(ch) {
	case _CHR_('a'): return  CHR_ALARM;
	case _CHR_('b'): return  CHR_BKSP;
	case _CHR_('t'): return  CHR_TAB;
	case _CHR_('n'): return  CHR_LINEFEED;
	case _CHR_('v'): return  CHR_VTAB;
	case _CHR_('f'): return  CHR_FORMFEED;
	case _CHR_('r'): return  CHR_RET;
	case _CHR_('"'): return  CHR_DBLQUOT;
	case _CHR_('\\'): return CHR_SLASH;
	default:
		return -1;
	}
}

const char* chr_to_esc(bigchar ch)
{
	switch(ch) {
	case CHR_ALARM: return "\\a";
	case CHR_BKSP: return "\\b";
	case CHR_TAB: return "\\t";
	case CHR_LINEFEED: return "\\n";
	case CHR_VTAB: return "\\v";
	case CHR_FORMFEED: return "\\f";
	case CHR_RET: return "\\r";
	case CHR_DBLQUOT: return "\\\"";
	case CHR_SLASH: return "\\\\";
	default:
		return NULL;
	}
}

inline bool chrp_num_aftersign(bigchar ch)
{
	return chrp_digit(ch) || ch == _CHR_('.') || ch == _CHR_('n') || ch == _CHR_('i');
}
