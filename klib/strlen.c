/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  strlen routine                                                    */
/*                                                                    */
/**********************************************************************/
#include <kern/string.h>

/** 文字列sの長さを得る
    @param[in] s    調査対象文字列
    @return    文字列長
 */
size_t
strlen(char const *s){
	const char *sp;

	for(sp = s; *sp != '\0'; ++sp);

	return sp - s;
}
