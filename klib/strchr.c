/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  strchr routine                                                    */
/*                                                                    */
/**********************************************************************/
#include <kern/string.h>

/** 文字列s中に最初に文字cが現れた位置へのポインタを返す。
    @param[in] s 調査対象文字列
    @param[in] c 探査する文字
    @return 文字列s中の最初に文字cが現れた位置
 */
char *
strchr(const char *s, int c){

	for(; *s != (char)c ; ++s)
		if ( *s == '\0' ) 
			return NULL;

	return (char *)s;
}
