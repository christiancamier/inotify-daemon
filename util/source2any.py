#!/usr/bin/env python3

import sys
import os
import re
import getopt

class converter(object):
    _is_end   = re.compile('^#!END$').match
    _is_start = re.compile('^#!START$').match
    _is_macro = re.compile('^([\w_]+)\s*.=\s*(\S*)$').match
    _is_defin = re.compile('^([\w_]+)=(\S.*)$').match

    def __init__(self):
        self._macros = list()
        return;

    def __str__(self):
        str = ''
        for (k, v) in self._macros:
            str += "{0:s} = {1:s}\n".format(k, v)
        return str;

    def _process_model(self, fd):
        for line in fd:
            line = line.strip('\n\r\t ')
            if self._is_end(line):
                break
            mtch = self._is_macro(line)
            if not mtch:
                continue
            macro = mtch.group(1)
            value = mtch.group(2)
            value = self._translate(value)
            self._macros.append((macro, value, ))
        return

    def _translate(self, string):
        for (m, v) in self._macros:
            string = string.replace('${' + m + '}', v).replace('$(' + m + ')', v)
        return string

    def add_define(self, define):
        mtch = self._is_defin(define)
        if not mtch:
            raise ValueError("'{0:s}' bad define".format(define))
        macro = mtch.group(1)
        value = mtch.group(2)
        value = self._translate(value)
        self._macros.append((macro, value, ))
        return;

    def add_model(self, model):
        with open(model, 'r') as fd:
            for line in fd:
                line = line.strip('\n\r\t ')
                if self._is_start(line):
                    self._process_model(fd)
                    break
        return

    def process(self, source):
        if not source.endswith('.source'):
            sys.stderr.write('{:s}: not a .source file'.format(source))
            return
        target = source[:-7]
        with open(target, 'w') as fdout:
            with open(source, 'r') as fdinp:
                fdout.write(self._translate(fdinp.read()))
        return

def usage(status):
    writer = 0 == status and sys.stdout.write or sys.stderr.write
    writer('Usage :\n')
    writer('\n')
    exit(status)

def main():
    try:
        opts, args = getopt.getopt(sys.argv[1:], 'D:M:h')
    except getopt.GetoptError as err:
        sys.stderr.write(str(err))
        options.display_usage(1)

    processor = converter()
    for o, a in opts:
        if     o in ('-D', '--define' ): processor.add_define(a)
        elif   o in ('-M', '--model'  ): processor.add_model(a)
        elif   o in ('-h', '--help'   ): usage(0)
        else:                            usage(2)

    #print(str(processor))

    if not args: usage(3)
    for F in args:
        processor.process(F)
    return

if '__main__' == __name__:
    main()
