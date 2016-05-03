/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  strrchr routine                                                   */
/*                                                                    */
/**********************************************************************/
#include <kern/string.h>

/** 文字列s中の末尾c文字目以内の最初に文字cが現れた位置へのポインタを返す。
    @param[in] s 調査対象文字列
    @param[in] c 探査する文字
    @return 文字列s中の最初に文字cが現れた位置
 */
char *
strrchr(const char *s, int c){
	char *last;
	char *sp;

	for(sp = (char *)s, last = c?((char *)NULL):((char *)s); *sp != '\0'; ++sp) {

		if (*sp == c) {

			last = sp;
		}
	}

	return last;
}
