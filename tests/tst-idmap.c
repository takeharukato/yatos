/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  Test program for id bitmap routines                               */
/*                                                                    */
/**********************************************************************/

#include <stddef.h>
#include <stdint.h>

#include <kern/config.h>
#include <kern/assert.h>
#include <kern/string.h>
#include <kern/kprintf.h>
#include <kern/errno.h>
#include <kern/id-bitmap.h>

#include <kern/tst-progs.h>

/** IDビットマップ大域変数
 */
static id_bitmap g_bmap=__ID_BITMAP_INITIALIZER(ID_BITMAP_DEFAULT_RESV_IDS);

/** IDビットマップのテスト
    @param[in] name テスト名
    @param[in] idmap テスト対象のIDビットマップ
 */
static void
do_idmap_test(char *name, id_bitmap *idmap){
	int            rc;
	obj_id      newid;
	obj_id     min_id;
	obj_id     max_id;
	obj_id         id;

	newid = 0;

	kprintf(KERN_INF, "%s-1 get system id\n", name);
	rc = idbmap_get_id(idmap, ID_BITMAP_SYSTEM, &newid);
	kprintf(KERN_INF, "%s-1 get system id = %u rc=%d\n", name, newid, rc);
	kassert( rc == 0 );
	kassert( newid > 0 );

	kprintf(KERN_INF, "%s-2 put system id\n", name);
	idbmap_put_id(idmap, newid);

	kprintf(KERN_INF, "%s-3 specific system id (%u)\n", name, newid);
	rc = idbmap_get_specified_id(idmap, newid, ID_BITMAP_SYSTEM);
	kprintf(KERN_INF, "%s-3 get system id = %u rc=%d\n", name, newid, rc);
	kassert( rc == 0 );

	kprintf(KERN_INF, "%s-4 get busy bitmap free test\n", name);
	rc = idbmap_free(idmap);
	kassert( rc == -EBUSY );

	kprintf(KERN_INF, "%s-5 get busy system id\n", name);
	rc = idbmap_get_specified_id(idmap, newid, ID_BITMAP_SYSTEM);
	kassert( rc == -EBUSY );

	kprintf(KERN_INF, "%s-6 get invalid range ( too big ) system id\n", name);
	rc = idbmap_get_specified_id(idmap, idmap->nr_ids, ID_BITMAP_SYSTEM);
	kassert( rc == -EINVAL );

	kprintf(KERN_INF, "%s-7 get invalid range ( user id ) system id\n", name);
	rc = idbmap_get_specified_id(idmap, idmap->reserved_ids, ID_BITMAP_SYSTEM);
	kassert( rc == -EINVAL );

	kprintf(KERN_INF, "%s-8 put system id\n", name);
	idbmap_put_id(idmap, newid);

	min_id = ID_BITMAP_FIRST_VALID_ID;
	max_id = idmap->reserved_ids;
	kprintf(KERN_INF, "%s-9 get all system id range\n", name);
	for( id = min_id; max_id > id; ++id) {

		rc = idbmap_get_specified_id(idmap, id, ID_BITMAP_SYSTEM);
		if ( rc != 0 )
			kprintf(KERN_INF, "%s-9 get system id = %u rc=%d\n", 
			    name, id, rc);
		kassert( rc == 0 );
	}

	min_id = ID_BITMAP_FIRST_VALID_ID;
	max_id = idmap->reserved_ids;
	kprintf(KERN_INF, "%s-10 free all system id range\n", name);
	for( id = min_id; max_id > id; ++id) {

		idbmap_put_id(idmap, id);
		if ( rc != 0 )
			kprintf(KERN_INF, "%s-10 free system id = %u rc=%d\n", 
			    name, id, rc);
		kassert( rc == 0 );
	}

	min_id = idmap->reserved_ids;
	max_id = ID_BITMAP_DEFAULT_MAP_SIZE*2 - idmap->reserved_ids;
	kprintf(KERN_INF, "%s-11 get user id range\n", name);
	for( id = min_id; max_id > id; ++id) {

		rc = idbmap_get_specified_id(idmap, id, ID_BITMAP_USER);
		if ( rc != 0 )
			kprintf(KERN_INF, "%s-11 get user id = %u rc=%d\n", 
			    name, id, rc);
		kassert( rc == 0 );
	}

	min_id = idmap->reserved_ids;
	max_id = ID_BITMAP_DEFAULT_MAP_SIZE*2 - idmap->reserved_ids;
	kprintf(KERN_INF, "%s-12 free all user id range\n", name);
	for( id = min_id; max_id > id; ++id) {

		idbmap_put_id(idmap, id);
		if ( rc != 0 )
			kprintf(KERN_INF, "%s-12 free user id = %u rc=%d\n", 
			    name, id, rc);
		kassert( rc == 0 );
	}

	min_id = idmap->reserved_ids;
	max_id = idmap->nr_ids;
	kprintf(KERN_INF, "%s-13 get user id range with get_id \n", name);
	for( id = min_id; max_id > id; ++id) {

		rc = idbmap_get_id(idmap, ID_BITMAP_USER, &newid);
		if ( rc != 0 )
			kprintf(KERN_INF, "%s-13 get user id rc=%d\n", 
			    name, rc);
		kassert( rc == 0 );
	}

	min_id = idmap->reserved_ids;
	max_id = newid + 1;
	kprintf(KERN_INF, "%s-14 free all user id range\n", name);
	for( id = min_id; max_id > id; ++id) {

		idbmap_put_id(idmap, id);
		if ( rc != 0 )
			kprintf(KERN_INF, "%s-14 free user id = %u rc=%d\n", 
			    name, id, rc);
		kassert( rc == 0 );
	}

	rc = idbmap_resize(idmap, 0);
	kprintf(KERN_INF, "%s-15 shrink map error case zero\n", name);
	kassert( rc == -EINVAL );

	rc = idbmap_resize(idmap, idmap->reserved_ids);
	kprintf(KERN_INF, "%s-15 shrink map no user id error case \n", name);
	kassert( rc == -EINVAL );

	newid = idmap->reserved_ids + 1;
	rc = idbmap_get_specified_id(idmap, newid, ID_BITMAP_USER);
	kassert( rc == 0 );

	rc = idbmap_resize(idmap, newid);
	kprintf(KERN_INF, "%s-16 shrink map busy user id error case \n", name);
	kassert( rc == -EBUSY );

	idbmap_put_id(idmap, newid);

	rc = idbmap_resize(idmap, idmap->reserved_ids+1);
	kprintf(KERN_INF, "%s-17 shrink map minimal \n", name);
	kassert( rc == 0 );

	kprintf(KERN_INF, "%s-18 free map\n", name);
	rc = idbmap_free(idmap);
	kassert( rc == 0 );
}

/** ローカル変数ビットマップを使用したテスト
 */
static void
idmap_test1(void){
	id_bitmap bmap;

	idbmap_init(&bmap, ID_BITMAP_DEFAULT_RESV_IDS);
	kprintf(KERN_INF, "Test1: local bitmap variable\n");
	do_idmap_test("Test1", &bmap);
}

/** 静的大域変数ビットマップを使用したテスト
 */
static void
idmap_test2(void){

	kprintf(KERN_INF, "Test2: Static global bitmap variable\n");
	do_idmap_test("Test2", &g_bmap);
}

/** 動的確保したビットマップを使用したテスト
 */
static void
idmap_test3(void){
	int           rc;
	obj_id     newid;
	obj_id  failedid;
	id_bitmap *idmap;

	kprintf(KERN_INF, "Test3: Dynamic global bitmap variable\n");

	kprintf(KERN_INF, "Test3-pre1 create id bitmap\n");
	rc = idbmap_create(ID_BITMAP_DEFAULT_RESV_IDS, &idmap);
	kassert( rc == 0 );

	rc = idbmap_get_id(idmap, ID_BITMAP_SYSTEM, &newid);
	kassert( rc == 0 );

	kprintf(KERN_INF, "Test3-pre2 make id bitmap destroyed. \n");
	idbmap_destroy(idmap);

	kprintf(KERN_INF, "Test3-pre3 idbmap_free invalid id bitmap type\n");
	rc = idbmap_free(idmap);
	kassert( rc == -EINVAL );

	kprintf(KERN_INF, "Test3-pre4 idbmap_get_specified_id in deleted idbmap \n");
	rc = idbmap_get_specified_id(idmap, newid + 1, ID_BITMAP_SYSTEM);
	kassert( rc == -ENOENT );

	kprintf(KERN_INF, "Test3-pre5 idbmap_get_id in deleted idbmap \n");
	rc = idbmap_get_id(idmap, ID_BITMAP_SYSTEM, &failedid);
	kassert( rc == -ENOENT );

	kprintf(KERN_INF, "Test3-pre6 idbmap_resize deleted idbmap \n");
	rc = idbmap_resize(idmap, idmap->reserved_ids + 1);
	kassert( rc == -ENOENT );

	kprintf(KERN_INF, "Test3-pre7 idbmap_put_id in deleted idbmap \n");
	idbmap_put_id(idmap, newid);

	kprintf(KERN_INF, "Test3-pre8 create id bitmap\n");
	rc = idbmap_create(ID_BITMAP_DEFAULT_RESV_IDS, &idmap);
	kassert( rc == 0 );

	do_idmap_test("Test3", idmap);

	kprintf(KERN_INF, "Test3-post1 destroy id bitmap\n");
	idbmap_destroy(idmap);
}

/** IDビットマップテスト
 */
void
idbmap_test(void){

	idmap_test1();
	idmap_test2();
	idmap_test3();
}
