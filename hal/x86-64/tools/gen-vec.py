#!/usr/bin/env python
# -*- coding: utf-8 -*-

exception_names = ("Divide Error",
             "Debug Exception Fault/ Trap",
             "NMI Interrupt",
             "Breakpoint",
             "Overflow",
             "BOUND Range Exceeded",
             "Invalid Opcode (Undefined Opcode)",
             "Device Not Available (No Math Coprocessor)",
             "Double Fault",
             "Coprocessor Segment Overrun",
             "Invalid TSS",
             "Segment Not Present",
             "Stack-Segment Fault",
             "General Protection",
             "Page Fault",
             "Intel reserved",
             "x87 FPU Floating-Point Error (Math Fault)",
             "Alignment Check",
             "Machine Check",
             "SIMD Floating-Point Exception",
             "Virtualization Exception")

def genvec():
    print """/* -*- mode: gas; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  Trap vector relevant routines                                     */
/*  Note: This file is generated automatically, don't edit by hand.   */
/*                                                                    */
/**********************************************************************/
"""
    for i in range(256):
        if (i < 21):
            print "/*  %s */" % exception_names[i]
        elif (i < 32):
            print "/*  Intel reserved %d  */" % (i - 20)
        else:
            print "/*  External interrupt %d  */" % (i - 32)
        print ".globl vector%d" % (i) ;
        print "vector%d:" % (i) ;
        if (not ( (i == 8) or  ( (i >= 10) and (i <= 14) ) or ( i == 17 ) ) ):
                print "\tpush $0";
        print "\tpush $%d" % i;
        print "\tjmp build_trap_context";
        print ""

    print ".data";
    print ".globl vectors";
    print "vectors:";
    for i in range(256):
        print "  .quad vector%d" % (i);


if __name__ == "__main__":
    genvec()

