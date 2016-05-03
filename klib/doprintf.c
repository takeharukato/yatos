/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  format print core routines                                        */
/*                                                                    */
/**********************************************************************/
#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#include <limits.h>

#include <kern/kprintf.h>
#include <kern/string.h>

#define ZEROPAD	1		/* pad with zero */
#define SIGN	2		/* unsigned/signed long */
#define PLUS	4		/* show plus */
#define SPACE	8		/* space if plus */
#define LEFT	16		/* left justified */
#define SPECIAL	32		/* 0x */
#define LARGE	64		/* use 'ABCDEF' instead of 'abcdef' */

#define isdigit(c) ( ('0' <= c) && (c <= '9') )

static const char field_qualifiers[]={
	'+',
	'-',
	' ',
	'#',
	'0'
};


int
is_field_qualifiers(char c, int *flags){
	int i;

	for(i = 0; i < (int)sizeof(field_qualifiers)/(int)sizeof(char); ++i) {

		if (c == field_qualifiers[i]) {

			switch (c) {
			case '-':

				*flags |= LEFT; 
				break;
			case '+':

				*flags |= PLUS;
				break;
			case ' ':

				*flags |= SPACE; 
				break;
			case '#':

				*flags |= SPECIAL;
				break;
			case '0':

				*flags |= ZEROPAD; 
				break;
			}

			return 1;
		}
	}

	return 0;
}

static int 
simple_atoi(const char **s){
	int i=0;

	while (isdigit(**s))
		i = i*10 + *((*s)++) - '0';

	return i;
}

static unsigned long long 
do_div(unsigned long long *n, unsigned base){ 
	unsigned long long res;

	res = (*n) %  (unsigned long long)base;
	*n = (*n) / (unsigned long long)base;

	return res;
}

int 
number(long long num, unsigned base, int size, int precision, int type,
       int (*__putc)(char _c, void *_argp), void *argp){
	char c,sign,tmp[66];
	const char *digits="0123456789abcdefghijklmnopqrstuvwxyz";
	int i;
	int len;

	len = 0;

	if (type & LARGE)
		digits = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

	if (type & LEFT)
		type &= ~ZEROPAD;

	if (base < 2 || base > 36)
		return len;

	c = (type & ZEROPAD) ? '0' : ' ';
	sign = 0;

	if (type & SIGN) {

		if (num < 0) {

			sign = '-';
			num = -num;
			size--;
		} else if (type & PLUS) {

			sign = '+';
			size--;
		} else if (type & SPACE) {

			sign = ' ';
			size--;
		}
	}

	if (type & SPECIAL) {

		if (base == 16)
			size -= 2;
		else if (base == 8)
			size--;
	}

	i = 0;
	if (num == 0)
		tmp[i++]='0';
	else while (num != 0)
		     tmp[i++] = digits[do_div((unsigned long long *)&num, base)];

	if (i > precision)
		precision = i;

	size -= precision;
	if (!(type&(ZEROPAD+LEFT)))
		while(size-->0)
			len += __putc(' ', argp);

	if (sign)
		len += __putc(sign, argp);

	if (type & SPECIAL) {

		if (base==8)
			len += __putc('0', argp);
		else if (base==16) {

			len += __putc('0', argp);
			len += __putc(digits[33], argp);
		}
	}

	if (!(type & LEFT))
		while (size-- > 0)
			len += __putc(c, argp);

	while (i < precision--)
		len += __putc('0', argp);

	while (i-- > 0)
		len += __putc(tmp[i], argp);
	
	while (size-- > 0)
		len += __putc(' ', argp);

	return len;
}

int 
doprintf(int (*__putc)(char _c, void *_argp), void *argp, const char *fmt, va_list args){
	int flags;
	int slen;
	int field_width;
	int precision;
	int base;
	int qualifier;
	int i;
	long long num;
	const char *s;
	int outlen;

	outlen = 0;
	for ( ; *fmt ; ++fmt) {

		/*
		 *書式指定文字を除いてバッファに出力する
		 */
		if (*fmt != '%') {

			outlen += __putc(*fmt, argp);
			continue;
		}

                /* 
		 * フィールド修飾子の解析
		 */
		++fmt;		/* 最初の '%' を飛ばす */
		for(flags = 0;is_field_qualifiers(*fmt, &flags)>0;++fmt); 
		
		/*
		 * フィールド長の解析
		 */
		field_width = -1;
		if (isdigit(*fmt))
			field_width = simple_atoi(&fmt);
		else if (*fmt == '*') {

			++fmt; /* 最初の'*'を飛ばす  */
			field_width = va_arg(args, int);  /* 引数からフィールド長を得る  */

			if (field_width < 0) {

				field_width = -field_width;
				flags |= LEFT;
			}
		}

		/*
		 *フィールド精度の解析
		 */
		precision = -1;
		if (*fmt == '.') {

			++fmt;	
			if (isdigit(*fmt))
				precision = simple_atoi(&fmt);
			else if (*fmt == '*') {

				++fmt;
				/* it's the next argument */
				precision = va_arg(args, int);
			}

			if (precision < 0)
				precision = 0;
		}
		
		/* 
		 * 変換修飾子
		 */
		qualifier = -1;
		if (*fmt == 'h' || *fmt == 'l' || *fmt == 'L') {

			qualifier = *fmt;
			++fmt;
		}

		base = 10;
		
		/*
		 * 書式出力
		 */
		switch (*fmt) {
		case 'c':

			if (!(flags & LEFT))
				while(--field_width > 0)
					outlen += __putc(' ', argp);

			outlen += __putc((unsigned char)va_arg(args, int), argp);

			while(--field_width > 0)
				outlen += __putc(' ', argp);

			continue;
		case 's':

			s = va_arg(args, char *);
			if (!s)
				s = "<NULL>";

			slen = strnlen(s, precision);

			if (!(flags & LEFT))
				while(slen < field_width--) 
					outlen += __putc(' ', argp);

			for (i = 0; i < slen; ++i)
				outlen += __putc(*s++, argp);

			while(slen < field_width--)
				outlen += __putc(' ', argp);

			continue;
		case 'p':

			if (field_width == -1) {

				field_width = 2*sizeof(void *);
				flags |= ZEROPAD;
			}

			outlen += __putc('0', argp);
			outlen += __putc('x', argp);
			outlen += number((unsigned long) va_arg(args, void *), 16,
			       field_width, precision, flags, __putc, argp);
			continue;
		case 'o':

			base = 8;
			break;

		case 'X':

			flags |= LARGE;
		case 'x':

			base = 16;
			break;

		case 'd':
		case 'i':

			flags |= SIGN;
		case 'u':

			break;
			
		default:

			if (*fmt != '%')
				outlen += __putc('%', argp);

			if (*fmt)
				outlen += __putc(*fmt, argp);
			else 
				--fmt;

			continue;
		}

		/*
		 * 数値出力
		 */
		if (qualifier == 'L')
			num = va_arg(args, unsigned long long);
		else if (qualifier == 'l') {

			num = va_arg(args, unsigned long);
			if (flags & SIGN)
				num = (long) num;

		} else if (qualifier == 'h') {

			num = (unsigned short) va_arg(args, int);
			if (flags & SIGN)
				num = (short) num;

		} else if (flags & SIGN)
			num = va_arg(args, int);
		else
			num = va_arg(args, unsigned int);

		outlen += number(num, base, field_width, precision, flags, __putc, argp);
	}

	outlen += __putc('\0', argp);

	return outlen;
}

