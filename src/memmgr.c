/* memmgr.c Memmory Manager
 * scheme translator & interpreter
 * author: denis.lepekhin@gmail.com
 * created on: 2009
 */


#include <malloc.h>
#include "vmdefs.h"
#include "memmgr.h"
#include <math.h>
#include "sysexcep.h"
#include <string.h>
//#undef _LOG_DEBUG
#include "loggers.h"
#include "temp.h"

#include "ioport.h"

logger_t Gc_log = NULL;
FILE *Gc_log_file = NULL;

gc_t Gc_malloc = NULL;

typedef struct  {
	struct GcBase prot;
	byte *begin;
	byte *end;
	ulong size;
	byte *curp;
} gcperm_t;




inline void  *alloc_try(type_scp type, ulong vcount, byte *curp, void* upper);


datum_scp gc_perm_alloc(gcperm_t *gc, type_scp t, ulong size);
bool gc_perm_belong(gcperm_t *gc, datum_scp d);
void gc_perm_stat(gcperm_t *gc, gc_stat_t *stat);

void empty_delete(gc_t gc, datum_scp dm) { }
void empty_stat(gc_t gc, gc_stat_t *stat)
{
	stat->allocated_percent = NAN;
}

byte *tmalloc(ulong sz)
{
	byte *mem = (byte*) malloc( sz);
	if(!mem) {
		_mthrow_(EXCEP_NO_MEM, "malloc() failed to allocate memory");
	}
	return mem;
}

gc_t gc_perm_make(ulong size)
{
	gcperm_t *gc = __newstruct(gcperm_t);
	gc->prot.alloc =   (datum_allocator) gc_perm_alloc;
	gc->prot.belong =  (gc_belonger_t)   gc_perm_belong;
	gc->prot.getstat = (gc_getstat_t)    gc_perm_stat;
	gc->prot.free = empty_delete;
	gc->begin = tmalloc(size);
	if(gc->begin == NULL) {
		_mthrow_(EXCEP_NO_MEM, "malloc failed in gc_perm_init()");
	}
	gc->end = ((byte*)gc->begin) + size;
	gc->size = size;
	gc->curp = gc->begin;
	return (gc_t)gc;
}

bool gc_perm_belong(gcperm_t *gc, datum_scp d)
{
	return scp_valid(d) && datum_is_gcclass(d, GC_CLASS_PERM) ;
}

void gc_perm_stat(gcperm_t *gc, gc_stat_t *stat)
{
	stat->allocated_percent = ((double)((char*)gc->curp - (char*)gc->begin))/(double)gc->size;
}
/* returns NULL in case of error */
inline void  *alloc_try(type_scp type, ulong vcount, byte *curp, void* upper) {
	datum_scp scp = (datum_scp) curp;
	curp += type_aligned_sz(type, vcount) ;
	assert((ulong)curp % 8 == 0);
	if(curp >=( (byte*)upper-8)) {
		return NULL; /*ERROR*/
	}
	type_prepare_datum(type, scp, vcount);
	return curp;
}
void gc_perm_enlarge(gcperm_t *gc)
{
	_mthrow_(EXCEP_NO_MEM, "out of mem: can't enlarge permanent memory");
}

datum_scp gc_perm_alloc(gcperm_t *gc, type_scp t, ulong size)
{
	void *next = alloc_try(t, size, gc->curp, gc->end);
	while (next == NULL) {
		gc_perm_enlarge(gc);
		next = alloc_try(t, size, gc->curp, gc->end);
	}
	datum_scp result = (datum_scp) desc_from_scp(gc->curp);
	gc->curp = next;
	datum_set_gcclass(result, GC_CLASS_PERM);
	datum_set_old(result);
	return (datum_scp)result;
}



datum_scp gc_malloc_alloc(gc_t gc, type_scp t, ulong vcount)
{
	ulong sz = type_aligned_sz(t, vcount);
	datum_scp dm = (datum_scp) tmalloc(sz);
	assert((ulong) dm % 8 == 0);
	type_prepare_datum(t, dm, vcount);
	datum_set_gcclass(dm,GC_CLASS_MALLOC );
	datum_set_old(dm);
	return dm;
}

inline bool gc_malloc_belong(gc_t gc, datum_scp dm)
{
	return scp_valid(dm) && datum_is_gcclass(dm, GC_CLASS_MALLOC);
}

void  gc_malloc_free(gc_t gc, datum_scp dm)
{
	if(gc_malloc_belong(gc, dm))
	{
		free(dm);
	}
}

/*for raw allocation of global data only! */
datum_scp raw_allocate(uint tix, ulong mask, ulong size)
{
	datum_scp d = (datum_scp) tmalloc(size);
	d->tag = tag_from_typeix(tix) | mask;
	datum_set_gcclass(d, GC_CLASS_MALLOC);
	return  d;
}



void gc_init()
{
	global_once_preamble();
	Gc_malloc = __newstruct(struct GcBase);
	Gc_malloc->alloc = gc_malloc_alloc;
	Gc_malloc->free =  gc_malloc_free;
	Gc_malloc->belong = gc_malloc_belong;
	Gc_malloc->getstat = empty_stat;
}

typedef struct {
	byte *begin, *end, *ptr, *scan;
	ulong size;
} memblk_t;

#define mblk_realsz(pb)   ((pb)->ptr - (pb)->begin)
#define mblk_leftsz(pb)		((pb)->end - (pb)->ptr)

void mblk_alloc(memblk_t *mb, ulong sz)
{
	mb->ptr =  mb->begin = (byte*) malloc(sz);
	mb->size = sz;
	mb->end = mb->begin + sz;
}

inline void mblk_init(memblk_t *mb, void* begin, ulong sz)
{
	mb->scan = mb->ptr =  mb->begin = (byte*) begin;
	mb->size = sz;
	mb->end = mb->begin + sz;
}
inline byte* mblk_copyto(memblk_t *blk, datum_scp d)
{
	assert(!datum_is_forward(d) && "non frw");
	ulong sz = datum_aligned_sz(d);
	byte* ptr = blk->ptr;
	assert(ptr + sz <= blk->end);
	memcpy(ptr, d, sz);
	blk->ptr += sz;
	datum_set_forward(d, ptr);
	return ptr;
}

inline void mblk_add(memblk_t *dest, memblk_t *src)
{
	ulong sz = mblk_realsz(src);
	assert(mblk_leftsz(dest) >= sz);
	memcpy(dest->ptr, src->begin, sz);
	dest->ptr+=sz;
}


#define mblk_scanset(pb)   ((pb)->scan = (pb)->ptr)
#define mblk_scansetbegin(pb)   ((pb)->scan = (pb)->begin)
#define mblk_ptrset(pb)   ((pb)->ptr = (pb)->begin)
#define mblk_belong(pblk, ptr) ( (  (byte*)(ptr) >= (byte*)(pblk)->begin) && ((byte*)(ptr) < (byte*)(pblk)->end) )

inline void mblk_swap(memblk_t *b1, memblk_t *b2)
{
	memblk_t m = *b1;
	*b1 = *b2;
	*b2 = m;
}

typedef struct {
	struct GcBase prot;
	memblk_t from, to, ofrom, oto;
	void *memold, *memnov;
	float live_per_all;
	float old_per_new;
	uint newagemax;
	ulong totalsz, oldsz, novsz, limsz, eursz;

	datum_scp *orefs, *orefscur, *orefsy2o, *orefslim;


	//ulong or_index; ulong or_count;



} gcmov_t;




void gc_mov_param_init(gcmov_param_t *p)
{
	p->live_per_all = 0.43;
	p->old_per_new = 8;
	p->limsz = _Mb_(100);
	p->eursz = _Kb_(16);
	p->nov_sz = _Mb_(3);//_Mb_(3);
	p->oref_sz = _Mb_(2);
	p->newagemax = 1;
}

void gc_mov_param_init_small(gcmov_param_t *p)
{
	p->live_per_all = 0.73;
	p->old_per_new = 10;
	p->limsz = _Mb_(50);
	p->eursz = _Kb_(4);
	p->nov_sz = _Kb_(5);
	p->oref_sz = _Kb_(1);
	p->newagemax = 1;
}

datum_scp gc_mov_alloc(gcmov_t *gc, type_scp typ, ulong vcount);
void gc_mov_minorcollect(gcmov_t *gc);
void gc_mov_majorcollect(gcmov_t *gc);
void gc_mov_getstat(gcmov_t *gc, gc_stat_t *p);
bool gc_mov_belong(gcmov_t *gc, datum_scp dm);

inline void gc_mov_init_layout(gcmov_t *gc)
{
	mblk_init(&gc->ofrom, gc->memold, gc->oldsz);
	mblk_init(&gc->oto, gc->ofrom.end , gc->oldsz);
	mblk_init(&gc->from, gc->memnov, gc->novsz);
	mblk_init(&gc->to, gc->from.end, gc->novsz);
}



gc_t gc_mov_make(gcmov_param_t *p)
{
	gcmov_t *gc = __newstruct(gcmov_t);

	gc->prot.alloc = (datum_allocator) gc_mov_alloc;
	gc->prot.free = empty_delete;
	gc->prot.getstat = (gc_getstat_t) gc_mov_getstat;
	gc->prot.belong = (gc_belonger_t) gc_mov_belong;

	gc->newagemax = p->newagemax;
	assert(gc->newagemax <= 7);
	gc->novsz = align_8( p->nov_sz);
	gc->old_per_new = p->old_per_new;
	gc->live_per_all = p->live_per_all;
	assert(p->live_per_all <= 1.0);
	assert(p->old_per_new >= 2.0);
	gc->limsz = p->limsz;
	gc->eursz = p->eursz;
	gc->oldsz = align_8( (ulong)(p->old_per_new * (float)gc->novsz));
	gc->totalsz =  (gc->oldsz + gc->novsz) * 2;
	gc->memold = tmalloc(gc->oldsz * 2);
	gc->memnov = tmalloc(gc->novsz * 2);

	gc->orefsy2o = gc->orefscur = gc->orefs = (datum_scp*)tmalloc(p->oref_sz);
	gc->orefslim =  (datum_scp*) ( ((byte*)gc->orefs) + align_8(p->oref_sz) );
	gc_mov_init_layout(gc);
	return (gc_t)gc;
}






inline void __gc_check_old(svm_t vm, datum_scp l)
{
	assert(datum_is_old(l) && "must be old object");
	gcmov_t *gc = (gcmov_t*) vm_gc(vm);
	tag_t tag = datum_get_tag(l);
	if(!tag_is_added(tag)) {
		*(gc->orefscur++) = (datum_scp) l;
		((datum_scp)l)->tag = tag_set_added(tag, 1);
		if(gc->orefslim == gc->orefscur) {
			log_info(Gc_log, "old->new assign's pool is full. minor collection.");
			gc_mov_minorcollect(gc);
		}

	}
#ifdef DBG_MEMMGR_SLOTSET
		//gc_mov_majorcollect(gc);
		gc_mov_minorcollect(gc);
#endif
}


void gc_mov_getstat(gcmov_t *gc, gc_stat_t *p)
{
	ulong alloc = mblk_realsz(&gc->ofrom) + mblk_realsz(&gc->from);
	p->allocated_percent = ((double) alloc) / ((double)gc->totalsz);
}

bool gc_mov_belong(gcmov_t *gc, datum_scp dm)
{
	return (dm!=0) && datum_is_gcclass(dm, GC_GENERAL);
}

datum_scp gc_mov_alloc(gcmov_t *gc, type_scp typ, ulong vcount)
{
	byte* next = alloc_try(typ, vcount, gc->from.ptr, gc->from.end);
	int i = 0;
	while(next == NULL) {
		logf_info(Gc_log, "young space is full. minor collection, attempt = %i", i++);
		gc_mov_minorcollect(gc);
		next = alloc_try(typ, vcount, gc->from.ptr, gc->from.end);
	}
	datum_scp result = (datum_scp) gc->from.ptr;
	datum_set_gcclass(result, GC_GENERAL);
	// result->tag = tag_set_young(datum_get_tag(result));
	gc->from.ptr = next;
	return result;
}





typedef descr (*cheneycopier_t) (gcmov_t *gc, datum_scp datum, datum_scp slot, memblk_t *blk);


#define CHENEY_MISWEAK 1

#define vm_stk_count(vm)  (((byte*)vm->rstk_ptr - (byte*)vm->rstk_begin)/sizeof(descr**))

inline void cheney_copy(gcmov_t *gc, memblk_t *blk, cheneycopier_t copyproc, ulong bits)
{
	decl_vm();
	int i = 0;

	logf_debug(Gc_log, "CHENEY START %i  ptr -> %p  stkcount -> %ld", i, blk->ptr, vm_stk_count(vm));
	while (blk->scan != blk->ptr) {
		i ++;
		datum_scp dm = (datum_scp) blk->scan;
		assert("cheney vptr" && scp_valid(dm));
		/*
		if(datum_is_forward(dm)) {
			printf("\n-->>>>%i  %ld", i, vm_stk_count(vm) );
			oport_print_datum(vm, Out_console_port, datum_get_forward(dm));
			printf("\n" );
			return;
		}
		*/

		logf_debug(Gc_log, "CHENEY %i  %s %s sz=%i  scan=%p  ptr->%p age->%ld", i, datum_get_type(dm)->asci, datum_is_old(dm) ? "OLD": "YNG", datum_aligned_sz(dm),
				blk->scan, blk->ptr, tag_get_age(dm->tag));
		fflush(stdout);
		assert("cheney non frw" && !datum_is_forward(dm));
		blk->scan += datum_aligned_sz(dm);
		if((bits & CHENEY_MISWEAK) && datum_is_weakcont(dm) ) {
			assert(tag_is_added(dm->tag) && "weak added");
			continue;
		}

		assert(blk->scan <= blk->end);
		if(datum_is_nonterm(dm)) {
			SLOT_FOREACH(dm)
				if (scp_valid(slot)) {
					if (datum_is_forward(slot)) {
						*pslot = datum_get_forward(slot);
					} else if(copyproc && datum_is_generalcls(slot)) {
						*pslot =  copyproc(gc, dm, (datum_scp)slot, blk);
						assert(blk->ptr <= blk->end);

					}

				}
			END_SLOT_FOREACH
		}
	}
}


inline void major_weak_update(gcmov_t *gc)
{
	svm_t vm = gc->prot.vm;
	datum_scp *p = gc->orefs;
	descr *lim = gc->orefscur;
	for(; p != lim; p++) {
		assert(lim == gc->orefscur && "finalizer not mutate");
		datum_scp dm = *p;
		assert(datum_is_weakcont(dm) && "must be weak cont");
		dm->tag = tag_set_added(dm->tag, 0);
		descr *slots, *slots_end;
		datum_get_slots(dm, &slots, &slots_end);
		for (;slots != slots_end; slots++) {
			descr slot = *slots;
			if (scp_valid(slot)) {
				if (datum_is_forward(slot)) {
					*slots = datum_get_forward(slot);
				} else if(datum_is_generalcls(slot)) {

					datum_finalize(vm, slot);
					datum_set_forward(slot, 0);
					*slots = NIHIL;
				}
			}
		}
	}
	gc->orefscur = gc->orefs;
}

inline descr chcpy_minor_to_old(gcmov_t *gc,  datum_scp datum,  datum_scp slot, memblk_t *blk)
{
	//assert(!datum_is_old(slot));
	if(datum_is_old(slot)) {
		return slot;
	}
	slot->tag = tag_set_old(tag_set_added(slot->tag, 0));
	assert(datum_is_old(slot) && "mstbe old");

	return (descr) mblk_copyto(blk, slot);
}



inline descr chcpy_minor_young(gcmov_t *gc,  datum_scp datum,  datum_scp slot, memblk_t *blk)
{

	if(datum_is_old(slot)) {
		return slot;
	}
#ifdef DBG_MEMMGR
	if(mblk_belong(blk, slot)) {
		logf_debug(Gc_log, "slot  belong:  %s  at %p of %s at %p", datum_get_type(slot)->asci,  slot, datum_get_type(datum)->asci,datum);
		//decl_vm();
		//oport_print_datum(vm, Out_console_port, slot);
		(assert(0 && "slot belong to to"));
	}
#endif
	assert(!mblk_belong(blk, slot) && "slot must belong to from space");
	assert(imply(datum, !datum_is_old(datum)) && "dm mstbe young");

	descr result = NIHIL;
	tag_t tag = tag_inc_age(slot->tag);
	slot->tag = tag;
	result = (descr) mblk_copyto(blk, slot);
	if (gc->newagemax <= tag_get_age(tag) ){
		if(datum != NIHIL && tag_is_added(datum_get_tag(datum))) {
			tag =  tag_set_added(tag, 1);
			datum_set_tag(result, tag);
		}
		if(!tag_is_added(tag)) {
			datum_set_tag(result, tag_set_added(tag, 1));
			logf_debug(Gc_log, "overaged  %s at %p  age->%ld", datum_get_type(result)->asci,  result, tag_get_age(tag) );
			*(gc->orefscur++) = result;
			assert(gc->orefscur  < gc->orefslim && "orefs ovf");
		}
	}
	//result = (descr) chcpy_minor_to_old(gc, slot, &gc->ofrom);
	//cheney_copy(gc, &gc->ofrom, chcpy_minor_to_old, 0);
	// to young space
	return result;
}


inline descr chcpy_major_old(gcmov_t *gc,  datum_scp datum, datum_scp slot, memblk_t *blk)
{
#ifdef DBG_MEMMGR
	if(mblk_belong(blk, slot)) {
		logf_debug(Gc_log, "slot  belong:  %s  at %p of %s at %p", datum_get_type(slot)->asci,  slot, datum_get_type(datum)->asci,datum);
		//decl_vm();
		//oport_print_datum(vm, Out_console_port, slot);
		(assert(0 && "slot belong to to"));
	}
#endif

	slot = (descr) mblk_copyto(blk, slot);

	if(datum_is_young(slot)) {
		slot->tag = tag_set_old(tag_set_added(slot->tag, 0));
	}
	if( !tag_is_added(slot->tag) && datum_is_weakcont(slot)) {
		slot->tag = tag_set_added(slot->tag, 1);
		*(gc->orefscur++) = slot;
		if(gc->orefscur == gc->orefslim) {
			_mthrow_(EXCEP_INDEX_OUT_OF_RANGE, "weak refs array overflow");
		}
	}
	return slot;
}



typedef datum_scp (*rootiniter_t) (gcmov_t *gc,  memblk_t *blk, datum_scp d);

datum_scp rootinit_old(gcmov_t *gc,  memblk_t *blk, datum_scp d)
{
	assert(scp_valid(d) && datum_is_generalcls(d)  && "pval");
	datum_scp result = d;


	result = (datum_scp) chcpy_major_old(gc, NIHIL, d, blk);

	return result;
}

datum_scp rootinit_young(gcmov_t *gc,  memblk_t *blk, datum_scp d)
{
	bool old = datum_is_old(d);
	if(old) { return d; };
	descr result = chcpy_minor_young(gc, NIHIL, d, blk);
	logf_debug(Gc_log, "yng roots %s %s %p -> %p  ptr->%p", datum_get_type(result)->asci, (old) ? "OLD" : "YNG", d, result, blk->ptr);
	return result;
}

datum_scp rootinit_young_to_old_0(gcmov_t *gc,  memblk_t *blk, datum_scp d)
{
	assert(scp_valid(d) && datum_is_generalcls(d)  && "pval");
	bool old = datum_is_old(d);
	if(old) { return d; };
	d->tag = tag_set_old(tag_set_added(d->tag, 0));
	descr result = mblk_copyto(blk, d);
	logf_debug(Gc_log, "yng0 roots %s %s %p -> %p  ptr->%p", datum_get_type(result)->asci, (old) ? "OLD" : "YNG", d, result, blk->ptr);
	return result;
}

inline void gc_mov_rootinit(gcmov_t *gc, memblk_t *blk, rootiniter_t initproc)
{
	svm_t vm = gc->prot.vm;
	blk->scan = blk->ptr;
	// *vm_topframe(vm)= (frame_scp) initproc(gc, blk, (datum_scp) *vm_topframe(vm) );
	volatile descr **stk = vm->rstk_begin;
	for (; stk != vm->rstk_ptr; stk++) {
		descr d = **stk;
		if (scp_valid(d)) {
			if(datum_is_forward(d)) {
				**stk = datum_get_forward(d);
				logf_debug(Gc_log, "RSTK: forw %p->%p", d, **stk);

			} else if (datum_is_generalcls(d)) {
#ifdef DBG_MEMMGR
				//if(!datum_is_old(d)) {
					if(mblk_belong(blk, d)) {
						logf_debug(Gc_log, "RSTK: non belong stk %ld stksz %ld obj %p", (stk - vm->rstk_begin), vm_stk_count(vm),d);
						assert(0 && "belongs to 'to'");
					}
		   	//	}
#endif
				**stk = (descr)initproc(gc, blk, (datum_scp) d);
				logf_debug(Gc_log, "RSTK: copy %s %p->%p", datum_get_type(**stk)->asci, d, **stk);
			}

		}
	}

	if(initproc == rootinit_old) {
		descr *pglob, *lim;
		vm_get_globals(vm, &pglob, &lim);

		for(;pglob != lim; pglob++) {
			box_scp d = (box_scp) *pglob;
			if(!scp_valid(d)) {
				continue;
			}


			assert(datum_is_box(d) && "global is box");

			if(scp_valid(d->boxed)) {
				if(datum_is_forward(d->boxed)) {
					d->boxed =  datum_get_forward(d->boxed);
				} else if(datum_is_generalcls(d->boxed)) {
					d->boxed = (descr) initproc(gc, blk, (datum_scp) d->boxed);
				} /*else if(datum_is_globscan(d) && datum_is_nonterm(d)) {
					SLOT_FOREACH(d)
						if(scp_valid(slot) && datum_is_generalcls(slot)) {
							*pslot = (descr) initproc(gc, blk, (datum_scp) slot);
						}
					END_SLOT_FOREACH
				} */
			}
		}
	}


}




void gc_collect(svm_t vm)
{
	gc_mov_minorcollect((gcmov_t*)vm_gc(vm));
}

void gc_mov_minorcollect(gcmov_t *gc)
{
	_try_
		if(mblk_leftsz(&gc->ofrom) < gc->novsz ) {
			log_info(Gc_log, "old space is full. major collection.");
			gc_mov_majorcollect(gc);
		} else {
			// (stage 1) pool to OLD-FROM SPACE
			byte* init_ofrom_ptr = gc->ofrom.ptr; // stat
			ulong init_from_sz = mblk_realsz(&gc->from); // stat
			double msstart =  msecs_relstartup(); // stat
			logf_info(Gc_log, "minor start at: %g ms", msstart);

			assert("newagemax axiom" && imply(gc->newagemax == 0, gc->orefsy2o == gc->orefs));
			datum_scp *dcur = gc->orefs;
			ulong stat_slottraced = 0, stat_oldi_traced = 0;
			mblk_scanset(&gc->ofrom);
			datum_scp d;
			for(;dcur != gc->orefsy2o; dcur++) {
				d = *dcur;
				assert("y2o  not frw" && !datum_is_forward(d));
				assert("y2o  young" && datum_is_young(d));
				chcpy_minor_to_old(gc, NIHIL,  (datum_scp)d, &gc->ofrom);
				stat_oldi_traced++;
			}

			for(;dcur != gc->orefscur; dcur++) {
				d = *dcur;
				datum_set_tag(d, tag_set_added(datum_get_tag(d), 0));
				assert("invptr nonterm" && datum_is_nonterm(d) );
				assert("invptr old" && datum_is_old(d));
				SLOT_FOREACH(d)
					if(scp_valid(slot)) {
						if(datum_is_forward(slot)) {
							*pslot = datum_get_forward(slot);
						} else if(tag_is_young(datum_get_tag(slot))) {
							*pslot = chcpy_minor_to_old(gc, NIHIL, (datum_scp)slot, &gc->ofrom);
						}
					}
					stat_slottraced++;
				END_SLOT_FOREACH
			}
			gc->orefsy2o = gc->orefscur = gc->orefs; // clear o->n pool

			log_debug(Gc_log, "Cheney YNG to OLD" );
			cheney_copy(gc, &gc->ofrom, chcpy_minor_to_old, 0);
			ulong becomeoldsz = gc->ofrom.ptr - init_ofrom_ptr; // stat
			logf_info(Gc_log, "minor I - %ld bytes; slot-traced - %ld; y2o traced - %ld", becomeoldsz, stat_slottraced, stat_oldi_traced);

			// (stage 2): copy young to  to-space and over-aged to old-space
			// incase newagemax == 0 -> all to old;
			if(gc->newagemax != 0) {
				mblk_ptrset(&gc->to);
				gc_mov_rootinit(gc, &gc->to, rootinit_young);
				log_debug(Gc_log, "Cheney YNG to YNG" );
				cheney_copy(gc, &gc->to, chcpy_minor_young, 0);
				gc->orefsy2o = gc->orefscur;
				mblk_swap(&gc->from, &gc->to);
				assert(gc->ofrom.ptr <= gc->ofrom.end && "enough space in old");
			} else {
				gc_mov_rootinit(gc, &gc->ofrom, rootinit_young_to_old_0);
				log_debug(Gc_log, "Cheney YNG to OLD-0" );
				cheney_copy(gc, &gc->ofrom, chcpy_minor_to_old, 0);
				mblk_ptrset(&gc->from);
			}


			ulong collectedsz = init_from_sz - mblk_realsz(&gc->from) - becomeoldsz; // stat

			// statistics:
			logf_info (Gc_log,
					"minor II: dur - %g ms; alive - %g; collected - %ld bytes; grow old - %g; young size - %ld;  young-free - %g; old size %ld",
					msecs_relstartup() - msstart,
					(double)( becomeoldsz + mblk_realsz(&gc->from))/(double)init_from_sz,
					collectedsz,
					(double)( becomeoldsz) / (double) init_from_sz,  mblk_realsz(&gc->from),
					(double)(gc->novsz - mblk_realsz(&gc->from))/(double)gc->novsz,

					mblk_realsz(&gc->ofrom));
		}



	_except_
	_catchall_
		log_excep_severe(Sys_log);
		log_severe(Sys_log, "GC minor collection failed!");
		_rethrow_;
	_end_try_

}


void gc_mov_majorcollect(gcmov_t *gc)
{
		double msstart =  msecs_relstartup(); // stat
		logf_info(Gc_log, "major start at: %g ms", msstart);
		ulong init_ofrom_sz = mblk_realsz(&gc->ofrom) + mblk_realsz(&gc->from);

		gc->orefscur = gc->orefs;
		// copy old to old to-space
		mblk_ptrset(&gc->oto);
		gc_mov_rootinit(gc, &gc->oto, rootinit_old);
#ifdef MEMMGR_WREFS
		cheney_copy(gc, &gc->oto, chcpy_major_old, CHENEY_MISWEAK);
#else
		cheney_copy(gc, &gc->oto, chcpy_major_old, 0);
#endif
		mblk_swap(&gc->ofrom, &gc->oto);
		// update young refs (forwarding)
		gc->from.ptr = gc->from.begin;
#ifdef MEMMGR_WREFS
		major_weak_update(gc);
#endif
		gc->orefscur  =  gc->orefsy2o = gc->orefs ;

		double oldsize = (double)mblk_realsz(&gc->ofrom);
		ulong collectsz =  init_ofrom_sz - oldsize;
		double alive = (double)oldsize/(double)init_ofrom_sz;
		logf_info(Gc_log, "major dur - %g ms; alive - %g; collected - %ld bytes; oldsize - %ld bytes;"
				"free - %g ",
					msecs_relstartup()-msstart,  alive, collectsz, mblk_realsz(&gc->ofrom), (double) (gc->oldsz-oldsize) /(double)gc->oldsz );

		if(mblk_leftsz(&gc->ofrom) < gc->novsz)
		{
			/*ulong nsz = gc->oldsz * 2;
			logf_info(Gc_log, "enlarging old generation up to %lu bytes", nsz);
			memblk_t nfrom = {0};

			mblk_init(&nfrom, tmalloc(nsz * 2), nsz);
			// copy old objects to new old from spce
			gc_mov_rootinit(gc, &nfrom, rootinit_old);
			cheney_copy(gc, &nfrom, chcpy_simple, 0);
			// update young from space - forwarding
			mblk_scansetbegin(&gc->from);
			cheney_copy(gc, &gc->from, NULL );*/


			printf("\nError[GC] old size is too small! major collection failed\n");
			_mthrow_(EXCEP_NO_MEM, "old space is too small");
		}

		if(alive >= gc->live_per_all && gc->oldsz <= gc->limsz) {
			/*
			memblk_t nfrom = {0};
			ulong nsz = gc->oldsz * 2;
			mblk_init(&nfrom, tmalloc(nsz * 2), nsz);
			// copy old objects to new old from spce
			gc_mov_rootinit(gc, &nfrom, rootinit_old);
			cheney_copy(gc, &nfrom, chcpy_simple);
			// update young from space - forwarding
			mblk_scansetbegin(&gc->from);
			cheney_copy(gc, &gc->from, NULL );

			gc->ofrom = nfrom;
			mblk_init(&gc->oto, nfrom.end, nsz);
			gc->oldsz = nsz;
			gc->totalsz = 2*(nsz + gc->novsz);
			log_info(Gc_log, "major: old generation enlarged by 2");
			// memory reclaimation
			free(gc->memold);
			gc->memold = nfrom.begin;
			*/
		}

}




void gc_test(svm_t vm)
{
	decl_gc_of_vm();
	SAVE_RSTK();

	__protect(cons_scp, p, NIHIL);
	__protect(cons_scp, q, NIHIL);
	__protect(descr, z, NIHIL);
	__protect(descr, z1, NIHIL);
	__protect(descr, z2, NIHIL);

	z = new_string_asci(vm,gc, "a");
	z1 = symbol_from_string(vm,new_string_asci(vm,gc, "byaka"));


	q = CONS(z, CONS(CONSCAR(z1), CONSCAR(desc_from_fixnum(4560))));
	z = new_string_asci(vm,gc, "c");

	p = CONS(q, CONS(z, CONSCAR(q)));

	oport_write_asci(Out_console_port, ">>>>>\n");
	oport_print_datum(vm, Out_console_port, p);
	oport_write_asci(Out_console_port, ">>>>>\n");

	gc_collect(vm);

	oport_print_datum(vm, Out_console_port, p);
	oport_write_asci(Out_console_port, ">>>>>\n");
	oport_print_datum(vm, Out_console_port, q);

	FIELD_SET(p, cdr, CONSCAR(CONSCAR(q)));
	gc_collect(vm);
	gc_collect(vm);
	oport_write_asci(Out_console_port, "\n>>>>>\n");
	oport_print_datum(vm, Out_console_port, p);
	oport_write_asci(Out_console_port, ">>>>>\n");
	oport_print_datum(vm, Out_console_port, q);
	oport_write_asci(Out_console_port, "\nend\n");

	UNWIND_RSTK();
}

void gc_minor(svm_t vm)
{
	gc_mov_minorcollect(vm_gc(vm));
}

void gc_major(svm_t vm)
{
	gc_mov_majorcollect(vm_gc(vm));
}

void vm_stk_print(svm_t vm, oport_scp op,  ulong count)
{
	volatile descr **p = vm->rstk_ptr - 1;
	char s[128];
	oport_write_asci(op, "\nvm rstk:");
	for(;count && p >= vm->rstk_begin ; count--, p--) {

		sprintf(s, "\n<%lu>[%p] ", count, **p);
		oport_write_asci(op, s);
		oport_flush(op);
		oport_print_datum(vm,  op, **p);
		oport_flush(op);
	}
	oport_write_asci(op, "\n----------------------------\n");
}

void vm_stk_printall(svm_t vm, oport_scp op)
{
	volatile descr **p = vm->rstk_begin;
	char s[128];
	ulong count  = 0;
	oport_write_asci(op, "\n---------------------------\nvm rstk all:\n");
	for(;p != vm->rstk_ptr ; p ++) {

		sprintf(s, "\n<%lu>[%p] ", count++, **p);
		oport_write_asci(op, s);
		oport_flush(op);
		//oport_begin_limit(op, 350);
		oport_print_datum(vm,  op, **p);
		//oport_end_limit(op);
		oport_flush(op);
	}
	oport_write_asci(op, "\n----------------------------\n");
}



