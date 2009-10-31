#!/usr/bin/python
#
# Copyright (C) 2009 Sun Microsystems, Inc.
#
# This file is part of VirtualBox Open Source Edition (OSE), as
# available from http://www.virtualbox.org. This file is free software;
# you can redistribute it and/or modify it under the terms of the GNU
# General Public License (GPL) as published by the Free Software
# Foundation, in version 2 as it comes in the "COPYING" file of the
# VirtualBox OSE distribution. VirtualBox OSE is distributed in the
# hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
#
# Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa
# Clara, CA 95054 USA or visit http://www.sun.com if you need
# additional information or have any questions.
#


import os,sys

versions = ["2.3", "2.4", "2.5", "2.6", "2.7", "2.8"]
prefixes = ["/usr", "/usr/local", "/opt", "/opt/local"]
known = {}

def checkPair(p, v,dllpre,dllsuff, bitness_magic):
    file =  os.path.join(p, "include", "python"+v, "Python.h")
    if not os.path.isfile(file):
        return None

    lib = os.path.join(p, "lib", dllpre+"python"+v+dllsuff)

    if bitness_magic == 1:
        lib64 = os.path.join(p, "lib", "64", dllpre+"python"+v+dllsuff)
    elif bitness_magic == 2:
        lib64 = os.path.join(p, "lib64", dllpre+"python"+v+dllsuff)
        if not os.path.isfile(lib64):
            lib64 = lib
    else:
        lib64 = None
    return [os.path.join(p, "include", "python"+v), 
            lib,
            lib64]

def print_vars(vers, known, sep, bitness_magic):
    print "VBOX_PYTHON%s_INC=%s%s" %(vers, known[0], sep)
    if bitness_magic > 0:
       print "VBOX_PYTHON%s_LIB=%s%s" %(vers, known[2], sep)
    else:
       print "VBOX_PYTHON%s_LIB=%s%s" %(vers, known[1], sep)


def main(argv):
    dllpre = "lib"
    dllsuff = ".so"
    bitness_magic = 0

    if len(argv) > 1:
        target = argv[1]
    else:
        target = sys.platform

    if len(argv) > 2:
        arch = argv[2]   
    else:
        arch = "unknown"

    if target == 'darwin':
        prefixes.insert(0, '/Developer/SDKs/MacOSX10.4u.sdk/usr')
        prefixes.insert(0, '/Developer/SDKs/MacOSX10.5.sdk/usr')
        # Python 2.3 on Darwin buildbox is bad
        # /Developer/SDKs/MacOSX10.4u.sdk/usr/include/python2.3/pyport.h:554:2: error: #error "LONG_BIT definition appears wrong for platform (bad gcc/glibc config?).
        versions.remove("2.3")
        dllsuff = '.dylib'

    if target == 'solaris' and arch == 'amd64':
        bitness_magic = 1

    if target == 'linux' and arch == 'amd64':
        bitness_magic = 2

    for v in versions:
        for p in prefixes:
            c = checkPair(p, v, dllpre, dllsuff, bitness_magic)
            if c is not None:
                known[v] = c
                break
    keys = known.keys()
    # we want default to be the lowest versioned Python
    keys.sort()
    d = None
    # We need separator other than newline, to sneak through $(shell)  
    sep = "|"
    for k in keys:
        if d is None:
            d = k
        vers = k.replace('.', '')
        print_vars(vers, known[k], sep, bitness_magic)
    if d is not None:
        print_vars("DEF", known[d], sep, bitness_magic) 

if __name__ == '__main__':
    main(sys.argv)