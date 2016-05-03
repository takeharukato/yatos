/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  strstr routine                                                    */
/*                                                                    */
/**********************************************************************/
#include <kern/string.h>

/** 文字列haystack中の文字列needleが表れる最初の位置を返却する
    @param[in] haystack 調査対象文字列
    @param[in] needle   探査する文字列
    @return 文字列s中の最初に文字列が現れた位置
    @return 見つからなかった場合はNULLを返す。
 */
char *
strstr(const char *haystack, const char *needle){
	char *cq;
	char *cr;

	for ( ; *haystack != '\0'; ++haystack){
		
		if (*haystack == *needle) {

			cq = (char *)haystack;
			cr = (char *)needle;
			while ( (*cq != '\0') && (*cr != '\0') ) {

				if (*cq != *cr)
					break;

				++cq;
				++cr;
			}

			if ( *cr == '\0' )
				return (char *)haystack;
		}
	}

	return NULL;
}
