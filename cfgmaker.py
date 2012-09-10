#!/usr/bin/env python

import os
import sys

# ./cfgmaker.py cfg_templ cfg_out

env = {}

###
## ######################################################################
# Load the cfg_templ

for l in open(sys.argv[1]).readlines():
    segs = l.strip().replace(":=", " := ").replace("+=", " += ").split()
    if len(segs) < 2 or not segs[1] in [ ":=", "+=" ]:
        continue
    k = segs[0]

    curv = env.get(k, [])
    if segs[1] == ':=':
        if len(segs) > 2:
            curv = [segs[2]]
    else:
        if len(segs) > 2:
            curv.append(segs[2])
    env[k] = curv

###
## ######################################################################
# Write cfg_out

out = open(sys.argv[2], "wt")
for k, v in env.items():
    out.write("export %s=%s\n" % (k, ":".join(v)))

# PATH
path_seg = env.get("GCC_PATH", [])
path_cur = os.environ.get("PATH", "")
for p in path_cur.split(":"):
    if p not in path_seg:
        path_seg.append(p)

out.write("export PATH=%s\n" % ":".join(path_seg))

