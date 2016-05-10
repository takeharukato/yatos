/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  printf routines                                                   */
/*                                                                    */
/**********************************************************************/

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdarg.h>
#include <limits.h>

#include <ulib/libyatos.h>

/** 文字列バッファ出力制御情報
 */
struct _string_stream{
	size_t    pos;  /*< 書き込み位置  */
	size_t    max;  /*< バッファ長    */
	char    *buff;  /*< バッファ      */
};

/** 文字列バッファ出力用ストリームの初期化
    @param[in] sbuf 文字列バッファ出力制御情報へのポインタ
    @param[in] buf  文字列バッファ
    @param[in] max  文字列バッファ長
 */
static void
init_string_stream(struct _string_stream *sbuf, char *buf, size_t max){

	if (sbuf == NULL)
		return;

	sbuf->pos = 0;
	sbuf->buff = buf;
	sbuf->max = max;
}

/** 文字列バッファへの1文字出力
    @param[out] c   出力する文字
    @param[in]  argp 出力制御データ
    @retval     0    出力失敗
    @retval     1    出力成功
 */
static int 
string_stream_putc(char c, void *argp){
	struct _string_stream *sbuf;

	sbuf = (struct _string_stream *)argp;

	if (sbuf->pos == sbuf->max)
		return 0;

	sbuf->buff[sbuf->pos] = c;
	++sbuf->pos;

	return 1;
}

/** 簡易版snprintf関数
    @param[out] buf 出力先バッファ
    @param[in] size バッファ長
    @param[in] fmt  書式指定文字列
    @param[in] ...  可変長引数
    @return         出力した文字列長
 */
int
yatos_snprintf(char *buf, size_t size, const char *fmt,...){
	va_list args;
	int rc;
	struct _string_stream sbuf;

	init_string_stream(&sbuf, buf, size);

	va_start(args, fmt);
	rc = doprintf(string_stream_putc, &sbuf, fmt, args);
	va_end(args);
	
	return rc;
}

/** 簡易版printf関数
    @param[in] fmt 書式指定文字列
    @param[in] ... 可変長引数
    @return        出力した文字列長
 */
int
yatos_printf(const char *fmt,...) {
	va_list                  args;
	int                        rc;
	char buf[YATOS_PRINTF_BUFSIZ];
	struct _string_stream    sbuf;
	msg_body                  msg;
	pri_string_msg          *pmsg;

	init_string_stream(&sbuf, buf, YATOS_PRINTF_BUFSIZ);

	va_start(args, fmt);
	rc = doprintf(string_stream_putc, &sbuf, fmt, args);
	va_end(args);

	pmsg= &msg.sys_pri_dbg_msg;
	memset( &msg, 0, sizeof(msg_body) );

	pmsg->req = 0;
	pmsg->msg = buf;
	pmsg->len = strlen(pmsg->msg) + 1;

	yatos_lpc_send_and_reply(ID_RESV_DBG_CONSOLE, &msg);

	return rc;
}

/** デバッグコンソールへの書き込み
    @param[in] buf 書き込むデータ
    @param[in] len 書き込む長さ
    @return    書き込んだ長さ
 */
int
yatos_dbg_write(const char *buf, size_t len){
	int                        rc;
	msg_body                  msg;
	pri_string_msg          *pmsg;

	pmsg= &msg.sys_pri_dbg_msg;
	memset( &msg, 0, sizeof(msg_body) );

	pmsg->req = 0;
	pmsg->msg = (void *)buf;
	pmsg->len = len;

	rc = yatos_lpc_send_and_reply(ID_RESV_DBG_CONSOLE, &msg);

	return rc;
	
}
