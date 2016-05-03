/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  memchr routine                                                    */
/*                                                                    */
/**********************************************************************/
#include <kern/string.h>

/** sからcountバイト以内に値cが含まれる位置のアドレスを返却する
    @param[in] s 探索開始アドレス
    @param[in] c 探索対象値
    @param[in] count 探索バイト数
    @return 見つかった位置のアドレス
 */
void *
memchr(const void *s, int c, size_t count){

	for(; count > 0 ; ++s, --count)
		if ( *(char *)s == (char)c ) 
			return (void *)s;
	return NULL;
}
