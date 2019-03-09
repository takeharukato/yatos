/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  some parameter relevant definitions                               */
/*                                                                    */
/**********************************************************************/
#if !defined(_KERN_PARAM_H)
#define  _KERN_PARAM_H 

#include <limits.h>

#define NR_CPUS                 (1)
#define NR_IRQS                 (16)      /*< PC/AT機の割込み数を元に設定                  */
#define THR_MIN_PRIO            (0)
#define THR_MAX_PRIO            (32)
#define THR_MAX_USER_PRIO       (16)
#define THR_MAX_SLICE           (40)
#define THR_DEFAULT_SLICE       (21)
#define THR_NIN_SLICE           (1)
#define PAGE_POOL_MAX_ORDER     (11)      /*< 最大4MiBページ                               */
#define KSTACK_ORDER            (1)       /*< 2ページ                                      */
#define KSTACK_SIZE             (0x2000)  /*< PAGE_SIZE * (1 << KSTACK_ORDER)バイト        */
#define MAX_OBJ_ID              (ULLONG_MAX)   /*< ID範囲は0-( MAX_OBJ_ID - 1)             */
#define SPURIOUS_INTR_THRESHOLD (64)      /*< 許容可能な誤検出割込み回数の限界値           */
#define HZ                      (100)     /*< 10ms ティック                                */
#define PROC_NAME_LEN           (128)     /*< プロセス名の長さ（NULL終端含む)              */
#define KM_DEFAULT_LIMIT        (10)      /*< slabキャッシュの規定キャッシュオブジェクト数 */
#define DEFAULT_L1_CACHE_BYTE   (64)      /*< L1キャッシュサイズの規定バイト数             */
#define BACKTRACE_MAX_DEPTH     (20)      /*< バックトレースの最大深度                     */
#define SPINLOCK_BT_DEPTH       (BACKTRACE_MAX_DEPTH)  /*< スピンロック中のバックトレース保存量 */
#endif  /*  _KERN_PARAM_H   */
