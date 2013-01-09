#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os
import sys
import re

def loud_rename(src, dst):
    print "O: '%s'" % src
    print "N: '%s'" % dst
    os.rename(src, dst)
    print

def smart_rename(old, dir, ext, nnnn, yy, rr, ss, ff, mm):
    name = "%04d-%02d-%02d %02d-%02d-%02d" % (int(nnnn), int(yy), int(rr),
            int(ss), int(ff), int(mm))
    newpath = "%s/%s%s" % (dir, name, ext)
    if old == newpath:
        return

    if not os.path.exists(newpath):
        loud_rename(old, newpath)
        return

    for i in range(1, 10000):
        newpath = "%s/%s(%d)%s" % (dir, name, i, ext)
        print "DUP-then-NEXT: %s" % newpath
        if not os.path.exists(newpath):
            loud_rename(old, newpath)
            return

def do_recursive_rename(root, ext, repat):
    def walk_dir(dir, ext, repat):
        for root, dirs, files in os.walk(dir, True):
            for name in files:
                n, e = os.path.splitext(name)
                if e.upper() == ext:
                    m = repat.match(n)
                    if m:
                        old = "%s/%s" % (root, name)
                        smart_rename(old, root, ext, m.group(1), m.group(2),
                                m.group(3), m.group(4), m.group(5), m.group(6))

    walk_dir(root, "." + ext, repat)

if __name__ == "__main__":
    if len(sys.argv) < 3:
        print "fix-datetime-name EXT DIR0 DIR1 ..."
        sys.exit(1)

    ext = sys.argv[1]
    dirs = sys.argv[2:]

    repat = re.compile("(\d+)-(\d+)-(\d+) (\d+)-(\d+)-(\d+)")
    for d in dirs:
        do_recursive_rename(d, ext.upper(), repat)

# vim: sw=4 ts=4 sts=4 ai et
