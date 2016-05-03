/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  UART definitions                                                  */
/*                                                                    */
/**********************************************************************/
#if !defined(_HAL_UART_H)
#define  _HAL_UART_H 

#define UART_COM1_BASE   (0x3f8)
#define UART_COM2_BASE   (0x2f8)
#define UART_COM3_BASE   (0x3e8)
#define UART_COM4_BASE   (0x2e8)

#define UART_COM1_IRQ        (4)
#define UART_COM2_IRQ        (3)
#define UART_COM3_IRQ        (4)
#define UART_COM4_IRQ        (3)

#if defined(CONFIG_DBG_SERIAL_COM1)
#define UART_COM__NAME "COM1"
#define DBG_COMBASE    UART_COM1_BASE
#define UART_COM_IRQ   UART_COM1_IRQ
#elif defined(CONFIG_DBG_SERIAL_COM2)
#define UART_COM__NAME "COM2"
#define DBG_COMBASE    UART_COM2_BASE
#define UART_COM_IRQ   UART_COM2_IRQ
#endif  /*  CONFIG_DBG_SERIAL_COM1  */

#define UART_DAT     (0x00)
#define UART_LLAT    (0x00)   /* 上位バイトディバイザ */
#define UART_HLAT    (0x01)   /* 下位バイトバイトディバイザ */

#define UART_INTR    (0x01)   /* 割り込み許可 */
#define UART_FIFO    (0x02)   /* FIFO  */
#define UART_IIR     (0x02)   /* 割り込みID  */
#define UART_LCTRL   (0x03)   /* ラインコントロール  */
#define UART_MCTRL   (0x04)   /* モデムコントロール  */
#define UART_LSR     (0x05)   /* ラインステータス    */
#define UART_MSR     (0x06)   /* モデムステータス    */
#define UART_SERIAL_FREQ                      (1846154)
#define UART_INTERNAL_BIAS                         (16)

/*
 *  割り込み関連
 */
#define UART_IIR_NO_INT	(0x01)	/* 割り込み未発生 */
#define UART_IIR_IDMASK	(0x06)	/* 割り込みIDマスク */
#define UART_IIR_MSI	(0x00)	/* モデムステータス */
#define UART_IIR_THRI	(0x02)	/* 送信完了(バッファエンプティ) */
#define UART_IIR_RDI	(0x04)	/* 受信完了 */
#define UART_IIR_RLSI	(0x06)	/* 受信回線状態更新 */
#define uart_getiir(val) ( (val) & (UART_IIR_IDMASK) )
/*  
 *  モデムコントロール関連
 */
#define UART_MCTL_DTR_LOW     (0x1)
#define UART_MCTL_RTS_LOW     (0x2)
#define UART_MCTL_OUT1_LOW    (0x4)
#define UART_MCTL_ENINTR      (0x8)
#define UART_MCTL_LOOPBAK     (0x10)
#define UART_MCTL_CONST       (UART_MCTL_DTR_LOW|UART_MCTL_RTS_LOW)
#define uart_mk_mctl(func)    ( (func) | UART_MCTL_CONST )

/*
 * ラインステータス
 */
#define UART_LSR_TXHOLD      (0x20)
/*
 * パラメタ
 */
#define UART_DEFAULT_BAUD     (115200)
#define UART_SERIAL_BUFSIZ      (4096)

#endif  /*  _HAL_UART_H   */
