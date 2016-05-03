/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  Reserved ID definitions                                           */
/*                                                                    */
/**********************************************************************/
#if !defined(_KERN_KRESV_IDS_H)
#define  _KERN_KRESV_IDS_H 

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <kern/param.h>

#define ID_RESV_IDLE        (0)  /*< アイドルスレッド用に予約            */
#define ID_RESV_REAPER      (1)  /*< reaperスレッド用に予約              */
#define ID_RESV_NAME_SERV   (2)  /*< ネームサービススレッド用に予約      */
#define ID_RESV_DBG_CONSOLE (3)  /*< デバッグコンソールサービス用に予約  */
#define ID_RESV_PROC        (4)  /*< PROCサービス用に予約                */
#define ID_RESV_VM          (5)  /*< VMサービス用に予約                  */
#define ID_NR_RESVED        (6)  /*< 予約ID数                            */
#define ID_RESV_INVALID     (MAX_OBJ_ID)  /*< 不正ID  */

#define ID_RESV_NAME_DBG_CONSOLE "DebugConsole"
#define ID_RESV_NAME_PROC        "ProcessService"
#define ID_RESV_NAME_VM          "VMService"

#define KSERV_NAME_SERV_PRIO (THR_MAX_PRIO - 4)
#define KSERV_DBG_CONS_PRIO  (THR_MAX_PRIO - 5)
#define KSERV_KSERVICE_PRIO  (THR_MAX_PRIO - 5)
#endif  /*  _KERN_KRESV_IDS_H   */
