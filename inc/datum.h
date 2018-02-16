/* datum.h Datum and tags related defs
 * musc- scheme translator & interpreter
 * author: denis.lepekhin@gmail.com
 * created on: 2009
 */

#ifndef DATUM_H_
#define DATUM_H_


#include "base.h"



struct DatumTag;
struct ArrayDatum;
struct IntegerDatum;
struct RealDatum;
struct ComplexDatum;
struct CharDatum;
struct ConsDatum;
struct DatumType;
struct StructType;
struct SyntaxIdDatum;

/** \brief descriptor (tagged pointer)
 *
 * Descriptor is a conception of a machine word.
 * It can "contain" pointer, integer, char, cpecial value(nil, \#t, \#f)
 * For instance VM stack consists of descriptors only.
 * \ingroup Types
 */
typedef void  *descr;

#define  NIHIL					((descr)0)

#define _DESC_SPEC_NIL		    0
#define _DESC_SPEC_UNDEF 		 1
#define _DESC_SPEC_T  			 2
#define _DESC_SPEC_F 			 3
#define _DESC_SPEC_VOID 		 4
#define _DESC_SPEC_EOF			 5

#define _DESC_MASK_SMALL  		_umask_(3)
#define _DESC_SHIFT_BIG        5
#define _DESC_MASK_BIG			_umask_(_DESC_SHIFT_BIG)

#define _DESC_TAG_SPEC     		6 	// ...xxx00'110
#define _DESC_TAG_LOCAL  	  	   14	// ...xxx01'110
#define _DESC_TAG_GLOBAL         30 // ...xxx11'110
#define _DESC_TAG_CHAR     		22 //   ...x10'110
#define _DESC_TAG_DATUMTAG  	   2 	//   ...xxx010
#define _DESC_TAG_SCP       	   0 	//   ...xxx000
#define _DESC_TAG_FIXNUM     	   1 	//     ...xxx1
//#define _DESC_SPEC_BOOL			32	//

#define _D_(d)  ((descr)(d))
#define _DU_(d)  ((ulong)(d))
// Special values (symbol-like)
#define SYMB_NIL				desc_from_spec(_DESC_SPEC_NIL)
#define SYMB_UNDEF			desc_from_spec(_DESC_SPEC_UNDEF)
#define SYMB_T					desc_from_spec(_DESC_SPEC_T)
#define SYMB_F					desc_from_spec(_DESC_SPEC_F)
#define SYMB_VOID				desc_from_spec(_DESC_SPEC_VOID)
#define SYMB_EOF				desc_from_spec(_DESC_SPEC_EOF)


#define _DESC_LOCAL_SHIFT _DESC_SHIFT_BIG
#define _DESC_FRAME_SHIFT 5
#define _DESC_FRAME_MASK  (_umask_(8) << _DESC_FRAME_SHIFT)
#define _DESC_LOCIX_SHIFT 13



#define desc_from_local(nfr,  ix)    _D_(  ((((nfr) <<  _DESC_FRAME_SHIFT) & _DESC_FRAME_MASK) | ((ix) << _DESC_LOCIX_SHIFT)) | _DESC_TAG_LOCAL  )
#define desc_loc_ix(d)               (_DU_(d) >> _DESC_LOCIX_SHIFT)
#define desc_loc_fr(d)               ((_DU_(d) & _DESC_FRAME_MASK) >> _DESC_FRAME_SHIFT)
#define desc_from_glob(ix)           _D_(((ix) << _DESC_SHIFT_BIG) | _DESC_TAG_GLOBAL )
#define desc_to_glob(d)                 (_DU_(d) >> _DESC_SHIFT_BIG)

#define desc_is_local(d)    ((_DU_(d) & _DESC_MASK_BIG) == _DESC_TAG_LOCAL)
#define desc_is_global(d)   ((_DU_(d) & _DESC_MASK_BIG)== _DESC_TAG_GLOBAL)
/**
 * \defgroup DescPred Descriptor predicates
 * @{
 */
#define desc_is_fixnum(d)    (_DU_(d) & _DESC_TAG_FIXNUM)
#define desc_is_scp(d)       (!(  _DU_(d) & _DESC_MASK_SMALL) )
#define scp_valid(d)			( desc_is_scp(d) && ((d) != 0) )
#define desc_is_char(d)	     ((_DU_(d) & _DESC_MASK_BIG) == _DESC_TAG_CHAR)

#define desc_is_spec(d)      ((_DU_(d) & _DESC_MASK_BIG) == _DESC_TAG_SPEC)
#define desc_is_symbol(d)		desc_is_spec(d)

#define desc_is_nil(d)		 ((_D_(d) == SYMB_NIL))
#define desc_is_t(d)		    (_D_(d) == SYMB_T)
#define desc_is_f(d)    	 (_D_(d) == SYMB_F)
#define desc_is_bool(d) 	 (desc_is_t(d) || desc_is_f(d))
#define desc_is_undef(d) 	 (_D_(d) == SYMB_UNDEF)
#define desc_is_void(d)  	 (_D_(d) == SYMB_VOID)
#define desc_is_eof(d)  	 (_D_(d) == SYMB_EOF)
#define desc_is_tag(d)  	 ((_DU_(d)&_DESC_MASK_SMALL) == _DESC_TAG_DATUMTAG)


#define desc_is_specval(d)   (desc_is_spec(d) && (desc_to_spec(d) < 10))
/*@}*/

#define desc_cond(cond)   ((cond) ? SYMB_T : SYMB_F)
/**
 * descriptor convertors (no verification is made!)
 */

#define desc_from_scp(d) 			_D_(d)
#define desc_to_scp(v)  			((datum_scp)(v))
#define desc_from_fixnum(v) 		_D_(((long)(v)<<1) | _DESC_TAG_FIXNUM)
#define desc_to_fixnum(d)   		((long)(( ((long)(d))>>1)))
#define desc_from_char(v)			((descr)(   (((long)(v))<<_DESC_SHIFT_BIG ) | _DESC_TAG_CHAR ))
#define desc_to_char(d)				((UChar32)(((long)(d))>>_DESC_SHIFT_BIG))
#define desc_from_spec(ix) 		((descr) ((_DU_(ix)<<_DESC_SHIFT_BIG) | _DESC_TAG_SPEC))
#define desc_to_spec(d) 		    ((ulong)(_DU_(d) >> _DESC_SHIFT_BIG))
//#define desc_to_spec(d)          ((uint) (_DU_(d) >> _DESC_SHIFT_BIG))
/**
 * \brief Scheme Cell Pointer
 * \ingroup Types
 * SCP - is \e descriptor ::descr which is pointer
 * to the location in a heap. Location contains scheme
 * object - number, vector, string or other structure
 * (cons cell, port, etc.)
 * Heap must aligned to 8 bytes boundary at least.
 */
typedef void         *scp;  // scm which is Scheme Cell Pointer
typedef struct Datum *datum_t;
/**
 * each location has TAG of type dtag
 */
typedef uint64_t     tag_t;
#define _T_(literal)       ((tag_t)(literal))
typedef struct DatumType       *type_scp;








/**
 * Special kinds of SCP:
 */
typedef struct ArrayDatum   			*vec_scp; // scp which refers to vector-cell
typedef struct ArrayStackDatum   			*astk_scp; // scp which refers to vector-cell
typedef struct ArrayDatum   			*buf_scp;
typedef struct ArrayListDatum   		*arlist_scp;
typedef struct StringDatum  			*string_scp;

typedef struct FixnumDatum  			*fixnum_scp;
typedef struct RealDatum   			*real_scp;
typedef struct ComplexDatum 			*complex_scp;
typedef struct CharDatum    			*char_scp;
typedef struct ConsDatum    			*cons_scp;
typedef struct ConsDatum    			*cl_scp;
typedef struct Datum 					*datum_scp;
typedef struct SymAssocDatum        *symasc_scp;
typedef struct BoxDatum             *box_scp;
typedef cons_scp                     env_scp;

typedef descr                       symbol_d;  //symbol datum
// extern tables;
extern descr   * __Glob_table__;
extern symasc_scp * __Sym_table__;


#define _MEMBER_ALIGN 8 //TODO: platform

#define __datum_tail(aseltype, datum, typenim)      ((aseltype*)(((char*)(datum)) + sizeof(typenim)))

struct Datum {
	tag_t tag;
};

struct StructDatum {
	tag_t tag;
};

struct ArrayDatum {
	tag_t tag;
	ulong count;
};

struct ArrayStackDatum {
	tag_t tag;
	ulong count;
	ulong ptr; // slot count
};

#define OFFSET_ARRAY sizeof(struct ArrayDatum)
#define OFFSET_DATUM sizeof(struct Datum)

#define vec_tail(v)  __datum_tail(descr, v, struct ArrayDatum)
#define buf_tail(v)	 __datum_tail(byte,  v, struct ArrayDatum)
#define datum_tail(v) __datum_tail(byte,  v, struct Datum)
#define ucs_tail(v)	 __datum_tail(UChar,  v, struct ArrayDatum)
#define ar_count(a)  (((struct ArrayDatum*)(a))->count)

#define buf_len(b)  ar_count(b)
#define vec_len(b)  ar_count(b)

#define astk_tail(a)    __datum_tail(descr, a, struct ArrayStackDatum)
#define astk_capac(a)   ar_count(a)
#define astk_size(a)    ((astk_scp)(a))->ptr
#define astk_top(a)     (astk_tail(a)[astk_size(a)-1])
#define astkp_empty(a)  (astk_size(a) == 0)

#define check_array_index(ar, index)  \
	_mthrow_assert_(((index) >= 0 && (index) < ar_count(ar)), \
			EXCEP_INDEX_OUT_OF_RANGE, "index out of range")

// array-struct slot-offset = sizeof(struct ArrayListDatum) slot-count=.count
struct ArrayListDatum {
	tag_t tag;
	vec_scp data;
	ulong index;
};

arlist_scp raw_new_arlist(ulong count);
#define arlist_tail(al) 	vec_tail((al)->data)
#define arlist_count(al)	vec_len((al)->data)
#define arlist_elems(al) 	arlist_tail(al)
#define arlist_index(al)	(al)->index
#define arlist_clear(al)   (al)->index = 0
#define arlist_is_empty(al)  ((al)->index == 0)
#define arlist_pop(al)       arlist_tail(al)[--(al)->index]
#define arlist_is_full(al) 	(arlist_index(al) >= arlist_count(al))




typedef struct {
	int line1, line2, col1, col2;
}__attribute__((packed)) textpos_t ;


#define field_offs_scp(x_scp_type, field)  ((ulong) (void*) ( &(((x_scp_type)0)->field)))

#define TYPEFLAG_CONS   1
#define TYPEFLAG_SYNTAX 2


struct DatumType {
	tag_t tag;

	tag_t itag;
	ulong category;
	ulong szobj, szelem;
	ulong slot_count, slot_offs, slot_count_offs;
	uint tix;
	const char *asci;

	descr finalizer;
	descr printer;
};
#define _SLOTC_TYPE 2

#define type_slot_count(tp)  (((type_scp)(tp))->slot_count)
#define array_slot_count(tp,dm)  (*((ulong*)(  ((char*)dm) + (((type_scp)(tp))->slot_count_offs)) ))
#define type_slot_offset(tp) (((type_scp)(tp))->slot_offs)
#define type_index(tp)       (((type_scp)(tp))->tix)
#define type_getbits(tp)         ((tp)->category)
#define type_setfin(tp, fi)    	((tp)->finalizer = (descr) (fi))
#define type_setprint(tp, pri)   ((tp)->printer = (descr) (pri))

struct SymAssocDatum {
	tag_t tag;
	descr globix; // global index in form of fixnum, initial == NIHIL.
	string_scp name;
};
#define _SLOTC_SYMBOL 1

#define DTAG_SIZE  	(sizeof(struct DatumTag))
struct RealDatum {
	tag_t tag;
	double value;
};

struct ComplexDatum {
	tag_t tag;
	complex_t value;
};


struct StringDatum {
	tag_t tag;
	buf_scp buf;
	ulong   length;
	int     hash;
};

#define _SLOTC_STRING 1

#define ss_clr_hash(s)  ((s)->hash = NO_HASH)

#define HASH_MASK 0x3FFFFFFF
#define NO_HASH -1
#define HASH_S_MULT 37

// id est pair - always 2 slots
struct ConsDatum {
	tag_t tag;
	descr car;
	descr cdr;
};


#define cl_end(p)  (!(p) || (p) == SYMB_NIL)


#define CAR(l)  (((cons_scp)(l))->car)
#define CDR(l)  (((cons_scp)(l))->cdr)


// always 1 slot
struct BoxDatum {
	tag_t tag;
	descr boxed;
};






#define box_set(b, val) FIELD_SET(((box_scp)b), boxed, val)



#define unbox(box)   (((box_scp)(box))->boxed)


#define unboxif(mbbox) ((datum_is_box(mbbox)) ? unbox(mbbox) : (mbbox))



/**
 * Datum tag layout
 */

#define TAG_MASK_DATUM_PART   _lmask_(24) //first 24 bits
#define TAG_BITMASK_NUMBER        _lbit_(3)
#define TAG_BITMASK_CONST		 	_lbit_(4)
#define TAG_BITMASK_NONTERM       _lbit_(5)

#define TAG_TYPE_SHIFT        6
#define TAG_MASK_TYPE        _llshft_(_lmask_(18), TAG_TYPE_SHIFT )


#define tag_make(tag)          (1  | (tag))
#define tag_mask_type(tag)        ((tag) &  TAG_MASK_TYPE)
#define tag_mask_datum(tag)       ((tag) & TAG_MASK_DATUM_PART)
#define tag_is_number(tag) 		 ((tag) & TAG_BITMASK_NUMBER)
#define tag_is_const(tag)		    ((tag) & TAG_BITMASK_CONST)
#define tag_is_nonterm(tag)       ((tag) & TAG_BITMASK_NONTERM)

#define datum_set_const(dm)    datum_set_tag(dm, (datum_get_tag(dm) | TAG_BITMASK_CONST) )

#define tag_set_term(tag)         ((tag) & ~ TAG_BITMASK_NONTERM)
#define datum_set_term(d)		 (datum_get_tag(d) = tag_set_term(datum_get_tag(d)))

#define tag_get_typeix(tag)    ((uint) _getmasked_(tag,TAG_MASK_TYPE,TAG_TYPE_SHIFT))
#define tag_from_typeix(tix)    tag_make(_llshft_(tix, TAG_TYPE_SHIFT))
// main tag constructor
#define tag_from_tixcateg(tix, cat)   (tag_from_typeix(tix) | (cat))

#define datum_get_tag(d)					(((datum_scp)(d))->tag)
#define datum_set_tag(dm, t)           ( datum_get_tag(dm) = (t))
#define datum_get_typeix(d)					tag_get_typeix(datum_get_tag(d))



// basic type indices
#define TYPEIX_NUM_REAL 			1
#define TYPEIX_NUM_COMPLEX			2
#define TYPEIX_NUM_RATIONAL		3
#define TYPEIX_NUM_BIGNUM       	4
#define TYPEIX_VECTOR				10
#define TYPEIX_BUFFER				11
#define TYPEIX_ARRAY_LIST			12
#define TYPEIX_CONS					13
#define TYPEIX_SYNTAX_CONS 		14
#define TYPEIX_SYNTAX_IDENT 		15
#define TYPEIX_TEXTPOS           16
#define TYPEIX_STRING           	17
#define TYPEIX_HASHTABLE        	18
#define TYPEIX_HASHNODE         	19
#define TYPEIX_TYPE	            20
#define TYPEIX_SYMASSOC          21
#define TYPEIX_OPORT             22
#define TYPEIX_IPORT             23
#define TYPEIX_PARSER            24
#define TYPEIX_FRAME             25
#define TYPEIX_CODEBLOCK         26
#define TYPEIX_CLOSURE           27
#define TYPEIX_WEAKBOX           28
#define TYPEIX_ENVIRONMENT       29
#define TYPEIX_TRANSFORMER       30
#define TYPEIX_BOX       			31
#define TYPEIX_ARRAYSTACK        32
#define TYPEIX_CONTINUATION      33

#define TYPEIX_VMI_VAR           	40
#define TYPEIX_VMI_LIT           	41
#define TYPEIX_VMI_VAR_LIT           42
#define TYPEIX_VMI_VAR_VAR           43
#define TYPEIX_VMI_MARK          	44
#define TYPEIX_VMI_PREP          	45


#define datum_is_instof_ix(dm, tix )         (scp_valid(dm) && (tag_mask_type(datum_get_tag(dm)) == _llshft_(tix, TAG_TYPE_SHIFT)))

#define datum_is_number(datum)					(scp_valid(datum) && ( tag_is_number(datum_get_tag(datum))))
#define datum_is_vector(datum)					datum_is_instof_ix(datum, TYPEIX_VECTOR)
#define datum_is_buffer(datum)					datum_is_instof_ix(datum, TYPEIX_BUFFER)
#define datum_is_string(datum)					datum_is_instof_ix(datum, TYPEIX_STRING)
#define datum_is_real(datum)						datum_is_instof_ix(datum, TYPEIX_NUM_REAL)
#define datum_is_complex(datum)				 	datum_is_instof_ix(datum, TYPEIX_NUM_COMPLEX)
#define datum_is_bignum(datum)					datum_is_instof_ix(datum, TYPEIX_NUM_BIGNUM)
#define datum_is_rational(datum)				   datum_is_instof_ix(datum, TYPEIX_NUM_RATIONAL)
#define datum_is_symbol(d)                   (desc_is_symbol(d)  ||  datum_is_instof_ix(d, TYPEIX_SYMASSOC))
#define datum_is_box(datum)						(datum_is_instof_ix(datum, TYPEIX_WEAKBOX) || datum_is_instof_ix(datum, TYPEIX_BOX))

#define datum_is_carcdr(datum)				(scp_valid(datum) && type_is_carcdr(datum_get_type(datum)))
#define datum_is_const(datum)					(scp_valid(datum) &&(tag_is_const(datum_get_tag(datum)) ))
#define datum_is_mutable(dm)              (scp_valid(dm) &&(!datum_is_const(dm) ))
#define datum_is_nonterm(datum)           (scp_valid(datum) && tag_is_nonterm(datum_get_tag(datum)))
#define datum_is_weakcont(datum)          (scp_valid(datum) && type_is_weakcont(datum_get_type(datum)))
#define datum_is_syntax(datum)            (scp_valid(datum) && type_is_syntax(datum_get_type(datum)))

#define datum_is_cl(datum )    ((datum) == SYMB_NIL || datum_is_carcdr(datum))


#define TYPE_CATEGORY_NUMBER 			 _lbit_(0)
#define TYPE_CATEGORY_CARCDR	   	 _lbit_(1)
#define TYPE_CATEGORY_SYNTAX			 _lbit_(2)
#define TYPE_CATEGORY_ARRAY	    	 _lbit_(3)
#define TYPE_CATEGORY_CONST          _lbit_(4)
#define TYPE_CATEGORY_NONTERM        _lbit_(5)
#define TYPE_CATEGORY_WEAKCONT       _lbit_(6) // all slot are weak refs.

#define type_is_number(t)		(type_getbits(t)  & TYPE_CATEGORY_NUMBER)
#define type_is_carcdr(t)			(type_getbits(t)  & TYPE_CATEGORY_CARCDR)
#define type_is_syntax(t)		(type_getbits(t)  & TYPE_CATEGORY_SYNTAX)
#define type_is_array(t)		(type_getbits(t)  & TYPE_CATEGORY_ARRAY)
#define type_is_nonterm(t)  	(type_getbits(t)  & TYPE_CATEGORY_NONTERM)
#define type_is_weakcont(t) 	(type_getbits(t)  & TYPE_CATEGORY_WEAKCONT)

inline void type_init(type_scp t);
#define type_get_inst_tag(t) ((t)->itag)




inline ulong type_get_size(type_scp type, ulong acount);
inline void type_prepare_datum(type_scp type, datum_scp datum, ulong vcount);
inline ulong datum_get_size(datum_scp datum);

#define datum_aligned_sz(dm)  align_8(datum_get_size(dm))

#define type_aligned_sz(tp, vcount)   align_8(type_get_size(tp, vcount))
#define datum_get_type(dm)  (type_from_ix(datum_get_typeix(dm)))
#define type_from_ix(tix)   ((type_scp)(__Glob_table__[tix]))

#define datum_size(dm)  datum_get_size((datum_scp)dm)



#define ss_len(str) (((string_scp)(str))->length)
#define ss_capac(str) ( ((str)->buf->count >> 1) - 1) // capac is max uclen possible
#define ss_ucs(str)     ucs_tail((str)->buf)


// slots
#define datum_slots(tp, dm)    ((descr*) ( ((byte*)(dm))   +  type_slot_offset(tp) )  )
#define datum_slot_count(tp, dm)   (type_slot_count(tp) + (type_is_array(tp) ? array_slot_count(tp, dm) : 0   ))
#define datum_slot_full_count(tp, dm)   (type_slot_count(tp) + (type_is_array(tp) ? vec_len(dm) : 0   ))
inline void datum_get_slots(datum_scp dm, descr **from, descr **lim);
int descr_hash_code(descr d);

// datum must be nonterminal!
#define SLOT_FOREACH(dm) { \
	descr *pslot, *__psf_end; datum_get_slots( (datum_scp)(dm), &pslot, &__psf_end); \
	for(;pslot != __psf_end; pslot ++) { descr slot = *pslot;


#define END_SLOT_FOREACH }}



#endif /* DATUM_H_ */
