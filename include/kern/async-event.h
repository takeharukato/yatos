/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  Asynchronous event relevant definitions                           */
/*                                                                    */
/**********************************************************************/
#if !defined(_KERN_ASYNC_EVENT_H)
#define  _KERN_ASYNC_EVENT_H 

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <kern/config.h>
#include <kern/kernel.h>
#include <kern/param.h>
#include <kern/kern_types.h>
#include <kern/spinlock.h>
#include <kern/queue.h>

#include <hal/traps.h>
#include <hal/arch-cpu.h>

/** イベントの数
 */
#define EV_NR_EVENT  (128)                     /*<  128イベント                           */
#define EV_MAP_LEN   ( EV_NR_EVENT / 64 )      /*<  ビットマップ配列長(uint64_t配列の数)  */
#define EV_RESERVED  (0)                       /*<  0番は予約                             */

/** イベントフラグ
 */
#define EV_FLAGS_NONE              (0)       /*< 通常のイベント                          */
#define EV_FLAGS_THREAD_SPECIFIC   (1)       /*< プロセス内の他のスレッドに引き継がない  */

/** システム定義イベント
 */
#define EV_SIG_HUP      (1)
#define EV_SIG_INT      (2)
#define EV_SIG_QUIT     (3)
#define EV_SIG_ILL      (4)
#define EV_SIG_ABRT     (6)
#define EV_SIG_FPE      (8)
#define EV_SIG_KILL     (9)
#define EV_SIG_BUS     (10)
#define EV_SIG_SEGV    (11)
#define EV_SIG_PIPE    (13)
#define EV_SIG_ALRM    (14)
#define EV_SIG_TERM    (15)
#define EV_SIG_CHLD    (20)
#define EV_SIG_USR1    (30)
#define EV_SIG_USR2    (31)
#define EV_SYS_NOTIFY  (63)
#define EV_SYS_NR      (64)

/** CPUによる例外送出イベント
 */
#define ev_is_cpu_exception(ev) \
	( (ev) < EV_SIG_USR1 )

#define EV_SIG_SI_USER    (0)
#define EV_SIG_SI_KERNEL  (1)
#define EV_SIG_SI_TKILL   (2)

#define EV_CODE_ILL_ILLOPC    (1)  /*<  不正な命令コード            */
#define EV_CODE_ILL_ILLOPN    (2)  /*<  不正なオペランド            */
#define EV_CODE_ILL_ILLADR    (3)  /*<  不正なアドレッシングモード  */
#define EV_CODE_ILL_ILLTRP    (4)  /*<  不正なトラップ              */
#define EV_CODE_ILL_PRVOPC    (5)  /*<  特権が必要な命令コード      */
#define EV_CODE_ILL_PRVREG    (6)  /*<  特権が必要なレジスター      */
#define EV_CODE_ILL_COPROC    (7)  /*<  コプロセッサのエラー        */
#define EV_CODE_ILL_BADSTK    (8)  /*<  内部スタックエラー          */

#define EV_CODE_FPE_INTDIV    (1)  /*< 整数の 0 による除算        */
#define EV_CODE_FPE_INTOVF    (2)  /*< 整数のオーバフロー         */
#define EV_CODE_FPE_FLTDIV    (3)  /*< 浮動小数点の0除算          */
#define EV_CODE_FPE_FLTOVF    (4)  /*< 浮動小数点のオーバフロー   */
#define EV_CODE_FPE_FLTUND    (5)  /*< 浮動小数点のアンダーフロー */
#define EV_CODE_FPE_FLTRES    (6)  /*< 浮動小数点の不正確な演算   */
#define EV_CODE_FPE_FLTINV    (7)  /*< 浮動小数点の不正な操作     */
#define EV_CODE_FPE_FLTSUB    (8)  /*< 範囲外の添字               */

#define EV_CODE_SEGV_MAPERR   (1)  /*< オブジェクトにマッピングされていないアドレス */
#define EV_CODE_SEGV_ACCERR   (2)  /*< オブジェクトに対するアクセス許可がない       */

#define EV_CODE_BUS_ADRALN    (1)  /*< 不正なアドレス・アライメント  */
#define EV_CODE_BUS_ADRERR    (2)  /*< 存在しない物理アドレス        */
#define EV_CODE_BUS_OBJERR    (3)  /*< 存在しない物理アドレス        */

/** イベント情報
 */
typedef struct _evinfo{
	event_flags        flags;  /*< イベント処理用のフラグ        */
	event_id              no;  /*< イベント番号                  */
	event_errno          err;  /*< イベントエラー番号            */
	event_code          code;  /*< イベントコード                */
	event_trap          trap;  /*< トラップコード                */
	tid            sender_id;  /*< 配送元スレッドID              */
	exit_code      ev_status;  /*< 終了ステータス                */
	ticks          *ev_utime;  /*< ユーザ時間                    */
	ticks          *ev_stime;  /*< システム時間                  */
	void            *ev_addr;  /*< 不正メモリアクセス先アドレス  */
	event_data          data;  /*< 付帯情報                      */
	event_data_size data_siz;  /*< 付帯情報の長さ(単位:バイト)   */
}evinfo;

/** イベントマスク
 */
typedef struct _event_mask{
	events_map map[EV_MAP_LEN];  /*< イベントのビットマップ配列  */
}event_mask;

/** 非同期イベントキュー
 */
typedef struct _event_queue{
	spinlock                 lock;  /*< イベントキューのロック                */
	event_mask             events;  /*< ペンディング中イベントのビットマップ  */
	event_mask              masks;  /*< イベントマスクのビットマップ          */
	queue        que[EV_NR_EVENT];  /*< イベントキュー                        */
}event_queue;

/** イベントキューのノード
 */
typedef struct _event_node{
	list   link;  /*< イベントキューへのリンク  */
	evinfo info;  /*< イベント情報              */	
}event_node;

/** イベントフレーム
 */
typedef struct _event_frame{
	evinfo            info;
	trap_context  trap_ctx;    /*< トラップコンテキストのコピー  */
	fpu_context  fpu_frame;    /*< FPUフレーム                   */
}event_frame;

struct _proc;
struct _thread;
void ev_queue_init(event_queue *_que);
void ev_free_pending_events(event_queue *_que);
void ev_get_mask(event_mask *_mask);
void ev_update_mask(struct _thread *_thr, event_mask *_mask);

void ev_mask_clr(event_mask *_maskp);
bool ev_mask_test(event_mask *_mask, event_id _id);
bool ev_mask_empty(event_mask *mask);
void ev_mask_set(event_mask *_maskp, event_id _id);
void ev_mask_unset(event_mask *_maskp, event_id _id);
void ev_mask_xor(event_mask *_mask1, event_mask *_mask2, event_mask *_maskp);
void ev_mask_and(event_mask *_mask1, event_mask *_mask2, event_mask *_maskp);
void ev_mask_fill(event_mask *_mask);
int  ev_mask_find_first_bit(event_mask *_mask, event_id *_valp);

int  ev_send(tid _dest, event_node *_node);
void ev_send_to_process(struct _proc *_p, event_node *_node);
int  ev_send_to_all_threads_in_process(struct _proc *_p, event_node *_node);
int  ev_dequeue(event_node **_nodep);
bool ev_has_pending_events(struct _thread *_thr);
int  ev_alloc_node(event_id id, event_node **nodep);
void ev_handle_exit_thread_events(void);

int  kcom_handle_system_event(event_node *_node);
#endif  /*  _KERN_ASYNC_EVENT_H   */
