
#include <assert.h>
#include "datum.h"
#include "opcore.h"
#include "sysexcep.h"
#include "unittest.h"
#include <string.h>
#include "loggers.h"
#include <malloc.h>


/**
 * \brief Get datum size by tag (may use tag-cache)
 *
 * Is more efficient then ::tag_calc_size(), because of
 * using the cached size in a \a tag
 */
inline void __pure datum_clr_slots(type_scp tp, datum_scp dm)
{
	assert(tp->tix == datum_get_typeix(dm));
	memset(datum_slots(tp, dm), 0, datum_slot_full_count(tp, dm) * sizeof(descr) );
}

// called by GC
inline void __pure type_prepare_datum(type_scp t, datum_scp datum, ulong vcount)
{
	datum->tag = type_get_inst_tag(t);
	// logf_debug(Sys_log, "tpdat type %u", datum_get_typeix(datum));
	if(type_is_array(t)) {
		((vec_scp)datum)->count = vcount;
	}

	if(datum_is_nonterm(datum)) {
		datum_clr_slots(t, datum);
	}
}

inline ulong __pure type_get_size(type_scp type, ulong vcount) {
	return type->szobj + ( type_is_array(type)  ? (vcount * type->szelem) : 0);
}




inline ulong __pure datum_get_size(datum_scp dm)
{
	type_scp t = datum_get_type(dm);
	return type_get_size(t, type_is_array(t) ? ar_count(dm) : 0);
}


inline void __pure type_init(type_scp t)
{
	tag_t r = tag_from_typeix(t->tix);
	ulong cat = t->category;
	if(cat & TYPE_CATEGORY_CONST)   r |= TAG_BITMASK_CONST;
	if(cat & TYPE_CATEGORY_NONTERM) r |= TAG_BITMASK_NONTERM;
	if(cat & TYPE_CATEGORY_NUMBER)  r |= TAG_BITMASK_NUMBER;
	t->itag = r;

}

inline void __pure datum_get_slots(datum_scp dm, descr **from, descr **lim) {
	assert(datum_is_nonterm(dm) && "datum with slots is nonterm");
	type_scp t = datum_get_type(dm);
	*from = datum_slots(t, dm);
	*lim = *from + datum_slot_count(t, dm);
}


int __pure buf_hash_code(buf_scp b, int prev, ulong n)
{
	ulong count = min(n, ar_count(b));
	if(count == 0) return prev;
	unsigned short *s = (unsigned short*) buf_tail(b);
	if(count & 1) {
		prev += buf_tail(b)[0];
		s = (unsigned short*) (buf_tail(b) + 1);
	}
	ulong l = count >> 1;
	for(ulong i = 0; i < l; i++){
		prev = (prev * HASH_S_MULT + s[i]) & HASH_MASK;
	}
	return prev;
}


int __pure ss_hash_code(string_scp str)
{
	if(str->hash == NO_HASH) {
		str->hash = buf_hash_code(str->buf, 0, 2 * str->length);
	}
	return str->hash;
}



void __pure ss_clear(string_scp str)
{
	assert(datum_is_mutable(str));
	ss_ucs(str)[0] = 0;
	str->hash = NO_HASH;
	str->length = 0;
}

void ss_enlarge(svm_t vm, gc_t gc, __sbp(string_scp) ps, ulong ncap)
{
	SAVE_RSTK();
	ulong oc = ss_capac(_DS(ps));
	assert(oc < ncap);
	__loc buf_scp b = new_buf(gc, ncap);
	string_scp s = _DS(ps);
	memcpy(buf_tail(b), ss_ucs(s), ss_len(s)*2 + 1);
	del_inst(gc, s->buf );
	FIELD_DSET(ps, buf, b);
	UNWIND_RSTK();
}

void ss_add(svm_t vm, gc_t gc,  __sbp(string_scp) pstr, bigchar ch)
{

	string_scp str = _DS(pstr);
	str->hash = NO_HASH;
	assert(datum_is_mutable(str));
	ulong k = ss_capac(str);
	if(str->length >= k) {
		assert(gc != NULL);
		ss_enlarge(vm, gc, pstr,  (k + 8)*2);
	}
	str = _DS(pstr);
	UChar *ucs = ss_ucs(str);
	UTF_APPEND_CHAR_UNSAFE(ucs, str->length, ch );
	ucs[str->length] = 0;
}

void ss_dec(string_scp str, ulong dec)
{
	assert(datum_is_mutable(str));
	assert(dec <= ss_len(str));
	str->length -= dec;
	str->hash = NO_HASH;
	ss_ucs(str)[str->length] = 0;
}

bool __pure ss_eq(string_scp str1, string_scp str2)
{
	return (str1->length == str2->length) &&
	 imply(str1->hash != NO_HASH &&  str2->hash != NO_HASH, str1->hash == str2->hash)  &&
	u_strcmp(ss_ucs(str1), ss_ucs(str2)) == 0;
}

inline buf_scp new_buf(gc_t gc, ulong sz)
{
	return (buf_scp) inew_array(gc, TYPEIX_BUFFER, sz);
}

string_scp new_string(svm_t vm, gc_t gc, ulong uclen)
{
	SAVE_RSTK();
	__loc buf_scp b =  new_buf(gc, 2*(uclen+1));
	PUSH_RSTK(b);
	string_scp scp = (string_scp) inew_inst(gc, TYPEIX_STRING);
	scp->buf = b;
	ss_ucs(scp)[0] = 0;
	scp->hash = NO_HASH;
	scp->length = 0;
	UNWIND_RSTK();
	return scp;
}

string_scp new_string_const(svm_t vm, gc_t gc,__sbp(string_scp) ps)
{
	string_scp s = _DS(ps);
	if(datum_is_const(s)) {
		return s;
	} else {
		__ret string_scp r = new_string(vm, gc, ss_len(s));
		s = _DS(ps);
		ss_set_u(r, ss_ucs(s));
		r->hash = s->hash;
		datum_set_const(r);
		return r;
	}
}



string_scp new_string_asci(svm_t vm, gc_t gc, const char *asci)
{
	assert(asci != 0);
	ulong clen = (ulong)strlen(asci);
	string_scp scp = new_string(vm, gc, clen );
	return ss_setn_asci(scp, clen, asci);
}

string_scp new_string_uni(svm_t vm, gc_t gc, const UChar* ustr)
{
	ulong len = u_strlen(ustr);
	string_scp scp = new_string(vm, gc, len);
	ss_setn_u(scp, len,  ustr);
	return scp;
}

/** n <= strlen (asci) */
string_scp __pure ss_setn_asci(string_scp s, ulong n, const char* asci)
{
	assert(datum_is_mutable(s));
	s->hash = NO_HASH;
	s->length = min(n, ss_capac(s));
	u_charsToUChars(asci, ss_ucs(s),s->length);
	ss_ucs(s)[s->length]=0;
	return s;
}

string_scp __pure ss_setn_u(string_scp s, ulong n, const UChar* uni)
{
	assert(datum_is_mutable(s));
	s->hash = NO_HASH;
	s->length = min(n, ss_capac(s));
	u_strncpy(ss_ucs(s), uni, s->length);
	ss_ucs(s)[s->length] = 0;
	return s;
}

string_scp __pure ss_set_u(string_scp s, const UChar* uni)
{
	assert(datum_is_mutable(s));
	return ss_setn_u(s, u_strlen(uni), uni );
}

string_scp __pure ss_set_asci(string_scp s, const char* asci)
{
	assert(datum_is_mutable(s));
	return ss_setn_asci(s, strlen(asci), asci );
}

void ss_get_asci(string_scp s, char* buf, int bufsz) {
	UErrorCode ec = U_ZERO_ERROR;
	int sz = 0;
	u_strToUTF8(buf, bufsz, &sz, ss_ucs(s), ss_len(s), &ec);
}





real_scp new_real(gc_t gc, double d)
{
	real_scp r = (real_scp) inew_inst(gc, TYPEIX_NUM_REAL);
	r->value = d;
	return r;
}

cons_scp new_cons(svm_t vm, gc_t gc,__arg descr car, __arg descr cdr)
{
	SAVE_RSTK();
	PUSH_RSTK(car);
	PUSH_RSTK(cdr);
	cons_scp c = (cons_scp) inew_inst(gc, TYPEIX_CONS);
	c->car = car;
	c->cdr = cdr;
	UNWIND_RSTK();
	return c;
}

box_scp new_box(svm_t vm, gc_t gc, __arg descr boxed)
{
	SAVE_RSTK();
	PUSH_RSTK(boxed);
	__ret box_scp r = (box_scp) inew_inst(gc, TYPEIX_BOX);
	r->boxed = boxed;
	UNWIND_RSTK();
	return r;
}

cons_scp new_cons2(svm_t vm, gc_t gc,__arg descr car1, __arg descr car2, __arg descr cdr2)
{
	SAVE_RSTK();
	PUSH_RSTK(car1);
	PUSH_RSTK(cdr2);
	PUSH_RSTK(cdr2);
	__protect(cons_scp, cdr1, (cons_scp)inew_inst(gc, TYPEIX_CONS));
	cdr1->car = car2; cdr1->cdr = cdr2;
	__ret cons_scp c = (cons_scp) inew_inst(gc, TYPEIX_CONS);
	c->car = car1;
	c->cdr = cdr1;
	UNWIND_RSTK();
	return c;
}

inline vec_scp new_vec(gc_t gc, ulong count)
{
	return (vec_scp) inew_array(gc, TYPEIX_VECTOR, count);
}

vec_scp new_vec_gbox(svm_t vm, gc_t gc, ulong count, uint box_tix )
{
	SAVE_RSTK();
	__protect(vec_scp, v, new_vec(gc, count));
	__protect(descr, d, NIHIL);
	for(ulong i = 0; i < count; i++) {
		d =  inew_inst(gc, box_tix);
		VEC_SET(v, i, d );
	}
	UNWIND_RSTK();
	return v;
}

vec_scp new_vec_box(svm_t vm, gc_t gc, ulong count )
{
	return new_vec_gbox(vm, gc, count, TYPEIX_BOX);
}

vec_scp new_vec_wbox(svm_t vm, gc_t gc, ulong count)
{
	return new_vec_gbox(vm, gc, count, TYPEIX_WEAKBOX);
}




void __pure vec_fill(vec_scp vec, descr d)
{
	descr *a = vec_tail(vec);
	const ulong len =  vec_len(vec);
	for(ulong i = 0; i < len ; i++) {
		a[i] = d;
	}
}

void __pure vec_clear(vec_scp vec)
{
	memset(vec_tail(vec), 0, vec_len(vec) * sizeof(descr));
}





int __pure descr_hash_code(descr d)
{
	if(scp_valid(d)) {
		switch(datum_get_typeix(d)) {
		case TYPEIX_STRING:   return   ss_hash_code((string_scp)d);
		case TYPEIX_SYMASSOC: return   ss_hash_code(((symasc_scp)d)->name);
		case TYPEIX_NUM_REAL: {
			volatile double val = ((real_scp)d)->value;
			volatile int32_t *x  = (int32_t*)&val;
			return (x[1]^x[0]) & HASH_MASK;
		}
		default:
			assert("never here hash" && false);
		}
	} else if(desc_is_spec(d)) {
		return descr_hash_code(symbolobj(d));
	} else if(desc_is_fixnum(d))  {
		return desc_to_fixnum(d) & HASH_MASK;
	} else if(desc_is_char(d)) {
		return desc_to_char(d) & HASH_MASK;
	}
	//logf_debug(Sys_log, "hash key %ld", ((scp_valid(d) ) ? datum_get_typeix(d) : d));
	_mthrow_(EXCEP_UNSUPPORTED_OP, "no hash funct for the type");
	return -1;
}

bool __pure datum_eq(datum_scp d1, datum_scp d2)
{
	ulong s1 = datum_size(d1);
	return s1 == datum_size(d2) &&
		(0 == memcmp(datum_tail(d1), datum_tail(d2), s1 - sizeof(struct Datum)));
}

bool inline __pure typep_compatible(type_scp t1, type_scp t2)
{
	return (type_is_carcdr(t1) && type_is_carcdr(t2));
}

bool __pure descr_eq_general(descr d1, descr d2, bool requr)
{
	if(d1 == d2 || (d1 == NIHIL && d2 == SYMB_NIL) || (d2==NIHIL && d1==SYMB_NIL)) {
		return true;
	}
	if(scp_valid(d1) && scp_valid(d2)) {
		uint tix = datum_get_typeix(d1);
		uint tix2 = datum_get_typeix(d2);
		type_scp t = type_from_ix(tix);
		type_scp t2 = type_from_ix(tix2);
		if(tix == datum_get_typeix(d2)) {

			switch(tix) {
				case TYPEIX_NUM_REAL:
					return ((real_scp)d1)->value == ((real_scp)d2)->value;
				case TYPEIX_STRING:
					return ss_eq((string_scp)d1, (string_scp)d2);
				case TYPEIX_SYMASSOC:
					return d1 == d2;
				case TYPEIX_SYNTAX_IDENT:
					return id_eq( d1, d2);
				default:
					if (requr && type_is_nonterm(t)) {
						ulong count = datum_slot_count(t, d1);
						if(count != datum_slot_count(t, d2)) {
							return false;
						}
						descr *slots1 = datum_slots(t, d1);
						descr *slots2 = datum_slots(t, d2);
						for(ulong i = 0; i < count; i++) {
							if(!descr_eq_general(slots1[i], slots2[i], true)) {
								return false;
							}
						}
						return true;
					} else {
						return datum_eq((datum_scp)d1, (datum_scp)d2);
					}


			}
		} else if(typep_compatible(t,t2)) {
			ulong count = min(datum_slot_count(t2, d2),datum_slot_count(t, d1));
			descr *slots1 = datum_slots(t, d1);
			descr *slots2 = datum_slots(t, d2);
			for(ulong i = 0; i < count; i++) {
				if(!descr_eq_general(slots1[i], slots2[i], true)) {
					return false;
				}
			}
			return true;
		}
	}
	return false;
}

inline bool descr_eq(descr d1, descr d2)
	{ return descr_eq_general(d1, d2, false); }
inline bool descr_eqrec(descr d1, descr d2)
	{ return descr_eq_general(d1, d2, true); }



descr bin_htb_eq_classic(svm_t vm, uint argc, __binargv(argv))
{
	descr mbkey = argv[0]; cons_scp val = (cons_scp) argv[1];
	if(scp_valid(mbkey) && datum_is_carcdr(mbkey)) {mbkey = ((cons_scp)mbkey)->car;} // getting key;
	return desc_cond(descr_eq(mbkey, val->car));

}
descr bin_htb_hash_classic(svm_t vm, uint argc,__binargv(argv))
{
	descr mbkey = argv[0];
	if(scp_valid(mbkey) && datum_is_carcdr(mbkey)) {mbkey = ((cons_scp)mbkey)->car;} // getting key;
	return desc_from_fixnum(descr_hash_code(mbkey));
}






extern closure_scp Bin_htb_eq_classic,
	Bin_htb_hash_classic;

hashtable_scp new_htbc(svm_t vm, gc_t gc, ulong count)
{
	return new_htb(vm, gc, count, &Bin_htb_hash_classic, &Bin_htb_eq_classic);
}

hashtable_scp new_htb(svm_t vm, gc_t gc, ulong count, __sbp(closure_scp) hashfun, __sbp(closure_scp) eqfun)
{
	SAVE_RSTK();
	__protect( vec_scp, v,  new_vec_box(vm, gc, count));
	__ret hashtable_scp h = new_htb_g(vm, gc, &v, hashfun, eqfun);
	UNWIND_RSTK();
	return h;
}

// general hash table constructor
hashtable_scp new_htb_g(svm_t vm, gc_t gc, __sbp(vec_scp) buckets,
		__sbp(closure_scp) hashfun, __sbp(closure_scp) eqfun )
{
	__ret hashtable_scp result =  (hashtable_scp) inew_inst(gc, TYPEIX_HASHTABLE);
	result->b = _DS(buckets);
	result->hashfun = _DS(hashfun);
	result->eqfun = _DS(eqfun);
	result->bits = 0;
	return result;
}

inline bool htb_eq(svm_t vm, __sbp(hashtable_scp) ph, __sbp(descr) key, __sbp(descr) pair )
{
	decl_bin_argv(argv) {_DS(key), _DS(pair)};
	return desc_is_t(vm_call(vm, _DS(ph)->eqfun, 2, argv));
}

inline  box_scp htb_getix(svm_t vm, __sbp(hashtable_scp) ph, __sbp(descr) pairOrKey)
{
	decl_bin_argv(argv) {_DS(pairOrKey)};
	long hc = desc_to_fixnum(vm_call(vm, _DS(ph)->hashfun, 1, argv));
	if(hc < 0) {
		_mthrow_(EXCEP_ASSERT, "no hash code");
	}
	return vec_tail(_DF(ph, b))[ hc % htb_rowc(_DS(ph))];
}



descr _htb_getnode(svm_t vm, gc_t rem,  __sbp(hashtable_scp) ph, __sbp(descr) key, box_scp row)
{
	SAVE_RSTK();
	PUSH_RSTK(row);
	__protect(vec_scp, buks, _DF(ph,b));
	row = (row != NIHIL) ? row : htb_getix(vm, ph, key);
	__protect(cons_scp, v, unbox(row));
	__protect(cons_scp, prev, NIHIL);
	__ret descr result = NIHIL;
	if(v) {
		lcycprev(cons_scp, v, prev) {
			if(htb_eq(vm, ph, key, &v->car)) {
				if(rem) {
					if(prev == NULL) {
						box_set(row, v->cdr);
					} else {
						FIELD_SET(prev, cdr, v->cdr);
					}
					result = v->car; // on remove we return pair removed! (not a node)
					break;
				}
				result = (descr) v; // on get we return a node.
				break;
			}
		}
	}
	UNWIND_RSTK();
	return result;
}


// PUSHROOT key
descr htb_getpair(svm_t vm,  __sbp(hashtable_scp) h, __sbp(descr) key)
{

	cons_scp n = (cons_scp) _htb_getnode(vm, NULL, h, key, NIHIL);
	return n ? n->car : NIHIL;
}



/*ret NULL if new, or old value*/
// PUSHROOT pair
descr htb_putpair(svm_t vm, gc_t gc, __sbp(hashtable_scp) h, __sbp(descr) pair)
{
	SAVE_RSTK();
	__protect(box_scp, row,  htb_getix(vm, h, pair));
	__protect( cons_scp, nod , (cons_scp) _htb_getnode(vm, NULL, h, pair, row));
	__protect( descr, result,  NIHIL);
	if(nod) {
		if(_DS(h)->bits & HTB_BITM_NO_REPUT) {
			_mthrow_(EXCEP_HTB_REPEATED_PUT, "repeated hash put");
		}
		result = nod->car;
		FIELD_SET(nod, car, _DS(pair));
	} else {
		nod = new_cons(vm, gc, (descr) _DS(pair), unbox(row));
		box_set(row, nod);
	}
	UNWIND_RSTK();
	return result;
}

descr htb_rem(svm_t vm, gc_t gc,  __sbp(hashtable_scp) h, __sbp(descr) key)
{
	return _htb_getnode(vm, gc, h, key, NIHIL);
}

descr htb_put(svm_t vm, gc_t gc, __sbp(hashtable_scp) h, __sbp(descr) key, __sbp(descr) value)
{
	SAVE_RSTK();
	__protect(descr, pair,  new_cons(vm, gc, _DS(key), _DS(value)));
	__ret descr d = htb_putpair(vm, gc, h,  &pair );
	UNWIND_RSTK();
	return d == NIHIL ? NIHIL : CDR(CAR(d));
}

descr htb_get(svm_t vm, __sbp(hashtable_scp) h,__sbp(descr) key)
{
	descr d = htb_getpair(vm,  h,  key);
	return d == NIHIL ? NIHIL : CDR(d);
}


/*ASTK*/

astk_scp new_astk(gc_t gc, ulong count)
{
	__loc astk_scp r = (astk_scp) inew_array(gc, TYPEIX_ARRAYSTACK, count);
	r->ptr = 0;
	return r;
}

astk_scp astk_push(svm_t vm, __arg astk_scp stk, __arg descr val)
{
	SAVE_RSTK();
	PUSH_RSTK(stk);
	ulong pos = stk->ptr;
	if(stk->ptr < stk->count) {
		stk->ptr ++;
		ASTK_SET(stk, pos, val);
		UNWIND_RSTK();
		return stk;
	}
	assert(pos == astk_capac(stk));

	PUSH_RSTK(val);
	decl_gc_of_vm();
	__loc astk_scp r = new_astk(gc, astk_capac(stk) * 2);
	r->ptr = pos;
	memcpy(astk_tail(r), astk_tail(stk), sizeof(descr*)*pos);
	astk_tail(r)[pos] = val;
	r->ptr++;
	UNWIND_RSTK();
	return r;
}

descr __pure astk_pop(astk_scp stk)
{
	_mthrow_assert_(!astkp_empty(stk), EXCEP_INDEX_OUT_OF_RANGE, "stack array not empty for pop()");
	descr r = astk_tail(stk)[--stk->ptr];
	return r;
}



/** ARLIST routines */



arlist_scp new_arlist(svm_t vm, gc_t gc, ulong count)
{
	SAVE_RSTK();
	__protect(vec_scp, v, new_vec(gc, count));
	__ret arlist_scp al = (arlist_scp) inew_inst(gc, TYPEIX_ARRAY_LIST);
	al->index = 0;
	al->data = v;
	UNWIND_RSTK();
	return al;
}



void arlist_grow(svm_t vm, gc_t gc, __sbp(arlist_scp) pal, float multiple)
{
	SAVE_RSTK();
	assert(multiple > 1);
	ulong ncount =(ulong) (vec_len(_DS(pal)->data) * multiple);
	__protect(vec_scp,  nv,  new_vec(gc, ncount ));
	arlist_scp al  = _DS(pal);
	memcpy(vec_tail(nv), vec_tail(al->data), vec_len(al->data) * sizeof(descr));
	del_inst(gc, al->data);
	FIELD_DSET(pal, data, nv);
	UNWIND_RSTK();
}

inline void arlist_skip_at(arlist_scp al, ulong index)
{
	assert(index >= al->index && index < arlist_count(al));
	al->index = index;
}

inline void arlist_push(svm_t vm, gc_t gc, __arg arlist_scp al, __arg descr d )
{
	SAVE_RSTK();
	PUSH_RSTK2(al, d);
	arlist_add(vm,gc, &al,&d);
	UNWIND_RSTK();
}



inline ulong arlist_add(svm_t vm, gc_t gc, __sbp(arlist_scp) al, __sbp(descr) d)
{

	ulong r = _DS(al)->index;
	if(r >= arlist_count(_DS(al)) ) {
		arlist_grow(vm, gc, al, 2.0);
	}
	VEC_SET(_DS(al)->data, r, _DS(d));
	_DS(al)->index ++;
	return r;
}

// PUSHROOT name
inline symasc_scp new_symassoc(svm_t vm, gc_t gc, __arg string_scp name)
{
	SAVE_RSTK();
	PUSH_RSTK(name);
	__loc symasc_scp r = (symasc_scp) inew_inst(gc, TYPEIX_SYMASSOC);

	r->name = name;
	r->globix = NIHIL;
	UNWIND_RSTK();
	return r;
}
// PUSHROOT val
inline box_scp new_wbox(svm_t vm, gc_t gc, __arg descr val)
{
	SAVE_RSTK();
	PUSH_RSTK(val);
	__loc box_scp r = (box_scp) inew_inst(gc, TYPEIX_WEAKBOX);
	r->boxed = val;
	UNWIND_RSTK();
	return r;
}



/*list routines*/

inline ulong __pure cl_len(cons_scp cons) __throw
{
	ulong count = 0;
	cons_scp p = cons;
	until(!datum_is_carcdr(p)) {
		count ++;
		p = p->cdr;
	}
	if(!cl_end(p)) {
		_throw_(EXCEP_IMPROPER_LIST);
	}
	return count;
}


inline cons_scp __pure cl_last(cons_scp cl)
{
	if(cl_end(cl)) {
		return NIHIL;
	}
	until(!datum_is_carcdr(CDR(cl))) {
		cl = CDR(cl);
	}
	return cl;

}

inline long __pure cli_len(cons_scp cons)
{
	long count = 0;
	cons_scp p = cons;
	until(!datum_is_carcdr(p)) {
		count ++;
		p = p->cdr;
	}
	return cl_end(p) ? count : -count;
}




inline  bool __pure clp_proper(cons_scp cons)
{
	cons_scp p = cons;
	until(!datum_is_carcdr(p)) {
		p = CDR(p);
	}
	return cl_end(p);
}

inline cl_scp cl_clone(svm_t vm, __arg cons_scp cons)
{

	if(cl_end(cons)) {
		return SYMB_NIL;
	} else if(!datum_is_carcdr(cons)) {
		return cons;
	}

	SAVE_RSTK();
	PUSH_RSTK(cons);
	decl_gc_of_vm();
	__protect(cons_scp, p, cons);

	__protect(cons_scp, r, NIHIL);
	__protect(cons_scp, q, NIHIL);
	__protect(cons_scp, d,  CONSCAR(cl_clone(vm, CAR(p))));
	r = q = d;
	p = CDR(p);
	until(!datum_is_carcdr(p)) {
		d = CONSCAR(cl_clone(vm, CAR(p)));
		FIELD_SET(q, cdr, d); q = d;
		p = CDR(p);
	}
	if(!cl_end(p)) {
		d = cl_clone(vm, p);
		FIELD_SET(q, cdr, d);
	}
	UNWIND_RSTK();
	return r;
}



cl_scp args_to_cl(svm_t vm, descr *args, ulong from, ulong lim)
{
	decl_gc_of_vm();
	if(from == lim) {
		return (cl_scp) SYMB_NIL;
	}
	SAVE_RSTK();
	__protect(cons_scp, r, CONSCAR(args[from++]));
	__protect(cons_scp, p, r);
	__protect(cons_scp, d, 0);

	for(;from<lim;from++) {
		d = CONSCAR(args[from]);
		FIELD_SET(p, cdr, d);
		p = d;
	}
	UNWIND_RSTK();
	return r;
}

inline vec_scp cl_to_vec(svm_t vm, gc_t gc, __sbp(cons_scp) l)
{
	ulong count = cl_len(_DS(l));
	__ret vec_scp result = new_vec(gc, count);
	descr* v = vec_tail(result);
	__loc cons_scp p = _DS(l);
	ulong i = 0;
	until(cl_end(p)) {
		v[i] = p->car;
		p = p->cdr;
		i++;
	}
	return result;
}

inline buf_scp cl_to_vu8(svm_t vm, gc_t gc, __sbp(cons_scp) l)
{
	ulong count = cl_len(_DS(l));
	__ret buf_scp result = new_buf(gc, count);
	byte* v = buf_tail(result);
	cons_scp p = _DS(l);
	ulong i = 0;
	until(cl_end(p)) {
		descr n = p->car;
		if(!desc_is_fixnum(n) || (desc_to_fixnum(n) < 0 || desc_to_fixnum(n) > 255)) {
			return NIHIL;
		}
		v[i] = desc_to_fixnum(n);
		p = p->cdr;
		i++;
	}
	return result;
}






