#!/usr/bin/env python
# -*- coding: utf-8 -*-

##
# Copyright 2012 Takeharu KATO.  All Rights Reserved. 
# 
##

import sys, codecs, os, re, datetime
import ConfigParser
import csv
import shutil
import subprocess
import datetime
import tempfile
from optparse import OptionParser
from StringIO import StringIO

#
#リダイレクト時にUTF-8の日本語を含む文字を受け入れるための設定
#
sys.stdin  = codecs.getwriter('utf_8')(sys.stdin)
sys.stdout = codecs.getwriter('utf_8')(sys.stdout)
sys.stderr = codecs.getwriter('utf_8')(sys.stderr)

u"""
gen-asm-offset.py [options]
usage: gen-asm-offset.py -h -v 

options:
  --version                          版数情報表示
  -h, --help                         ヘルプ表示
  -v, --verbose                      冗長モードに設定
  -c, --config                       コンフィグファイルを設定
  -i INFILE,  --infile=inputfile     入力ファイル名
  -o OUTFILE, --outfile=outputfile   出力先ファイル名
"""

parser = OptionParser(usage="%prog [options]", version="%prog 1.0")
parser.add_option("-v", "--verbose", action="store_true", dest="verbose",
                  help="set verbose mode")
parser.add_option("-i", "--infile", action="store", type="string", dest="infile", help="path for input-file")
parser.add_option("-o", "--outfile", action="store", type="string", dest="outfile", help="path for output-file")


class genoffset:
    def __init__(self):
        self.cmdname = 'genoffset'
        self.infile = None
        self.outfile = None
        self.verbose = False
        self.msg_log = []
        self.war_log = []
        self.err_log = []
        self.dbg_log = []
        (self.options, self.args) = parser.parse_args()
        self.readConfig()
    def inf_msg(self, msg):
        self.msg_log.append(msg)
    def war_msg(self, msg):
        self.war_log.append(msg)
    def err_msg(self, msg):
        self.err_log.append(msg)
    def dbg_msg(self, msg):
        if self.getVerbose():
            self.dbg_log.append(msg)
    def setVerbose(self):
        self.verbose = True
    def getVerbose(self):
        return self.verbose;
    def getInfile(self):
        return self.infile
    def getOutfile(self):
        return self.outfile
    def outputHead(self, fh):
        fh.write("""/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2014 Takeharu KATO                                      */
/*                                                                    */
/*  structure offset definitions                                      */
/*  Note: This file is generated automatically, don't edit by hand.   */ 
/*                                                                    */
/**********************************************************************/

#if !defined(_HAL_ASM_OFFSET_H)
#define _HAL_ASM_OFFSET_H

""")
    def genAsmOffset(self):

        try:
            fh = open(self.outfile, 'w')
        except IOError:
            print '"%s" cannot be opened.' % self.outfile
        else:
            try:
                f = open(self.infile, 'r')
            except IOError:
                print '"%s" cannot be opened.' % self.infile
            else:
                self.outputHead(fh)
                for line in f:
                    if (re.search(r'@ASM_OFFSET@', line)):
                        line = line.lstrip()
                        line = line.rstrip()
                        line = line.rstrip('\r')
                        line = line.rstrip('\n')
                        line = re.sub(".ascii\s+","",line)
                        line = line.replace("@ASM_OFFSET@","")
                        line = line.lstrip('"')
                        line = line.rstrip('"')
                        defs = re.split('\s+', line ,2)
                        fh.write("#define %s (%s) /* %s */\n" % (defs[0], re.sub('\$*', '', defs[1]), defs[2]))
                fh.write("#endif  /*  _HAL_ASM_OFFSET_H   */\n")
                f.close()
            fh.close() 
        return

    def readConfig(self):
        if self.options.verbose:
            self.setVerbose()

        if self.options.infile and os.path.exists(self.options.infile) and os.path.isfile(self.options.infile):
            self.infile = self.options.infile
        else:
            self.infile="asm-offset.s"

        if self.options.outfile:
            self.outfile = self.options.outfile
        else:
            self.outfile = "asm-offset.h"
            
if __name__ == '__main__':

    obj = genoffset()
    obj.readConfig()
    obj.genAsmOffset()


