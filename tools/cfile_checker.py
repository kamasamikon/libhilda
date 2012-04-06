#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os
import sys
import re
import subprocess
try:
    from Tkinter import *
except:
    from tkinter import *

total_files = 0
total_lines = 0
total_funcs = 0
total_bad_funcs = 0
total_gt_30 = 0
total_gt_50 = 0
total_gt_100 = 0
total_2wide = 0
total_mixed = 0

opt_show_color = True   # show high light
opt_tabstop = 4         # default tabstop
opt_func_each = True    # show each function's info
opt_func_all = True     # show __all__ section
opt_summary_f = True    # show Summary for file
opt_summary_r = True    # show Summary for run
opt_use_gui = True      # use gui to show summary
opt_recursive = False
opt_max_width = 80

allines = []

def add_line(path, txt = ""):
    allines.append([path, txt])

def print_line():
    for line in allines:
        sys.stdout.write(line[1] + "\n");

def show_old_fun_info(path, name, pos, line, toowide, mix_line):
    if name:
        if opt_show_color:
            if line > 100:
                line = "\033[1;31m%-3d\033[0m" % line
            elif line > 50:
                line = "\033[1;35m%-3d\033[0m" % line
            elif line > 30:
                line = "\033[1;33m%-3d\033[0m" % line
            else:
                line = "%-3d" % line

            if toowide:
                toowide = "\033[1;31m%-3d\033[0m" % toowide
            else:
                toowide = "%-3d" % toowide

            if mix_line:
                mix_line = "\033[1;31m%-3d\033[0m" % mix_line
            else:
                mix_line = "%-3d" % mix_line
        else:
            line = "%-3d" % line
            toowide = "%-3d" % toowide
            mix_line = "%-3d" % mix_line

        fmt = "%40s pos:%-4d lines:%s wide:%s mix:%s"
        add_line(path, fmt % (name, pos, line, toowide, mix_line))

def get_tabstop_size(strip_line):
    # when error return -1
    tabstop = -1
    if strip_line.startswith('/*') and strip_line.find('vim') > 0:
        start = strip_line.find('ts=')

        if start == -1:
            start = strip_line.find('tabstop=')
            tabstop = int(strip_line[start+8])
        else:
            tabstop = int(strip_line[start+3])

    return tabstop

def calc_line_width(line, tabstop):
    width = 0

    for c in line:
        if c == '\t':
            width += tabstop
            width -= width % tabstop
        else:
            width += 1

    return width

def check_mix_line(line, tabstop):
    space_count = 0
    tab_count = 0

    for c in line:
        if c == '\t':
            if space_count:
                return True
            tab_count += 1
        elif c == ' ':
            space_count += 1
        else:
            break

    if tab_count:
        if space_count >= tabstop:
            return True

    return False

def check_file(path, def_tabstop=4):

    # k => fun_name, v => [tab_space_mix, line_count, long_line_count]
    fun_list = []
    sys.stderr.write("check_file: %s\r\n" % path)

    try:
        f = open(path, "rt")
    except:
        sys.stderr.write("check_file: error open: %s\r\n" % path)
        return

    global total_files
    global total_lines
    global total_funcs
    global total_bad_funcs
    global total_gt_30
    global total_gt_50
    global total_gt_100
    global total_2wide
    global total_mixed

    try:
        lines = f.readlines()
    except:
        sys.stderr.write("check_file: error readlines: %s\r\n" % path)
        return

    fun_name = None

    fun_pos = 0
    fun_line = 0
    fun_2wide = 0
    fun_2tall = 0
    fun_mixed = 0

    all_line = -1
    all_2wide = 0
    all_2tall = 0
    all_mixed = 0
    all_funcs = 0
    all_bad_funs = 0

    all_gt_30 = 0
    all_gt_50 = 0
    all_gt_100 = 0

    tabstop = def_tabstop
    mode_line_ts = -1
    in_func = False

    stats_list = []

    for line in lines:
        all_line += 1
        strip_line = line.strip()

        # get tabstop in mode line
        if all_line < 4 and mode_line_ts == -1:
            mode_line_ts = get_tabstop_size(strip_line)
            if mode_line_ts != -1:
                tabstop = mode_line_ts
                continue

        # line end with a \n
        if calc_line_width(line, tabstop) > (opt_max_width + 1):
            is_long_line = True
            all_2wide += 1
        else:
            is_long_line = False

        if check_mix_line(line, tabstop):
            is_mix_line = True
            all_mixed += 1
        else:
            is_mix_line = False

        if not in_func:
            if ';' in line:
                fun_name = None

            # search start of a function
            if line.rstrip() == "{" and fun_name:
                fun_pos = all_line

                in_func = True
                all_funcs += 1
                fun_line = 0
                fun_2wide = 0
                fun_mixed = 0
            else:
                pats = strip_line.replace('(', ' ( ').replace('#', ' # ').replace('*', '').split()
                # should skip #define xxxx
                if ';' not in line and pats and pats[0] != '#' and '(' in pats:
                    pos = pats.index('(')
                    fun_name = pats[pos - 1]
                    tmp_name = fun_name.replace('_', 'x').replace('-', '1').replace('+', '1')
                    if tmp_name[0].isdigit() or not tmp_name.isalnum():
                        fun_name = None
        else:

            if line.rstrip() == "}":
                if fun_2wide or fun_mixed:
                    all_bad_funs += 1
                if opt_func_each:
                    stats_list.append([fun_name, fun_pos, fun_line, fun_2wide, fun_mixed])

                if fun_line > 30:
                    all_gt_30 += 1
                if fun_line > 50:
                    all_gt_50 += 1
                if fun_line > 100:
                    all_gt_100 += 1

                in_func = False
                fun_name = None
                continue

            fun_line += 1

            if is_long_line:
                fun_2wide += 1

            if is_mix_line:
                fun_mixed += 1

            continue

    if all_funcs == 0:
        return

    if opt_func_all:
        stats_list.append(["__all__", 0, all_line, all_2wide, all_mixed])

    for item in stats_list:
        show_old_fun_info(path, item[0], item[1], item[2], item[3], item[4])

    total_files += 1
    total_lines += all_line
    total_funcs += all_funcs
    total_bad_funcs += all_bad_funs
    total_gt_30 += all_gt_30
    total_gt_50 += all_gt_50
    total_gt_100 += all_gt_100
    total_2wide += all_2wide
    total_mixed += all_mixed

    if opt_summary_f:
        fmt = "Summary: file:(%s), bad_funcs:(%d or %%%f), bad_lines:(%d or %%%f)"
        add_line(path, fmt % (path, all_bad_funs, (1.0 * all_bad_funs * 100 / all_funcs), \
                all_2wide + all_mixed, \
                (1.0 * (all_2wide + all_mixed) * 100 / all_line)))
        add_line("", "")

def do_recursive_check():
    def walk_dir(dir, topdown = True):
        for root, dirs, files in os.walk(dir, topdown):
            for name in files:
                if name.endswith(".c") or name.endswith(".cpp") or name.endswith(".cxx"):
                    check_file(os.path.join(root, name), opt_tabstop)
    walk_dir(".")

def show_dialog():
    root = Tk()
    root.option_add("*Font", "Courier")
    root.geometry("800x480")

    lb = Listbox(root)
    sl = Scrollbar(root)
    sl.pack(side = RIGHT, fill = Y)
    lb['yscrollcommand'] = sl.set

    index = 0
    for l in allines:
        lb.insert(index, l[1])
        index += 1
    lb.pack(expand=1, fill=BOTH)
    sl['command'] = lb.yview

    def open_file(path, text):
        reg = re.compile(" pos:\d+ ")
        m = reg.search(text)
        if not m:
            return

        line = int(m.group(0)[5:])

        vimpath = "gvim"
        try:
            vimpath = os.environ["VIMPATH"]
        except:
            pass

        subprocess.Popen([vimpath, path, "-c:%d" % line])

    def do_dbclick(*event):
        index = int(lb.curselection()[0])
        path = allines[index][0]
        text = allines[index][1]
        open_file(path, text)

    def do_keypress(*event):
        if event[0].keysym == "j":
            new_index = int(lb.curselection()[0]) + 1
        elif event[0].keysym == "k":
            new_index = int(lb.curselection()[0]) - 1
        elif event[0].keysym == "0":
            new_index = 0
        elif event[0].keysym == "G":
            new_index = lb.size() - 1
        else:
            return

        if new_index >= lb.size():
            new_index = lb.size() - 1
        if new_index < 0:
            new_index = 0
        lb.select_clear(0, END)
        lb.select_set(new_index)
        lb.activate(new_index)
        lb.see(new_index)

    lb.bind("<Double-Button-1>", do_dbclick)
    lb.bind("<Return>", do_dbclick)
    lb.bind("<KeyPress>", do_keypress)

    root.mainloop()

if __name__ == "__main__":

    add_line("", "usage: cfc.py -ts 8 -line 80 -no-fe -no-fa -no-sf -no-sr -no-cr -- file.c ...")
    add_line("", "usage: cfc.py -ts 8 -line 80 -no-fe -no-fa -no-sf -no-sr -no-cr -r")
    add_line("", "note: please set environment VIMPATH=<your-fullpath-of-gvim>")
    add_line("", "")

    if '--help' in sys.argv:
        print_line()
        sys.exit(0)

    if '--' in sys.argv:
        fl_pos = sys.argv.index('--')
    else:
        fl_pos = 1
    files = sys.argv[fl_pos:]

    for i in range(len(sys.argv)):
        if sys.argv[i] == '-ts':
            opt_tabstop = int(sys.argv[i + 1]) + 1
        elif sys.argv[i] == '-line':
            opt_max_width = int(sys.argv[i + 1]) + 1
        elif sys.argv[i] == '-no-cr':
            opt_show_color = False
        elif sys.argv[i] == '-no-fe':
            opt_func_each = False
        elif sys.argv[i] == '-no-fa':
            opt_func_all = False
        elif sys.argv[i] == '-no-sf':
            opt_summary_f = False
        elif sys.argv[i] == '-no-sr':
            opt_summary_r = False
        elif sys.argv[i] == '-no-gui':
            opt_use_gui = False
        elif sys.argv[i] == '-r':
            opt_recursive = True
        elif sys.argv[i] == '--':
            break

    if opt_use_gui:
        opt_show_color = False

    if "linux" not in sys.platform:
        opt_show_color = False

    if not opt_recursive:
        for path in files:
            check_file(path, opt_tabstop)
    else:
        do_recursive_check()

    if opt_summary_r:
        add_line("", "ThisRun: ")
        add_line("", "        files: %d" % total_files)
        add_line("")
        add_line("", "        functions: %d" % total_funcs)
        add_line("", "        bad functions: %d or %%%f" % (total_bad_funcs, 1.0 * total_bad_funcs * 100 / total_funcs))
        add_line("", "        functions (lines > 30): %d or %%%f" % (total_gt_30, 1.0 * total_gt_30 * 100 / total_funcs))
        add_line("", "        functions (lines > 50): %d or %%%f" % (total_gt_50, 1.0 * total_gt_50 * 100 / total_funcs))
        add_line("", "        functions (lines > 100): %d or %%%f" % (total_gt_100, 1.0 * total_gt_100 * 100 / total_funcs))
        add_line("")
        add_line("", "        lines: %d" % total_lines)
        add_line("", "        2wide lines: %d or %%%f" % (total_2wide, 1.0 * total_2wide * 100 / total_lines))
        add_line("", "        mixed lines: %d or %%%f" % (total_mixed, 1.0 * total_mixed * 100 / total_lines))
        add_line("")
        print_line()

        sys.stdout.write("usage: cfc.py -ts 8 -line 80 -no-fe -no-fa -no-sf -no-sr -no-cr -- file.c ...\n")
        if opt_use_gui:
            show_dialog()


# vim: sw=4 ts=4 sts=4 ai et
