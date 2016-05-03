/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  memset routine                                                    */
/*                                                                    */
/**********************************************************************/
#include <kern/string.h>

/** sで示されるメモリ領域の先頭からnバイトをcで埋める。
    @param[in] s 書き込み先メモリの先頭アドレス
    @param[in] c 書き込む値(charとして扱われる)
    @param[in] n 書き込むバイト数
 */
void *
memset(void *s, int c, size_t n){
	size_t len;
	char *d;

	for(d = (char *)s, len = n;len > 0;--len)
		*d++ = (char)c;

	return s;
}

