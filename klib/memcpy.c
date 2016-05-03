/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  memcpy routine                                                    */
/*                                                                    */
/**********************************************************************/
#include <kern/string.h>

/** 領域の重なりを考慮せず, srcからdestにcountバイト分のメモリをコピーする。
    @param[in] dest  コピー先のアドレス
    @param[in] src   コピー元のアドレス
    @param[in] count コピーするバイト数
    @return destのアドレス
    @note 領域が重なる場合は, memmoveを使用する。
 */
void *
memcpy(void *dest, const void *src, size_t count){
	char *d;
	char *s;
	size_t len;
	
	d = (char *)dest;
	s = (char *)src;
	for(len = count;len > 0;--len, ++s, ++d) 
		*d = *s;
	
	return dest;
}
