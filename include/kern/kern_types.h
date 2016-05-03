/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  relevant definitions                                              */
/*                                                                    */
/**********************************************************************/
#if !defined(_KERN_KERN_TYPES_H)
#define  _KERN_KERN_TYPES_H 

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <kern/config.h>
#include <kern/kernel.h>
#include <kern/param.h>

typedef uint64_t            obj_id;  /*< 汎用ID                                       */
typedef uint32_t thread_info_magic;  /*< スタックマジック                             */
typedef uint32_t thread_info_flags;  /*< 例外出口関連フラグ                           */
typedef uint32_t     preempt_count;  /*< スレッドディスパッチ許可状態                 */
typedef uint32_t        intr_depth;  /*< 割込み多重度                                 */
typedef uint32_t       mutex_flags;  /*< mutexの属性                                  */
typedef uint32_t            cpu_id;  /*< 論理CPU ID                                   */
typedef uint32_t          thr_prio;  /*< スレッドの優先度                             */
typedef obj_id                 tid;  /*< スレッドID                                   */
typedef uint32_t thread_wait_flags;  /*< スレッド待ち合わせフラグ                     */
typedef uint64_t         exit_code;  /*< スレッド終了コード                           */
typedef uint64_t      thread_flags;  /*< スレッド属性フラグ                           */
typedef void *         kstack_type;  /*< カーネルスタック                             */
typedef obj_id        obj_cnt_type;  /*< 加算オブジェクト数                           */
typedef obj_id                 pid;  /*< プロセスID                                   */
typedef uint64_t  bitmap_base_type;  /*< ビットマップの基礎サイズ                     */
typedef uint32_t           intr_no;  /*< 割込み番号                                   */
typedef uint32_t    ihandler_flags;  /*< 割込みハンドラフラグ                         */
typedef void *         private_inf;  /*< プライベート情報                             */
typedef uint64_t   intr_mask_state;  /*< 割込みマスク状態                             */
typedef intr_no          intr_mask;  /*< 割込みマスク                                 */
typedef uint32_t          vma_prot;  /*< メモリ領域保護属性                           */
typedef uint32_t         vma_flags;  /*< メモリ領域属性                               */
typedef uint32_t        page_state;  /*< ページ状態                                   */
typedef uint32_t     pgalloc_flags;  /*< ページ割り当てフラグ                         */
typedef uint32_t        slab_flags;  /*< スラブ獲得フラグ                             */
typedef uint32_t        slab_state;  /*< スラブ状態                                   */
typedef uint64_t         intrflags;  /*< 割り込みフラグ                               */
typedef int             page_order;  /*< ページオーダ                                 */
typedef uint64_t   page_order_mask;  /*< ページオーダマスク                           */
typedef uint32_t         tim_tmout;  /*< タイマハンドラのタイムアウト時間             */
typedef int32_t          lpc_tmout;  /*< LPCのタイムアウト値                          */
typedef tid               endpoint;  /*< LPCの端点(pid/tid)                           */
typedef uint64_t             ticks;  /*< 電源投入時からのティック発生回数             */
typedef uint64_t         delay_cnt;  /*< ミリ秒以下のループ待ち指定値                 */
typedef uint64_t        events_map;  /*< 非同期イベントのビットマップ                 */
typedef obj_cnt_type      event_id;  /*< 非同期イベントID                             */
typedef int            event_errno;  /*< 非同期イベントエラー番号                     */
typedef int             event_code;  /*< 非同期イベントコード番号                     */
typedef int             event_trap;  /*< 非同期イベントトラップ番号                   */
typedef void *          event_data;  /*< 非同期イベント付帯情報                       */
#endif  /*  _KERN_KERN_TYPES_H   */
