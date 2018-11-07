/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  kernel memory cache routines                                      */
/*                                                                    */
/**********************************************************************/

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include <kern/config.h>
#include <kern/string.h>
#include <kern/kprintf.h>
#include <kern/assert.h>
#include <kern/spinlock.h>
#include <kern/splay.h>
#include <kern/page-kmcache.h>
#include <kern/errno.h>
#include <kern/page.h>

static kmem_cache_db km_cache_db = __KMEM_CACHE_DB_INITIALIZER( &km_cache_db.head ) ;

static int kmem_cache_compare(struct _kmem_cache *_data1, struct _kmem_cache *_data2);

SPLAY_PROTOTYPE(kmcache_db, _kmem_cache, node, kmem_cache_compare);
SPLAY_GENERATE(kmcache_db, _kmem_cache, node, kmem_cache_compare);

/** kmem-cacheの名前を比較する
 */
static int
kmem_cache_compare(struct _kmem_cache *data1, struct _kmem_cache *data2) {

	kassert(data1 != NULL);
	kassert(data2 != NULL);

	return strcmp(data1->name, data2->name);
}

/*
 * assert マクロ群
 */
/** スラブがキュー内に登録されていることを確認する
    @param[in] que    確認対象のキュー
    @param[in] slabp  確認対象のスラブ
    @retval    true   スラブがキュー内に登録されている
    @retval    false  スラブがキュー内に登録されていない
 */
static bool
slab_in_queue(queue *que, slab *slabp) {
	list  *li;
	slab  *sp;

	for(li = queue_ref_top(que);
	    li != (list *)que;
	    li = li->next) {
		
		sp = CONTAINER_OF(li, struct _slab, link);
		if (sp == slabp)
			return true;
	}
	
	return false;
}

/** kmem_cacheの登録の有無を確認する
    @param[in] kcp    確認対象のkmem_cache
    @param[in] exists 
               true   kmem_cahceのDBに登録されていることを確認する
               false  kmem_cahceのDBに登録されていないことを確認する
 */
static void
assert_kmem_cache_valid(kmem_cache *kcp, bool exists) {
	struct _kmem_cache       *res;
	intrflags             flags;

	kassert(kcp != NULL);

	spinlock_lock_disable_intr(&km_cache_db.lock, &flags);
	res = SPLAY_FIND(kmcache_db, &km_cache_db.head, kcp);

	if (exists) 
		kassert(res != NULL);
	else
		kassert(res == NULL);
	spinlock_unlock_restore_intr(&km_cache_db.lock, &flags);	
}
/** スラブのkmem_cacheへの登録状況を確認する
    @param[in] slabp    確認対象のスラブ
    @param[in] exists 
               true   kmem_cahceに登録されていることを確認する
               false  kmem_cahceに登録されていないことを確認する
 */
static void
assert_slab_valid(slab *slabp, bool exists) {

	kassert(slabp != NULL);
	kassert(slabp->kmcache_ref != NULL);
	assert_kmem_cache_valid(slabp->kmcache_ref, true);


	if ( exists ) {
		kassert(spinlock_locked_by_self(&slabp->kmcache_ref->lock));
		kassert( slab_in_queue(&slabp->kmcache_ref->partial, slabp) ||
			 slab_in_queue(&slabp->kmcache_ref->full, slabp) ||
			 slab_in_queue(&slabp->kmcache_ref->free, slabp) );
	}
}

/** 所定のデータを格納するために必要なページ数を算出する(単位:ページオーダ)
    @param[in] size 格納するデータのサイズ
    @param[out] res 必要なページ数(単位:ページオーダ)
 */
void
calc_page_order(size_t size, page_order *res) {
	page_order   order;
	uintptr_t nr_pages;

	kassert( res != NULL );

	if (size == 0) {
		
		*res = 0;
		return;
	}

	nr_pages = ( PAGE_ALIGNED( size ) ? ( size ) : PAGE_NEXT( size ) ) >> PAGE_SHIFT;
	
	for( order = PAGE_POOL_MAX_ORDER; order > 0; --order) {
		
		if ( nr_pages & ( 1 << ( order - 1 ) ) ) {

			*res = ( order - 1 );
			return;
		}
	}
	
	return;
}

/** kmem-cacheを登録する
    @param[in]      kcp 登録するキャッシュ
    @retval  0      正常に登録した
    @retval -EBUSY  すでに登録済みのキャッシュを多重に登録しようとした
 */
static int
add_kmem_cache_into_db(struct _kmem_cache *kcp) {
	int                        rc;
	intrflags               flags;
	struct _kmem_cache       *res;
	
	kassert(kcp != NULL);

	spinlock_lock_disable_intr( &km_cache_db.lock, &flags);

	res = SPLAY_FIND(kmcache_db, &km_cache_db.head, kcp);
	if (res != NULL) {

		rc = -EBUSY;
		goto unlock_out;
	}

	res = SPLAY_INSERT(kmcache_db, &km_cache_db.head, kcp);
	kassert(res == NULL);

	rc = 0;

unlock_out:
	spinlock_unlock_restore_intr(&km_cache_db.lock, &flags );

	return rc;
}

/** kmem-cacheの登録を抹消する
    @param[in]      kcp 登録するキャッシュ
    @retval  0      正常に登録した
    @retval -EBUSY  すでに登録済みのキャッシュを多重に登録しようとした
 */
static int
remove_kmem_cache_from_db(struct _kmem_cache *kcp) {
	int                   rc;
	intrflags          flags;
	struct _kmem_cache  *res;

	assert_kmem_cache_valid(kcp, true);

	spinlock_lock_disable_intr(&km_cache_db.lock, &flags);

	res = SPLAY_FIND(kmcache_db, &km_cache_db.head, kcp);
	if (res == NULL) {

		rc = -ENOENT;
		goto unlock_out;
	}

	SPLAY_REMOVE(kmcache_db, &km_cache_db.head, kcp);

	rc = 0;

unlock_out:
	spinlock_unlock_restore_intr( &km_cache_db.lock, &flags);

	return rc;
}

/** スラブが使用する領域長を計算する
    @param[in] kcp スラブが所属するkmem_cache
    @return    スラブが使用する領域長(単位:バイト)
 */
static size_t
calc_slab_data_size(kmem_cache *kcp) {
	
	kassert( kcp->obj_size > 0 );

	return sizeof(slab_obj_head) + _kmem_calc_align_offset(kcp) + kcp->obj_size;
}

/** キャッシュカラーリング値を設定する
    @param[in] kcp    設定対象のkmem_cache
 */
static void
setup_color_param( kmem_cache *kcp ) {
	uintptr_t color_area_size;
	size_t l1_dcache_line_byte;

	kassert(kcp != NULL);

	color_area_size = kcp->slab_size - sizeof(slab) 
		- ( kcp->objs_per_slab * calc_slab_data_size(kcp) );

	l1_dcache_line_byte =  DEFAULT_L1_CACHE_BYTE;

	if ( color_area_size < l1_dcache_line_byte ) {
	
		kcp->color_num = 0;
	} else {

		kcp->color_num = color_area_size / l1_dcache_line_byte  + 1;
	}

	kcp->color_next = 0;	
}

/** 次に作成するslabのキャッシュカラーリングオフセットを設定する
    @param[in] kcp    設定対象のkmem_cache
 */
static size_t
update_color_offset( kmem_cache *kcp ) {
	size_t offset;
	size_t l1_dcache_line_byte;

	assert_kmem_cache_valid(kcp, true);

	if ( kcp->color_num == 0 )
		return 0;

	l1_dcache_line_byte = DEFAULT_L1_CACHE_BYTE;

	offset = kcp->color_next * l1_dcache_line_byte;
	kcp->color_next = (kcp->color_next + 1) % kcp->color_num;

	return offset;
}

/** スラブを初期化する
    @param[in] kcp          スラブが所属するkmem_cache
    @param[in] slabp        初期化対象のスラブ
    @param[in] page         スラブを配置するページのカーネル仮想アドレス
    @param[in] color_offset カラーリングオフセット
 */
static void
init_slab(kmem_cache *kcp, slab *slabp, void *page, size_t color_offset) {

	assert_kmem_cache_valid(kcp, true);

	kassert( spinlock_locked_by_self( &kcp->lock ) );
	kassert( slabp != NULL );
	
	list_init( &slabp->link );
	queue_init( &slabp->objects );
	spinlock_init( &slabp->lock );

	slabp->kmcache_ref = kcp;
	slabp->count = 0;
	slabp->color_offset = color_offset;
	slabp->page = page;
}

/** オフスラブのサイズを算出する
    @param[in] kcp    算出対象のkmem_cache
    @param[in] sizep  算出値格納先アドレス
 */
static void
calc_off_slab_size(kmem_cache *kcp, size_t *sizep) {
	page_order       min_order;
	page_order           order;
	size_t           area_size;
	size_t            min_size;
	size_t        last_remains;
	size_t           last_size;
	size_t         cur_remains;
	size_t            cur_size;

	kassert( sizep != NULL );

	min_order = 0;
	min_size = sizeof(slab) + calc_slab_data_size(kcp);
	calc_page_order( min_size, &min_order );

	last_size = PAGE_SIZE << min_order;
	area_size = last_size - sizeof(slab) ;
	last_remains = area_size % calc_slab_data_size(kcp);

	for(order = min_order ; order  < PAGE_POOL_MAX_ORDER; ++order) {

		cur_size = PAGE_SIZE << order;
		area_size = cur_size - sizeof( slab );
		cur_remains = area_size % calc_slab_data_size( kcp );

		if ( cur_remains < last_remains ) {
			
			last_remains = cur_remains;
			last_size = cur_size;
		}
	}
	
	*sizep = last_size;
}

/** スラブのパラメタ値を計算する
    @param[in] kcp    算出対象のkmem_cache
 */
static void
calc_slab_param(kmem_cache *kcp) {

	kassert(kcp != NULL);

	kcp->slab_size = PAGE_SIZE;
	if ( _kmem_is_off_slab(kcp) )
		calc_off_slab_size(kcp, &kcp->slab_size);
	
	kcp->objs_per_slab =
		( kcp->slab_size - sizeof(slab) ) / calc_slab_data_size(kcp) ;

	return;
}
/** スラブ中のオブジェクトのヘッダを参照する
    @param[in] kcp   操作対象のkmem_cache
    @param[in] slabp スラブ管理情報のアドレス
    @param[in] index 何番目のオブジェクトのヘッダを参照するかを指定するインデクス値
    @return 指定したオブジェクトのヘッダ情報
 */
static slab_obj_head *
refer_object_head(kmem_cache *kcp, slab *slabp, obj_cnt_type index) {
	slab_obj_head   *hp;
	uintptr_t base;

	assert_kmem_cache_valid(kcp, true);
	assert_slab_valid(slabp, false);

	base = (uintptr_t)( ((slab *)slabp) + 1);
	hp = (slab_obj_head *)( base + ( index *  calc_slab_data_size(kcp) ) );

	return hp;
}

/** スラブ中の全オブジェクトのヘッダを初期化する
    @param[in] kcp   操作対象のkmem_cache
    @param[in] slabp スラブ管理情報のアドレス
 */
static void
init_object_heads(kmem_cache *kcp, slab *slabp){
	obj_cnt_type         i;
	slab_obj_head      *hp;

	assert_kmem_cache_valid(kcp, true);

	kassert(spinlock_locked_by_self(&kcp->lock));
	kassert(spinlock_locked_by_self(&slabp->lock));

	for(i = 0; i < kcp->objs_per_slab; ++i) {

		hp = refer_object_head(kcp, slabp, i);
		list_init(&hp->link);
		queue_add(&slabp->objects, &hp->link);
	}
}

/** kmem_cacheを伸張する(実装部)
    @param[in] kcp    伸張対象のkmem_cache
    @param[in] sflags slab獲得フラグ
    @retval    0      正常に伸張した
    @retval   -ENOMEM メモリの獲得に失敗した
 */
static int 
kmem_cache_grow_nolock( kmem_cache *kcp, slab_flags __attribute__ ((unused)) sflags){
	int                    rc;
	void                  *pp;
	void              *kvaddr;
	slab               *slabp;
	size_t       color_offset;
	page_frame           *pgf;
	page_order          order;

	kassert( PAGE_ALIGNED(kcp->slab_size) );

	order = 0;
	calc_page_order(kcp->slab_size, &order);
	rc = alloc_buddy_pages( &kvaddr,  order, 0 );
	if ( rc != 0 ) {
		
		rc = -ENOMEM;
		goto error_out;
	}

	color_offset = update_color_offset( kcp );
	slabp = (slab *)( kvaddr +  color_offset );

	init_slab(kcp, slabp, kvaddr, color_offset);

	spinlock_lock(&slabp->lock);

	init_object_heads(kcp, slabp);

	spinlock_unlock(&slabp->lock);
	queue_add(&kcp->free, &slabp->link);
	++kcp->slab_count;

	for(pp = kvaddr; (uintptr_t)pp < ( (uintptr_t)kvaddr + kcp->slab_size ); 
	    pp += PAGE_SIZE) {

		//pgf = kmem_object_to_page(pp);
		rc = kvaddr_to_page_frame((void *)pp, &pgf);
		pgf->slabp = slabp;
	}

	rc = 0;

error_out:
	return rc;
}

/** kmem_cacheからメモリを割当てる
    @param[in] kcp    メモリ獲得元のkmem_cache
    @param[in] sflags スラブ獲得条件
    @return 獲得したメモリ領域のアドレス
 */
void *
kmem_cache_alloc( kmem_cache *kcp, slab_flags __attribute__ ((unused)) sflags ) {
	int              rc;
	intrflags     flags;
	slab         *slabp;
	list            *li;
	list       *head_li;
	void           *obj;
	slab_obj_head        *hp;

	spinlock_lock_disable_intr(&kcp->lock, &flags);
	if ( !queue_is_empty( &kcp->partial ) ) {

		li = queue_ref_top( &kcp->partial );
	} else {

		if ( queue_is_empty( &kcp->free ) ) {

			rc = kmem_cache_grow_nolock( kcp, sflags );
			if ( rc != 0 ) {
				
				obj = NULL;
				goto unlock_out;
			}
		}

		li = queue_get_top(&kcp->free);
		queue_add(&kcp->partial, li);
	}

	slabp = CONTAINER_OF(li, slab, link);

	kassert(slabp->count < kcp->objs_per_slab);

	head_li = queue_get_top( &slabp->objects );
	hp = CONTAINER_OF(head_li, slab_obj_head, link);
	
	++slabp->count;
	if ( slabp->count == kcp->objs_per_slab ) {
		
		list_del( &slabp->link );
		queue_add( &kcp->full, &slabp->link );
	}

	obj = (void *)( (uintptr_t)hp + sizeof(slab_obj_head) + _kmem_calc_align_offset(kcp) );

	if ( kcp->constructor != NULL )
		kcp->constructor( obj, kcp->obj_size );	

unlock_out:
	spinlock_unlock_restore_intr( &kcp->lock, &flags );

	return obj;
}

/** kmem_cacheにメモリを返却する
    @param[in] kcp    メモリ獲得元のkmem_cache
    @param[in] obj    返却するメモリ
 */
void
kmem_cache_free( kmem_cache *kcp, void *obj ) {
	intrflags  flags;
	slab      *slabp;
	slab_obj_head  *hp;

	spinlock_lock_disable_intr( &kcp->lock, &flags );
	if ( kcp->destructor != NULL )
		kcp->destructor(obj, kcp->obj_size);

	slabp = _kmem_obj_to_slab(obj);

	hp = (slab_obj_head *)( ( (uintptr_t)obj ) 
	    - _kmem_calc_align_offset(kcp) - sizeof(slab_obj_head) );

	spinlock_lock(&slabp->lock);
	queue_add(&slabp->objects, &hp->link);
	if (slabp->count == kcp->objs_per_slab) {
		
		list_del(&slabp->link);
		queue_add(&kcp->partial, &slabp->link);
	}

	--slabp->count;

	if (slabp->count == 0) {

		list_del(&slabp->link);
		queue_add(&kcp->free, &slabp->link);
	}
	spinlock_unlock(&slabp->lock);

	spinlock_unlock_restore_intr(&kcp->lock, &flags);
}

/** kmem_cache管理情報を獲得する
    @return 非NULL kmem_cache管理情報のアドレス
    @return NULL   メモリ不足による獲得失敗
 */
static kmem_cache *
alloc_kmem_cache(void) {
	
	return kmalloc(sizeof(kmem_cache), 0);
}

/** kmem_cache管理情報を開放する
    @param[in] ref 開放するkmem_cache管理情報のアドレス
 */
static void 
free_kmem_cache(void *ref){
	
	kfree(ref);
}

/** スラブオブジェクトのメモリアラインを算出する
    @param[in] kcp 算出対象のkmem_cache
 */
uintptr_t 
_kmem_calc_align_offset(kmem_cache *kcp) {

	kassert(kcp != NULL);
	kassert(kcp->obj_size > 0);

	return ( (kcp->obj_size % kcp->align) ? 
	    (kcp->align - ( kcp->obj_size % kcp->align ) ) : (0) );	
}

/** オフスラブのメモリアドレスからslab管理情報を取得する
    @param[in] obj 割当てたメモリアドレス
    @return slab管理情報のアドレス
 */
slab *
_kmem_obj_to_slab(void *obj) {
	int           rc;
	page_frame  *pgf;

	rc = kvaddr_to_page_frame((void *)obj, &pgf);
	kassert( rc == 0 );
	
	return pgf->slabp;
}

/** kmem_cacheのスラブがオフスラブキャッシュの場合真を返す
    @param[in] kcp 調査対象のkmem_cache
    @retval 真 kmem_cacheのスラブがオフスラブである
    @retval 偽 kmem_cacheのスラブがオフスラブでない
 */
bool
_kmem_is_off_slab(kmem_cache *kcp) {
	bool val;

	kassert(kcp != NULL);

	val = ( ( PAGE_SIZE / KM_SLAB_TYPE_DIVISOR ) <= kcp->obj_size  );

	return val;
}

/** kmem_cacheの管理ツリーから指定した名前のkmem_cache管理情報を検索する
    @param[in] name kmem_cacheの名前
    @return 非NULL 見つかったkmem_cache管理情報
    @return NULL   指定された名前のkmem_cacheが登録されていない
 */
kmem_cache *
get_kmem_cache(const char *name) {
	kmem_cache          *kcp;
	kmem_cache         dummy;
	intrflags        flags;

	kassert(name != NULL);

	spinlock_lock_disable_intr(&km_cache_db.lock, &flags);
	dummy.name = name;
	kcp = SPLAY_FIND(kmcache_db, &km_cache_db.head, &dummy);
	spinlock_unlock_restore_intr(&km_cache_db.lock, &flags);
	
	if (kcp == NULL) 
		goto error_out;

	spinlock_lock(&kcp->lock);
	++kcp->usage_count;
	spinlock_unlock(&kcp->lock);

error_out:
	return kcp;
}

/** kmem_cacheを開放する
    @param[in] kcp 開放対象のkmem_cache
 */
void 
put_kmem_cache(kmem_cache *kcp){
	intrflags        flags;

	kassert(kcp != NULL);

	spinlock_lock_disable_intr(&kcp->lock, &flags);

	kassert(kcp->usage_count > 0);

	--kcp->usage_count;

	if ( kcp->sflags & KM_SLAB_PREDEFINED_CACHE )
		goto unlock_out;

	if ( ( kcp->usage_count == 0 ) && (kcp->state & KM_STATE_DELAY_DESTROY) ) {

		spinlock_unlock_restore_intr(&kcp->lock, &flags);		
		free_kmem_cache(kcp);
		return;
	}

unlock_out:
	spinlock_unlock_restore_intr(&kcp->lock, &flags);
}

/** kmem_cacheを初期化する
    @param[in] kcp         初期化対象のkmem_cache
    @param[in] name        kmem_cacheの名前
    @param[in] size        slabのサイズ(単位:バイト)
    @param[in] sflags      slabの属性情報
    @param[in] align       slabのアラインメント
    @param[in] limits      slabキャッシュの保持数
    @param[in] constructor slab割当て時の後処理関数
    @param[in] destructor  slab開放時の前処理関数 
    @retval    0           正常割り当て完了
    @retval   -ENOMEM      メモリ不足により初期化失敗
*/
int
kmem_cache_init(kmem_cache *kcp, const char *name, size_t size, slab_flags sflags, 
    uintptr_t align, obj_cnt_type limits,
    void (*constructor)(void *, size_t), void (*destructor)(void *, size_t)) {
	int rc;

	kassert(kcp != NULL);
	kassert(size > 0);

	spinlock_init(&kcp->lock);
	kcp->name = name;
	SPLAY_LEFT(kcp, node) = kcp;
	SPLAY_RIGHT(kcp, node) = kcp;

	kcp->obj_size = size;
	if ( align != KM_ALIGN_HW )
		kcp->align = align;
	else
		kcp->align = sizeof(void *);

	calc_slab_param(kcp); /* slab_size/objs_per_slab  */

	setup_color_param( kcp );

	kcp->sflags = 0;
	if ( _kmem_is_off_slab(kcp) )
		kcp->sflags |= KM_SLAB_ON_SLAB;
	else
		kcp->sflags |= KM_SLAB_OFF_SLAB;

	kcp->sflags |= sflags;

	queue_init(&kcp->partial);
	queue_init(&kcp->full);
	queue_init(&kcp->free);
	
	kcp->slab_count = 0;
	kcp->usage_count = 0;
	kcp->state = 0;
	if (limits == KM_LIMIT_DEFAULT) 
		kcp->limits = KM_DEFAULT_LIMIT;
	else
		kcp->limits = limits;

	if (constructor != NULL)
		kcp->constructor = constructor;

	if (destructor != NULL)
		kcp->destructor = destructor;

	rc = add_kmem_cache_into_db(kcp);
	if ( ( rc != 0 ) && ( !( kcp->sflags & KM_SLAB_PREDEFINED_CACHE ) ) )
		free_kmem_cache(kcp);

	return rc;
}

/** kmem_cacheにslabを追加する
    @param[in] kcp    操作対象のkmem_cache
    @param[in] sflags スラブの属性情報
    @retval    0      正常に追加した
    @retval   -ENOMEM メモリの獲得に失敗した
 */
int 
_kmem_cache_grow( kmem_cache *kcp, slab_flags __attribute__ ((unused)) sflags){
	int                    rc;
	intrflags         flags;

	spinlock_lock_disable_intr(&kcp->lock, &flags);
	rc = kmem_cache_grow_nolock( kcp, sflags);
	spinlock_unlock_restore_intr(&kcp->lock, &flags);

	return rc;
}

/** kmem_cacheのslabを開放する
    @param[in] kcp    操作対象のkmem_cache
    @param[in] force  強制的に開放する場合は真を設定
    @retval    0      正常に開放した
 */
int 
kmem_cache_reap( kmem_cache *kcp, bool force) {
	int           rc;
	intrflags  flags;
	list         *li;
	slab      *slabp;
	void         *pp;
	page_frame  *pgf;

	spinlock_lock_disable_intr(&kcp->lock, &flags);

	if ( queue_is_empty(&kcp->free) ) {

		rc = -EBUSY;
		goto unlock_out;
	}

	while( !queue_is_empty(&kcp->free) ) {

		if ( ( !force ) && ( kcp->slab_count < kcp->limits ) ) 
			break;

		kassert(kcp->slab_count > 0);

		li = queue_get_top(&kcp->free);

		slabp = CONTAINER_OF(li, slab, link);

		list_del(li);

		for(pp = slabp->page; 
		    (uintptr_t)pp < ( (uintptr_t)(slabp->page) + kcp->slab_size); 
		    pp += PAGE_SIZE) {
			
			rc = kvaddr_to_page_frame((void *)pp, &pgf);			
			kassert( rc == 0 );

			pgf->slabp = NULL;
		}

		free_buddy_pages( slabp->page );
		--kcp->slab_count;
	}

	rc = 0;

unlock_out:
	spinlock_unlock_restore_intr(&kcp->lock, &flags);

	return rc;
}

/** kmem_cacheを作成する
    @param[in] name        kmem_cacheの名前
    @param[in] size        slabのサイズ(単位:バイト)
    @param[in] sflags      slabの属性情報
    @param[in] align       slabのアラインメント
    @param[in] limits      slabキャッシュの保持数
    @param[in] constructor slab割当て時の後処理関数
    @param[in] destructor  slab開放時の前処理関数 
    @retval    0           正常割り当て完了
    @retval   -ENOMEM      メモリ不足により作成失敗
 */
int
kmem_cache_create(const char *name, size_t size, uintptr_t align, obj_cnt_type limits,
    void (*constructor)(void *, size_t), void (*destructor)(void *, size_t)) {
	int rc;
	kmem_cache *kcp;

	kcp = alloc_kmem_cache();
	if (kcp == NULL)
		return -ENOMEM;

	rc = kmem_cache_init(kcp, name, size, align, limits, 0, constructor, destructor);
		
	return rc;
}

/** kmem_cacheを開放する
    @param[in] name        kmem_cacheの名前
    @param[in] size        slabのサイズ(単位:バイト)
    @param[in] sflags      slabの属性情報
    @param[in] align       slabのアラインメント
    @param[in] limits      slabキャッシュの保持数
    @param[in] constructor slab割当て時の後処理関数
    @param[in] destructor  slab開放時の前処理関数 
    @retval    0           正常割り当て完了
    @retval   -ENOMEM      メモリ不足により作成失敗
 */
void
kmem_cache_destroy(kmem_cache *kcp) {
	int                 rc;
	intrflags        flags;

	kassert(kcp != NULL);

	spinlock_lock_disable_intr(&kcp->lock, &flags);

	if ( kcp->sflags & KM_SLAB_PREDEFINED_CACHE )
		goto unlock_out;

	if ( !queue_is_empty(&kcp->partial) || !queue_is_empty(&kcp->full) )
		goto unlock_out;
		
	remove_kmem_cache_from_db(kcp);

	if ( kcp->usage_count == 0 ) {
		
		spinlock_unlock_restore_intr(&kcp->lock, &flags);

		rc = kmem_cache_reap( kcp, 1);
		kassert( rc == 0 );

		free_kmem_cache(kcp);
		return;
	} else {

		kcp->state |= KM_STATE_DELAY_DESTROY;
	}

unlock_out:
	spinlock_unlock_restore_intr(&kcp->lock, &flags);
}


