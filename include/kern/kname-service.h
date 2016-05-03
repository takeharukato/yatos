/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  kernel service relevant definitions                               */
/*                                                                    */
/**********************************************************************/
#if !defined(_KERN_KNAME_SERVICE_H)
#define  _KERN_KNAME_SERVICE_H 

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <kern/config.h>
#include <kern/kernel.h>
#include <kern/param.h>
#include <kern/assert.h>
#include <kern/kern_types.h>
#include <kern/thread-que.h>
#include <kern/thread-sync.h>
#include <kern/lpc.h>
#include <kern/kresv-ids.h>
#include <kern/rbtree.h>

#include <thr/thr-internal.h>

#define KSERV_NAME_LEN  (128)  /* サービス名の長さ  */

#define KSERV_REG_SERVICE   (0) /*<  サービスを登録する       */
#define KSERV_UNREG_SERVICE (1) /*<  サービスの登録を抹消する */
#define KSERV_LOOKUP        (2) /*< サービスを検索する        */

struct _thread;
struct _kern_service;

/** カーネルサービスデータベース
 */
typedef struct _kserv_db{
	spinlock                           lock;  /*< ロック変数                */
	struct _thread                     *thr;  /*< ネームサービススレッド    */
	RB_HEAD(kserv_tree, _kern_service) head;  /*< カーネルサービスのヘッド  */
}kserv_db;

#define __KSERV_DB_INITIALIZER(root)		\
	{					\
		.lock = __SPINLOCK_INITIALIZER,	\
		.thr    = NULL,		        \
		.head   =RB_INITIALIZER(root),  \
	}

/** カーネルサービス情報
 */
typedef struct _kern_service{
	RB_ENTRY(_kern_service) node;  /*< 赤黒木のノード              */
	char    name[KSERV_NAME_LEN];  /*< サービス名                  */
	endpoint                  id;  /*< サービス提供元への通信端点  */
}kern_service;

void kernel_name_service_init(void);
int kns_register_kernel_service(const char *_name);
int kns_unregister_kernel_service(const char *_name);
int kns_lookup_service_by_name(const char *_name, endpoint *_ep);
#endif  /*  _KERN_KNAME_SERVICE_H   */
