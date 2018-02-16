/* memmgr.h Memory manager (GC)
 * musc- scheme translator & interpreter
 * author: denis.lepekhin@gmail.com
 * created on: 2009
 */

#ifndef MEMMGR_H_
#define MEMMGR_H_

#include "datum.h"
#include "vmdefs.h"

#define EXCEP_NO_MEM 527




#define GC_BYTE_MASK 	(_lmask_(8) << GC_BYTE_SHIFT )
/* see vmdefs.h
 * #define GC_BYTE_SHIFT   24
#define GC_CLASS_MASK	(_lmask_(3) << GC_BYTE_SHIFT )
#define tag_is_generalcls(tag)      !((tag) & GC_CLASS_MASK)
#define datum_is_generalcls(dm)      tag_is_generalcls(datum_get_tag(dm))
#define datum_is_old(d)    tag_is_old(datum_get_tag(d))
#define datum_is_young(dm)   (datum_is_generalcls(dm) && !datum_is_old(dm))
*/

// #define GC_BIT_OLD		27 /*defined in vmdefs.h*/

#define GC_BIT_ADDED		28


#define GC_AGE_SHIFT    29
#define GC_AGE_MASK		( _lmask_(3) << GC_AGE_SHIFT )

#define GC_REFC_SHIFT  32
#define GC_REFC_MASK  (_lmask_(32) << GC_REFC_SHIFT)


#define GC_CLASS_PERM 		1
#define GC_CLASS_MALLOC 	2
#define GC_GENERAL         0





#define tag_get_gcclass(tag)		   _getmasked_(tag, GC_CLASS_MASK, GC_BYTE_SHIFT)
#define tag_set_gcclass(tag, gcc) 	_setmasked_(tag, _T_(gcc),GC_CLASS_MASK, GC_BYTE_SHIFT)


#define tag_set_old(tag)		   ((tag) | _lbit_(GC_BIT_OLD))

#define datum_set_old(dm)         datum_get_tag(dm) = tag_set_old(datum_get_tag(dm))

#define tag_set_young(tag)       ((tag) & ~_libit_(GC_BIT_OLD))

#define tag_is_young(tag)        (tag_is_generalcls(tag) && !tag_is_old(tag))

#define tag_is_added(tag)			((tag) & _lbit_(GC_BIT_ADDED))
#define tag_set_added(tag, bit)	(((tag) & ~_lbit_(GC_BIT_ADDED)) | _llshft_(((bit) & 1), GC_BIT_ADDED))

#define tag_get_age(tag)		_getmasked_(tag, GC_AGE_MASK, GC_AGE_SHIFT)
#define tag_set_age(tag, age)	_setmasked_(tag, _T_(age), GC_AGE_MASK, GC_AGE_SHIFT)
#define tag_inc_age(tag)		((tag) + _lbit_(GC_AGE_SHIFT))

#define tag_get_refc(tag)		_getmasked_(tag, GC_REFC_MASK, GC_REFC_SHIFT)
#define tag_set_refc(tag, refc)		_setmasked_(tag, _T_(refc), GC_REFC_MASK, GC_REFC_SHIFT)
#define tag_inc_refc(tag)		((tag) + _lbit_(GC_REFC_SHIFT))
#define tag_dec_refc(tag)		((tag) - _lbit_(GC_REFC_SHIFT))
#define tag_has_refc(tag)    	((tag) & GC_REFC_MASK)

#define datum_set_gcclass(dm, gcc)	dm->tag = tag_set_gcclass(dm->tag, gcc)
#define datum_is_gcclass(dm, gcc )  ( tag_get_gcclass((dm)->tag) == (gcc) )



#define GC_REFCNT_MASK  _mask_(8)

#define tag_get_upper_dword(tag) 		 ((tag) >> 32)
#define tag_get_refcnt(tag)            (((tag) >> 32) & GC_REFCNT_MASK)
#define tag_inc_refcnt(tag)				(tag + (1L << 32))
#define tag_dec_refcnt(tag)				(tag - (1L << 32))
#define tag_get_gix(tag)					((tag) >> (40))
#define tag_set_gix(tag, gix)				tag |= (gix) << 40





#define TAG_FORWARD_MASK      _lmask_(1)
#define tag_is_forward(tag)   (!((tag) & TAG_FORWARD_MASK))
#define datum_is_forward(d)   tag_is_forward(datum_get_tag(d))
#define datum_set_forward(dm, fw) ((datum_scp)dm)->tag = ((tag_t) (fw))
#define datum_get_forward(dm)     ((descr)((datum_scp)dm)->tag)


typedef struct {
	float live_per_all;
	float old_per_new;
	ulong eursz, limsz;
	uint newagemax;
	ulong   nov_sz, oref_sz; // size of semispace of new-zone

} gcmov_param_t;




void gc_test(svm_t vm);


void gc_mov_param_init(gcmov_param_t *p);
void gc_mov_param_init_small(gcmov_param_t *p);
gc_t gc_perm_make(ulong size);
gc_t gc_mov_make(gcmov_param_t *p);

#endif /* MEMMGR_H_ */
