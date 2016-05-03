/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  memmove routine                                                   */
/*                                                                    */
/**********************************************************************/
#include <kern/string.h>

/** 領域の重なりを考慮して, srcからdestにcountバイト分のメモリをコピーする。
    @param[in] dest  コピー先のアドレス
    @param[in] src   コピー元のアドレス
    @param[in] count コピーするバイト数
    @return destのアドレス
 */
void *
memmove(void *dest, const void *src, size_t count) {
	char *d;
	char *s;
	size_t len;

	d = (char *)dest;
        s = (char *)src;
	len = count;

	if (d == s)
		return dest;
	
	if ( ( s < d ) && ( d < ( s + count ) ) ) {

		s += count;
		d += count;
		while( len-- > 0 )
			*--d = *--s;
	} else {
		
		while( len-- > 0 )
			*d++ = *s++;
	}

	return dest;
}
