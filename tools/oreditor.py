#!/usr/bin/env python
# -*- coding: utf-8 -*-
# 2106031979

import os
import sys
import time
import socket
import hashlib
import re
import subprocess
try:
    from Tkinter import *
except:
    from tkinter import *

opts_all = []
opts_shown = []
out = sys.stdout

def help():
    s = "USAGE:\toredit -a server -p port -hey hey -ol optlist\n\n"
    s += "    server:      Default is localhost.\n"
    s += "    port:        Default is 9000.\n"
    s += "    hey:         The hey command to orserv.\n"
    s += "    optlist:     opt to get opt list, default is 's:/k/opt/diag/list'.\n"
    sys.stdout.write(s)

def help_and_die():
    help()
    sys.exit(0)

def add_opt_to_list(txt):
    opts_all.append(txt)

def show_dialog():
    root = Tk()
    root.option_add("*Font", "Courier")
    root.title("oreditor - A editor for opt rpc")
    root.geometry("480x640")

    paned = PanedWindow(root, orient=VERTICAL)
    paned.pack(fill=BOTH, expand=1)

    def update_opt_list(pattern):
        lb_list.delete(0, END)

        global opts_shown
        global opts_all

        opts_shown = []

        if pattern:
            for opt in opts_all:
                if opt.find(pattern) >= 0:
                    opts_shown.append(opt)
        else:
            opts_shown = opts_all[:]

        index = 0
        for opt in opts_shown:
            lb_list.insert(index, opt)
            index += 1
        lb_list.update()

    #
    # Add Opt List Panel
    #
    frame_list = LabelFrame(paned, text="Opts", relief=SUNKEN)
    paned.add(frame_list)

    def do_focus_optlist(*event):
        lb_list.focus_set()
        lb_list.select_set(0)
        lb_list.activate(0)

    def do_focus_filter(*event):
        entry_filter.focus_set()
        entry_filter.select_range(0, END)

    def do_focus_optset(*event):
        entry_optset.focus_set()
        entry_optset.select_range(0, END)

    def do_update_filter(*event):
        if event[0].keysym== "Escape":
            entry_filter.delete(0, END)

        text = entry_filter.get()
        update_opt_list(text)

    entry_filter = Entry(frame_list)
    entry_filter.pack(side = 'top', expand = 0, fill = "both")
    entry_filter.bind("<KeyRelease>", do_update_filter)
    entry_filter.bind("<Control-f>", do_focus_filter)
    entry_filter.bind("<Control-l>", do_focus_optlist)
    entry_filter.bind("<Control-o>", do_focus_optset)

    frm_list_body = Frame(frame_list)
    frm_list_body.pack(side = 'top', expand = 1, fill = "both")

    lb_list = Listbox(frm_list_body)
    lb_list.pack(side = 'left', expand = 1, fill = "x")
    sb_list = Scrollbar(frm_list_body)
    sb_list.pack(side = RIGHT, fill = Y)
    lb_list['yscrollcommand'] = sb_list.set
    sb_list['command'] = lb_list.yview
    #lb_list.pack(expand=1, fill=BOTH)

    update_opt_list(None)

    def do_set(*event):
        index = int(lb_list.curselection()[0])
        text = opts_shown[index]
        do_os(text)

    def do_get(*event):
        index = int(lb_list.curselection()[0])
        text = opts_shown[index]
        do_og(text)

    def do_og(opt):
        cmd = "og %s" % opt
        label_output.config(text = cmd)
        sock.send(str_to_bytes(cmd))
        resp = bytes_to_str(sock.recv(1024 * 512))
        output(resp)

    def do_os(opt):
        value = entry_optset.get().strip()
        if  value:
            cmd = "os %s=%s" % (opt, value)
            sock.send(str_to_bytes(cmd))
            resp = bytes_to_str(sock.recv(1024 * 512))
            output(resp)
        else:
            cmd = "os %s=%s" % (opt, "Null")
        label_output.config(text = cmd)

    def output(text):
        # txt_output.config(state=NORMAL)
        txt_output.delete(1.0, END)
        txt_output.insert(INSERT, text)
        # txt_output.config(state=DISABLED)

    def do_keypress(*event):
        if event[0].keysym == "j":
            new_index = int(lb_list.curselection()[0]) + 1
        elif event[0].keysym == "k":
            new_index = int(lb_list.curselection()[0]) - 1
        elif event[0].keysym == "0":
            new_index = 0
        elif event[0].keysym == "G":
            new_index = lb_list.size() - 1
        else:
            return

        if new_index >= lb_list.size():
            new_index = lb_list.size() - 1
        if new_index < 0:
            new_index = 0
        lb_list.select_clear(0, END)
        lb_list.select_set(new_index)
        lb_list.activate(new_index)
        lb_list.see(new_index)

    # <MODIFIER-MODIFIER-TYPE-DETAIL>
    #
    # MODIFIER =>
    #   Control, Mod2, M2, Shift, Mod3, M3, Lock, Mod4, M4,
    #   Button1, B1, Mod5, M5 Button2, B2, Meta, M, Button3,
    #   B3, Alt, Button4, B4, Double, Button5, B5 Triple,
    #   Mod1, M1.
    #
    # TYPE =>
    #   Activate, Enter, Map,
    #   ButtonPress, Button, Expose, Motion, ButtonRelease
    #   FocusIn, MouseWheel, Circulate, FocusOut, Property,
    #   Colormap, Gravity Reparent, Configure, KeyPress, Key,
    #   Unmap, Deactivate, KeyRelease Visibility, Destroy,
    #   Leave
    #
    # DETAIL =>
    #   the button number for ButtonPress,

    lb_list.bind("<Double-Button-1>", do_get)
    lb_list.bind("<Return>", do_get)
    lb_list.bind("<Control-g>", do_get)
    lb_list.bind("<Control-s>", do_set)
    lb_list.bind("<KeyPress>", do_keypress)
    lb_list.bind("<Control-f>", do_focus_filter)
    lb_list.bind("<Control-l>", do_focus_optlist)
    lb_list.bind("<Control-o>", do_focus_optset)

    #
    # Add Output Panel
    #
    frame_output = LabelFrame(paned, text="Console")
    paned.add(frame_output, min=70)

    label_output = Label(frame_output)
    label_output.pack(side = 'top', expand = 0, fill = "x")
    label_output.config(relief=SUNKEN, anchor = "w", foreground="red")
    label_output.config(padx=2, pady=2, borderwidth=2)

    frm_output_body = Frame(frame_output)
    frm_output_body.pack(side = 'top', expand = 1, fill = "both")

    txt_output = Text(frm_output_body)
    txt_output.pack(side = 'left', expand = 1, fill = "x")
    sb_output = Scrollbar(frm_output_body)
    sb_output.pack(side = RIGHT, fill = Y)
    txt_output['yscrollcommand'] = sb_output.set
    sb_output['command'] = txt_output.yview
    txt_output.bind("<Control-f>", do_focus_filter)
    txt_output.bind("<Control-l>", do_focus_optlist)
    txt_output.bind("<Control-o>", do_focus_optset)

    #
    # Add OptSet Output Panel
    #
    frame_optset = LabelFrame(paned, text="Optset")
    paned.add(frame_optset, min=40)
    paned.paneconfig(frame_optset, before=frame_output)

    label_optset = Label(frame_optset, text="Value:")
    #label_optset.config(text = cmd)
    label_optset.pack(side = 'left', expand = 0, fill = "x")

    entry_optset = Entry(frame_optset)
    entry_optset.pack(side = 'right', expand = 1, fill = "both")
    entry_optset.bind("<Control-f>", do_focus_filter)
    entry_optset.bind("<Control-l>", do_focus_optlist)
    entry_optset.bind("<Control-o>", do_focus_optset)

    root.mainloop()

def str_to_bytes(input):
    input = bytes(input, "utf-8")
    return input

def bytes_to_str(input):
    input = input.decode("utf-8");
    return input

def say_hey(heystr = None):
    if not heystr:
        seed = "%.9f" % time.time()
        hash = hashlib.md5(seed.encode("utf-8")).hexdigest()
        heystr = "hey o telnet %s auv auv" % hash
    sock.send(str_to_bytes(heystr))
    data = bytes_to_str(sock.recv(1024 * 512))

def get_opt_list(ol):
    if not ol:
        ol = "s:/k/opt/diag/list"
    ol = "og %s" % ol
    sock.send(str_to_bytes(ol))
    data = bytes_to_str(sock.recv(1024 * 512))
    for opt in data.split():
        if ":" in opt:
            add_opt_to_list(opt)

def get_opt(name):
    try:
        index = sys.argv.index(name)
    except:
        return None

    if index >= len(sys.argv) - 1:
        return None
    return sys.argv[index + 1]

def get_opts():
    serv = get_opt("-a")
    if not serv:
        serv = "localhost"

    port = get_opt("-p")
    if not port:
        port = 9000
    else:
        port = int(port)

    hey = get_opt("-hey")
    optlist = get_opt("-ol")

    return serv, port, hey, optlist

if __name__ == "__main__":
    if "--help" in sys.argv:
        help_and_die()

    while True:
        serv, port, heystr, ol = get_opts()
        out.write("Connect to %s:%d\r\n" % (serv, port))
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        try:
            opts_all = []
            opts_shown = []
            sock.connect((serv, port))

            say_hey(heystr)
            get_opt_list(ol)

            show_dialog()
        except KeyboardInterrupt:
            break
        except:
            time.sleep(1)
            continue
        finally:
            sock.close()

# vim: sw=4 ts=4 sts=4 ai et
