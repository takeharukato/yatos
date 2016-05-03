/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  strcpy routines                                                   */
/*                                                                    */
/**********************************************************************/
#include <kern/string.h>

/** 文字列 srcをdestにコピーする
    @param[in] dest コピー先のアドレス
    @param[in] src  コピー元のアドレス
    @retval 負の整数 s1がs2よりも小さい(s1 < s2)
    @retval 0        s1とs2が等しい
    @retval 正の整数 s1がs2よりも大きいさい(s1 > s2)
 */
char *
strcpy(char *dest, char const *src){
	char *tmp;

	for(tmp = dest; ; ++src, ++dest) {

		*dest = *src;
		if ( *src == '\0' )
			break;
	}
	return tmp;
}
