/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  strncpy routine                                                   */
/*                                                                    */
/**********************************************************************/
#include <kern/string.h>

char *
strncpy(char *dest, char const *src, size_t count){
	char *tmp;

	if (count == 0)
		return dest;

	for(tmp = dest; ; ++src, ++dest, --count) {

		*dest = *src;
		if ( ( count == 0 ) || ( *src == '\0' ) )
			break;
	}

	return tmp;
}
