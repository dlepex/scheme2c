/* charcat.h
 * musc- scheme translator & interpreter
 * author: denis.lepekhin@gmail.com
 * created on: 2009
 */

#ifndef CHARCAT_H_
#define CHARCAT_H_

#include "base.h"
#include <uchar.h>

// char categorization. see base.h
#define _CHR_(x)       ((bigchar)(x))
#define CHR_TAB_HOR  	_CHR_(0x0009)
#define CHR_TAB_VER  	_CHR_(0x000B)
#define CHR_LINEFEED		_CHR_(0x000A)
#define CHR_FORMFEED		_CHR_(0x000C)
#define CHR_CARRETRN		_CHR_(0x000D)
#define CHR_FILESEP		_CHR_(0x001C)
#define CHR_GROUPSEP		_CHR_(0x001D)
#define CHR_RECSEP		_CHR_(0x001E)
#define CHR_UNITSEP		_CHR_(0x001F)
#define CHR_NEXTLINE		_CHR_(0x0085)
#define CHR_LINESEP     _CHR_(0x2028)
#define CHR_PARASEP     _CHR_(0x2029)

#define CHR_OPEN_RO     _CHR_('(')
#define CHR_CLOSE_RO    _CHR_(')')
#define CHR_OPEN_SQ     _CHR_('[')
#define CHR_CLOSE_SQ    _CHR_(']')
#define CHR_OPEN_CURL     _CHR_('{')
#define CHR_CLOSE_CURL    _CHR_('}')
#define CHR_POINT         _CHR_('.')
#define CHR_COMMA         _CHR_(',')
#define CHR_QUASI         _CHR_('`')
#define CHR_UNQUOTE        CHR_COMMA
#define CHR_SEMICOL		  _CHR_(';')
#define CHR_HASH          _CHR_('#')
#define CHR_DBLQUOT       _CHR_('"')
#define CHR_QUOTE         _CHR_('\'')



#define CHR_BAR_VER       _CHR_('|')
#define CHR_UNDERSCR      _CHR_('_')
#define CHR_SLASH         _CHR_('\\')


#define CHR_ALARM				_CHR_(0x7)
#define CHR_BKSP				_CHR_(0x8)
#define CHR_VTAB				CHR_TAB_VER
#define CHR_TAB				CHR_TAB_HOR
#define CHR_PAGE				CHR_FORMFEED
#define CHR_RET			CHR_CARRETRN
#define CHR_ESC				_CHR_(0x1B)
#define CHR_SPACE				_CHR_(0x20)
#define CHR_DEL				_CHR_(0x7F)




/*
#define CHR_         _CHR_('')
#define CHR_         _CHR_('')
#define CHR_         _CHR_('')
*/

inline byte chr_to_radix(bigchar ch);
#define chr_to_digit(ch, radix)              u_digit(ch, radix)
inline bigchar chr_from_esc(bigchar ch);
const char* chr_to_esc(bigchar ch);

// Character predicates chrp_xxx
inline bool chrp_whitespace(bigchar ch);
inline bool chrp_line_ending(bigchar ch);
inline bool chrp_u_line_ending(UChar *uc);
inline bool chrp_open_bracket(bigchar ch);
inline bool chrp_close_bracket(bigchar ch);
inline bool chrp_bracket(bigchar ch);
inline bool chrp_delim(bigchar ch);
inline bool chrp_rdigit(byte radix, bigchar ch);
inline bool chrp_digit(bigchar ch);
inline bool chrp_letter(bigchar ch);
inline bool chrp_constituent(bigchar ch);
inline bool chrp_spec_initial(bigchar ch);
inline bool chrp_initial(bigchar ch);
inline bool chrp_subsequent(bigchar ch);
inline bool chrp_expmarker(bigchar ch);
inline bool chrp_num_aftersign(bigchar ch);
inline bool chrp_num_precmarker(bigchar ch);
inline bool chrp_num_afterhash(bigchar ch);
#define chrp_printable(ch)  u_isprint(ch)

#endif /* CHARCAT_H_ */
