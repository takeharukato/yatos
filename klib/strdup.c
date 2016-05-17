/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  strdup routines                                                   */
/*                                                                    */
/**********************************************************************/

#include <kern/string.h>
#include <kern/page.h>

/** 文字列 srcを複製する
    @param[in] src  コピー元のアドレス
    @return    複製した文字列へのポインタ
    @return    NULLL文字列の複製に失敗
 */
char *
strdup(const char *src){
	char     *r;
	size_t  len;

	len = strlen(src);

	r = kmalloc( len + 1, KMALLOC_NORMAL );
	if ( r == NULL )
		return NULL;

	strcpy(r, src);

	return r;
}
