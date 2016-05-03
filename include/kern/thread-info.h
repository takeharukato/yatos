/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  Thread info relevant definitions                                  */
/*                                                                    */
/**********************************************************************/
#if !defined(_KERN_THREAD_INFO_H)
#define  _KERN_THREAD_INFO_H 

#include <kern/config.h>
#include <kern/kernel.h>
#include <kern/param.h>

#define THR_THREAD_INFO_MAGIC (0xdeadbeef)  /*< スタックの底を示すマジック番号  */

#if !defined(ASM_FILE)

#include <stdint.h>
#include <stddef.h>

#include <kern/kern_types.h>
#include <kern/assert.h>

#include <hal/stack-ops.h>

struct _thread;
/** ディスパッチ許可状態
 */
#define THR_PREEMPT_ACTIVE    (0x80000000)            /*< ディスパッチ要求受付中  */

#define THR_DISPATCH_DELAYED  (0x1)                   /*< 遅延ディスパッチ        */
#define THR_EVENT_PENDING     (0x2)                   /*< 非同期イベント有り      */
/** スレッド管理情報(スタック部分)
 */
typedef struct _thread_info{
	thread_info_magic      magic;   /*< スタックの底を表すマジック番号 */
	intr_depth           intrcnt;   /*< 割込み多重度                   */
	preempt_count        preempt;   /*< ディスパッチ禁止状態管理情報   */
	thread_info_flags      flags;   /*< 遅延ディスパッチなどのフラグ   */
	thread_info_flags arch_flags;   /*< アーキ固有のフラグ             */
	struct _thread          *thr;   /*< スレッド情報へのポインタ       */
	cpu_id                   cpu;   /*< 自CPU番号                      */
}thread_info;

void ti_bind_thread_info(thread_info *_tinfo, struct _thread *_thr);
void *ti_get_current_kstack_top(void);
thread_info *ti_kstack_to_tinfo(void *_kstack_top);
thread_info *ti_get_current_tinfo(void);
struct _thread *ti_get_current_thread(void);

void ti_disable_dispatch(void);
void ti_enable_dispatch(void);
void ti_clr_thread_info(thread_info *tinfo);

void ti_set_delay_dispatch(thread_info *tinfo);
void ti_clr_delay_dispatch(thread_info *tinfo);

void ti_set_event(thread_info *tinfo);
void ti_clr_event(thread_info *tinfo);

void ti_disable_dispatch(void);
void ti_enable_dispatch(void);
void ti_clr_thread_info(thread_info *tinfo);

int ti_dispatch_disabled(thread_info *tinfo);
int ti_dispatch_delayed(thread_info *tinfo);
int ti_event_pending(thread_info *tinfo);

void ti_inc_intr(void);
void ti_dec_intr(void);
bool ti_in_intr(void);

void hal_ti_init(thread_info *tinfo);
#if !defined(__IN_ASM_OFFSET)

#include <hal/asm-offset.h>
#include <hal/stack-ops.h>

/** カレントスレッドのスレッド構造体を取得する
 */
#define current ((struct _thread *)ti_get_current_thread())

/** カレントスレッドの論理CPU番号を得る
 */
#define current_cpu() ( (cpu_id)( ( (thread_info *)ti_get_current_tinfo())->cpu ) )
#endif  /*  !__IN_ASM_OFFSET  */

#endif  /*  !ASM_FILE  */

#endif  /*  _KERN_THREAD_INFO_H   */
