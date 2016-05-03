/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  memcmp routine                                                    */
/*                                                                    */
/**********************************************************************/
#include <kern/string.h>

/** メモリ領域 m1 とm2 の最初のcountバイトを比較する 
    @param[in] m1 比較対象のメモリ領域1
    @param[in] m2 比較対象のメモリ領域2
    @param[in] count 比較する領域長
    (各バイトは unsigned char として解釈される)
    @retval 負の整数 m1の最初のcountバイトがm2の最初のcountバイトよりも小さい(m1 < m2)
    @retval 0   m1の最初のcountバイトがm2の最初のcountバイトとおなじ(m1 == m2)
    @retval 正の整数 m1の最初のcountバイトがm2の最初のcountバイトよりも大きい(m1 > m2)    
 */
int
memcmp(const void *m1, const void *m2, size_t count){
	const unsigned char *s1, *s2;
	signed char rc = 0;

	for(s1 = m1, s2 = m2; count > 0; ++s1, ++s2, --count) {
		rc = *s1 - *s2;
		if (rc != 0)
			break;
	}

	return rc;
}
