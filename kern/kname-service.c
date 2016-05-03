/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  kernel name service routines                                      */
/*                                                                    */
/**********************************************************************/

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <kern/config.h>
#include <kern/kernel.h>
#include <kern/param.h>
#include <kern/kern_types.h>
#include <kern/assert.h>
#include <kern/kprintf.h>
#include <kern/string.h>
#include <kern/errno.h>
#include <kern/spinlock.h>
#include <kern/rbtree.h>
#include <kern/proc.h>
#include <kern/thread.h>
#include <kern/vm.h>
#include <kern/kresv-ids.h>
#include <kern/kname-service.h>
#include <kern/lpc.h>
#include <kern/page.h>

//#define  DEBUG_KNAMEDB

/** サービスDB
 */
static kserv_db service_db=__KSERV_DB_INITIALIZER( &service_db.head );

static int kserv_cmp(struct _kern_service *_a, struct _kern_service *_b);

RB_GENERATE_STATIC(kserv_tree, _kern_service, node, kserv_cmp);

/** サービス名の比較関数
    @param[in] a カーネルサービス情報1
    @param[in] b カーネルサービス情報2
    @retval 0  比較対象のカーネルサービス情報1とカーネルサービス情報2
    名前が等しい
    @retval 負 カーネルサービス情報1の方が辞書順に先の位置にある
    @retval 正 カーネルサービス情報1の方が辞書順に後の位置にある
 */
static int
kserv_cmp(struct _kern_service *a, struct _kern_service *b) {

	kassert( a != NULL );
	kassert( b != NULL );
	kassert( a->name != NULL );
	kassert( b->name != NULL );

	return strcmp(a->name, b->name);
}

/** 名前からエンドポイントを検索する
    @param[in] name サービス名
    @retval    ID_RESV_INVALID     サービスが見つからなかった
    @retval    != ID_RESV_INVALID  サービスのエンドポイント
 */
static endpoint 
lookup_endpoint_by_name(const char *name) {
	endpoint             rc;
	kern_service serv, *res;

	kassert( !ti_in_intr() );
	kassert( name != NULL );

	strncpy(serv.name, name, KSERV_NAME_LEN);
	serv.name[KSERV_NAME_LEN - 1] = '\0';

	spinlock_lock( &service_db.lock );

	res = RB_FIND(kserv_tree, &service_db.head, &serv);
	if ( res == NULL ) {

		rc = ID_RESV_INVALID;  /* サービスが見つからなかった  */
		goto unlock_out;
	}

	rc = res->id;

unlock_out:
	spinlock_unlock( &service_db.lock );

	return rc;
}

/** サービスを検索する
    @param[in] src   送信元エンドポイント
    @param[in] knmsg ネームサービスメッセージ
    @param[in] rmsg  受信したメッセージ
    @param[in] key  検索キー文字列
 */
static void
handle_lookup_msg(endpoint src, kname_service_msg *knmsg, msg_body *rmsg, char *key) {
	endpoint           serv_id;

	kassert( src != LPC_RECV_ANY );
	kassert( rmsg != NULL );

	serv_id = lookup_endpoint_by_name(key);
	if ( serv_id == ID_RESV_INVALID ){
			
		/*  対象のサービスがいない場合  */
		knmsg->rc = -ENOENT;
		goto error_out;
	}

	/*
	 * 見つかったサービスのIDを返す
	 */
	knmsg->rc = 0;
	knmsg->id = serv_id;

error_out:
	lpc_send(src, LPC_INFINITE, rmsg);
}

/** サービスを登録
    @param[in] src   送信元エンドポイント
    @param[in] knmsg ネームサービスメッセージ
    @param[in] rmsg  受信したメッセージ
    @param[in] key   検索キー文字列
    @param[in] id    エンドポイントID
 */
static void
handle_register_msg(endpoint src, kname_service_msg *knmsg, msg_body *rmsg, 
    char *key, endpoint id) {
	kern_service  *servp, *res;

	kassert( !ti_in_intr() );
	kassert( src != LPC_RECV_ANY );
	kassert( rmsg != NULL );
	kassert( key != NULL );
	kassert( id != ID_RESV_IDLE );
	
	if ( id == ID_RESV_INVALID ) {

		/*  IDが不正  */
		knmsg->rc = -EINVAL;
		goto error_out;
	}

	servp = kmalloc( sizeof (kern_service), KMALLOC_NORMAL);
	if ( servp == NULL ) {

		/*  メモリ不足で登録できない  */
		knmsg->rc = -ENOMEM;
		goto error_out;
	}

	/*
	 * サービス名とIDを登録する
	 */
	strcpy(servp->name, key);
	servp->id = id;

	spinlock_lock( &service_db.lock );
	res = RB_INSERT(kserv_tree, &service_db.head, servp);
	spinlock_unlock( &service_db.lock );

	if ( res != NULL ){
		
		/*  対象のサービスが既に登録済み  */
#if defined(DEBUG_KNAMEDB)
	kprintf(KERN_INF, "kernel name service:  endpoint=%d name=%s "
	    "is registerd already\n", 
	    res->id, res->name);
#endif  /*  DEBUG_KNAMEDB  */

		kfree( servp );
		knmsg->rc = -EBUSY;
		goto error_out;
	}

#if defined(DEBUG_KNAMEDB)
	kprintf(KERN_INF, "kernel name service: register endpoint=%d name=%s\n", 
	    servp->id, servp->name);
#endif  /*  DEBUG_KNAMEDB  */

error_out:
	lpc_send(src, LPC_INFINITE, rmsg);
	
	return;
}

/** サービスの登録を解除
    @param[in] src   送信元エンドポイント
    @param[in] knmsg ネームサービスメッセージ
    @param[in] rmsg  受信したメッセージ
    @param[in] key   検索キー文字列
 */
static void
handle_unregister_msg(endpoint src, kname_service_msg *knmsg, msg_body *rmsg, 
    char *key) {
	kern_service  *res;
	kern_service  serv;

	kassert( !ti_in_intr() );
	kassert( src != LPC_RECV_ANY );
	kassert( rmsg != NULL );
	kassert( key != NULL );

	/*
	 * サービス名を検索し, 存在したら消去する
	 */
	strcpy(serv.name, key);

	spinlock_lock( &service_db.lock );

	res = RB_FIND(kserv_tree, &service_db.head, &serv);
	if ( res == NULL ){
			
		/*  対象のサービスが存在しない  */
#if defined(DEBUG_KNAMEDB)
	kprintf(KERN_INF, "kernel name service: service not found name=%s\n", 
	    serv.name);
#endif  /*  DEBUG_KNAMEDB  */
		knmsg->rc = -ENOENT;
		goto unlock_out;
	}

#if defined(DEBUG_KNAMEDB)
	kprintf(KERN_INF, "kernel name service: unregister service "
	    "name=%s endpoint=%u\n", res->name, res->id);
#endif  /*  DEBUG_KNAMEDB  */
	

	RB_REMOVE(kserv_tree, &service_db.head, res);
	kfree( res );

unlock_out:
	spinlock_unlock( &service_db.lock );
	lpc_send(src, LPC_INFINITE, rmsg);
	
	return;
}

/** サービス処理スレッド
 */
static int
kname_service(void __attribute__ ((unused)) *arg) {
	int                     rc;
	thread                *thr;
	endpoint               src;
	msg_body              rmsg;
	kname_service_msg   *knmsg;
	char  name[KSERV_NAME_LEN];
	size_t                 len;

#if defined(DEBUG_KNAMEDB)
	kprintf(KERN_INF, "kernel name service:tid=%d thread=%p\n", 
	    current->tid, current);
#endif  /*  DEBUG_KNAMEDB  */

	knmsg = &rmsg.kname_msg;  /*  カーネルネームサービス情報への参照  */

	while (1) {

		memset( &rmsg, 0, sizeof(msg_body) );

		rc = lpc_recv(LPC_RECV_ANY, LPC_INFINITE, &rmsg, &src);
		kassert( rc == 0 );

		thr = thr_find_thread_by_tid(src);
		if ( thr == NULL ) 
			continue;  /*  送信者が既に終了している  */

		/*
		 * メッセージの内容をコピーする
		 */
		len = ( KSERV_NAME_LEN > knmsg->len ) 
			? ( knmsg->len ) 
			: ( KSERV_NAME_LEN - 1 );

		rc = vm_copy_in(&thr->p->vm, name, knmsg->name, len);
		if ( rc == -EFAULT ) {

			/*  アクセスできない場合は, エラー情報を設定して返却  */
			knmsg->rc = rc;
			lpc_send(src, LPC_INFINITE, &rmsg);
			continue;
		}
		name[len]='\0';

		switch( knmsg->req ) {
			
		case KSERV_REG_SERVICE:
			if ( knmsg->id == ID_RESV_IDLE )  
				knmsg->id = src;
			handle_register_msg(src, knmsg, &rmsg, name, knmsg->id);
			break;
		case KSERV_UNREG_SERVICE:
			handle_unregister_msg(src, knmsg, &rmsg, name);
			break;
		case KSERV_LOOKUP:
			handle_lookup_msg(src, knmsg, &rmsg, name);
			break;
		default:
			/*  引数異常を返却する  */
			knmsg->rc = -EINVAL;
			lpc_send(src, LPC_INFINITE, &rmsg);
			break;
		}
	}

	return 0;
}

/** 自スレッドをカーネルサービス提供者として登録する
    @param[in] name  サービス名
    @retval    0         正常終了
    @retval   -EFAULT    nameがNULL
 */
int
kns_register_kernel_service(const char *name){
	int                  rc;
	msg_body            msg;
	kname_service_msg *nsrv;

	if ( name == NULL ) {
		
		return -EFAULT;
	}

	nsrv = &msg.kname_msg;
	
	memset( &msg, 0, sizeof(msg_body) );
	nsrv->req = KSERV_REG_SERVICE;
	nsrv->name = name;
	nsrv->len = strlen(nsrv->name);
	nsrv->id = current->tid;
	rc = lpc_send_and_reply(ID_RESV_NAME_SERV, &msg);
	kassert( rc == 0 );

	if ( nsrv->rc != 0 )
		return nsrv->rc;

	return 0;
}

/** カーネルサービス登録を破棄する
    @param[in] name  サービス名
    @retval    0         正常終了
    @retval   -EFAULT    nameがNULL
 */
int
kns_unregister_kernel_service(const char *name) {
	int                  rc;
	msg_body            msg;
	kname_service_msg *nsrv;

	if ( name == NULL ) {
		
		return -EFAULT;
	}

	nsrv = &msg.kname_msg;
	
	memset( &msg, 0, sizeof(msg_body) );
	nsrv->req = KSERV_UNREG_SERVICE;
	nsrv->name = name;
	nsrv->len = strlen(nsrv->name);
	nsrv->id = current->tid;
	rc = lpc_send_and_reply(ID_RESV_NAME_SERV, &msg);
	kassert( rc == 0 );

	if ( nsrv->rc != 0 )
		return nsrv->rc;

	return 0;
}

/** サービスを検索する
    @param[in] name    サービス名
    @param[out] ep     エンドポイント格納先アドレス    
    @retval    0       正常終了
    @retval   -EFAULT  nameがNULL
 */
int
kns_lookup_service_by_name(const char *name, endpoint *ep){
	int                  rc;
	msg_body            msg;
	kname_service_msg *nsrv;

	if ( name == NULL ) {
		
		return -EFAULT;
	}

	nsrv = &msg.kname_msg;
	
	memset( &msg, 0, sizeof(msg_body) );
	nsrv->req = KSERV_LOOKUP;
	nsrv->name = name;
	nsrv->len = strlen(nsrv->name);

	rc = lpc_send_and_reply(ID_RESV_NAME_SERV, &msg);
	kassert( rc == 0 );

	if ( nsrv->rc != 0 )
		return nsrv->rc;

	*ep = nsrv->id;

	return 0;
}

/** カーネルサービスの起動
 */
void
kernel_name_service_init(void) {
	int rc;

	spinlock_init( &service_db.lock );
	RB_INIT( &service_db.head );

	rc = thr_new_thread( &service_db.thr );
	kassert( rc == 0 );

	rc = thr_create_kthread(service_db.thr, KSERV_NAME_SERV_PRIO, THR_FLAG_NONE,
	    ID_RESV_NAME_SERV, kname_service, (void *)"KernelNameService");
	kassert( rc == 0 );
	
	rc = thr_start(service_db.thr, current->tid);
	kassert( rc == 0 );

}
