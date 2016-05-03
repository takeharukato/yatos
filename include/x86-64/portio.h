/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  port I/O handling routines                                        */
/*                                                                    */
/**********************************************************************/
#if !defined(_HAL_PORTIO_H)
#define  _HAL_PORTIO_H 

/*
 *  Port I/O without wait
 */
static inline void 
out_port_byte(unsigned short port, unsigned char data) {

	__asm__ __volatile__ ("outb %b1, %w0\n\t"
	    :/* No output */
	    : "Nd" (port), "a" (data));

	return;
}

static inline void
out_port_word (unsigned short port, unsigned short data) {

	__asm__ __volatile__ ("outw %w1, %w0\n\t"
	    :/* No output */
	    : "Nb" (port), "a" (data));

	return;
}

static inline void 
out_port_dword (unsigned short port, unsigned long data) {

	__asm__  __volatile__ ("outl %1, %w0\n\t"
	    : /* No output */
	    : "Nb" (port), "a" (data));

	return;
}

static inline unsigned char
in_port_byte(unsigned short port) {
	unsigned char ret;

	__asm__ __volatile__ ("inb %w1, %b0\n\t"
	    : "=a" (ret)
	    : "Nd" (port));

	return ret;
}

static inline unsigned short
in_port_word(unsigned short port) {
	unsigned short ret;

	__asm__ __volatile__ ("inw %w1, %b0\n\t"
	    : "=a" (ret)
	    : "Nd" (port));

	return ret;
}

static inline unsigned long
in_port_dword (unsigned short port) {
	unsigned long ret;

	__asm__ __volatile__ ("inl %w1, %w0\n\t"
	    : "=a" (ret)
	    : "Nd" (port));

	return ret;
}

#endif  /*  _HAL_PORTIO_H   */
