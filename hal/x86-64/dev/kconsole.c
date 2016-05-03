/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  low level kernel console routines                                 */
/*                                                                    */
/**********************************************************************/

#include <stdint.h>
#include <stddef.h>

#include <kern/config.h>
#include <hal/portio.h>
#include <hal/uart.h>

void
kconsole_putc(int ch) {

	while (!(in_port_byte(DBG_COMBASE + UART_LSR) & UART_LSR_TXHOLD));
	out_port_byte(DBG_COMBASE, (char) ch);
}

void
init_kconsole(void) {
	uint16_t baud_setting;

	baud_setting = UART_SERIAL_FREQ / UART_INTERNAL_BIAS / UART_DEFAULT_BAUD;
	out_port_byte(DBG_COMBASE + UART_INTR, 0);  /*  disable interrupt  */
	out_port_byte(DBG_COMBASE + UART_MCTRL, uart_mk_mctl(0)); 

	out_port_byte(DBG_COMBASE + UART_LLAT, baud_setting & 0xff ); /* low */
	out_port_byte(DBG_COMBASE + UART_HLAT, baud_setting >> 8);    /* high */
	out_port_byte(DBG_COMBASE + UART_LCTRL, 0x03); /*  8 bit stop=0 Non-parity */

	out_port_byte(DBG_COMBASE + UART_FIFO, 0x0);      /* disable FIFO */
}
