/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  strncmp routine                                                   */
/*                                                                    */
/**********************************************************************/
#include <kern/string.h>

/** 文字列 s1 とs2の先頭n文字を比較する 
    @param[in] s1 比較対象のメモリ領域1
    @param[in] s2 比較対象のメモリ領域2
    @retval 負の整数 s1がs2よりも小さい(s1 < s2)
    @retval 0        s1とs2が等しい
    @retval 正の整数 s1がs2よりも大きいさい(s1 > s2)
 */
int 
strncmp(const char *s1, const char *s2, size_t n){
	signed char rc;
	
	rc = (*s2 == '\0') ? (0) : (-1);
	for(; *s1 != '\0'; ++s1, ++s2, --n) {

		rc = *s1 - *s2;
		if ( (rc) || (n == 0) )
			break;
		
	}

	return (int)rc;
}
