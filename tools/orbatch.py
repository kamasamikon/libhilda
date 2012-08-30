#!/usr/bin/env python

import os, sys
import socket
import time, hashlib

def help():
    s = "USAGE:\torbatch -a server -p port -lo loop-outer -li loop-inner -cmd cmd-file -out out-file\n\n"
    s += "    server:      Default is localhost.\n"
    s += "    port:        Default is 9000.\n"
    s += "    loop-outer:  Outer loop count, default is 1.\n"
    s += "    loop-inner:  Inner loop count, default is 1.\n"
    s += "    cmd-file:    Command file, MUST be set.\n"
    s += "    out-file:    Output file, default is stdout.\n"
    s += "\n"
    s += "    Cmd file line can be:\n"
    s += "        orcmd                  # Normal opt rpc command.\n"
    s += "        OR\n"
    s += "        -inc another-file      # Include another cmd file.\n"
    s += "        OR\n"
    s += "        -loop loop-count       # Loop the block\n"
    s += "        OR\n"
    s += "        -endloop               # Loop till here.\n"
    s += "        OR\n"
    s += "        -sleep millisecond     # Pause execute a while.\n"
    s += "        OR\n"
    s += "        -msg message           # Direct print to out-file.\n"
    s += "        OR\n"
    s += "        -time                  # Print a timestamp to out-file\n"
    s += "        OR\n"
    s += "        -check                 # -check <opt> <wanted>\n"
    sys.stdout.write(s)

def help_and_die():
    help()
    sys.exit(0)

def get_opts():
    serv = get_opt("-a")
    if not serv:
        serv = "localhost"

    port = get_opt("-p")
    if not port:
        port = 9000
    else:
        port = int(port)

    lo = get_opt("-lo")
    if not lo:
        lo = 1
    else:
        lo = int(lo)

    li = get_opt("-li")
    if not li:
        li = 1
    else:
        li = int(li)

    cmd = get_opt("-cmd")
    if not cmd:
        cmd = None

    out = get_opt("-out")
    if not out:
        out = None

    return serv, port, lo, li, cmd, out

def get_opt(name):
    try:
        index = sys.argv.index(name)
    except:
        return None

    if index >= len(sys.argv) - 1:
        return None
    return sys.argv[index + 1]

def hack_hey_command(line):
    segs = line.split()

    if segs[0] == 'hey' and segs[1] == 'o' and segs[2] == 'telnet':
        seed = "%.9f" % time.time()
        hash = hashlib.md5(seed.encode("utf-8")).hexdigest()
        return "hey o telnet %s %s %s" % (hash, segs[4], segs[5])
    else:
        return line

def load_cmds(cmdfile, rootfiles = None):
    cmds = []

    if not rootfiles:
        rootfiles = []

    if cmdfile in rootfiles:
        sys.stdout.write("Error: Recursive include found: rootfiles:%s, cmdfile:%s" % (rootfiles, cmdfile))
        return []

    try:
        f = open(cmdfile, "rt")
        if f:
            for line in f.readlines():
                line = line.lstrip()
                if line:
                    if line.startswith('-inc '):
                        rootfiles.append(cmdfile)
                        inc_cmds = load_cmds(line[5:].strip(), rootfiles)
                        rootfiles.pop()
                        cmds.extend(inc_cmds)
                    else:
                        line = hack_hey_command(line)
                        cmds.append(line)
    except:
        sys.stdout.write("Error: Open cmdfile:%s failed.\r\n" % cmdfile)
        return []
    return cmds

def str_to_bytes(input):
    input = bytes(input, "utf-8")
    return input

def bytes_to_str(input):
    input = input.decode("utf-8");
    return input

def do_cmds(cmds, sock = None):
    line_ofs = 0

    while line_ofs < len(cmds):
        line = cmds[line_ofs]

        if line.startswith("-endloop"):
            line_ofs += 1
            return line_ofs
        elif line.startswith("-loop "):
            try:
                loop = int(line[6:])
            except:
                loop = 1
            for i in range(loop):
                sub_lines = do_cmds(cmds[line_ofs+1:], sock)
            line_ofs += sub_lines
            line_ofs += 1
        elif line.startswith("-sleep "):
            time.sleep(float(line[7:]) / 1000)
            line_ofs += 1
        elif line.startswith("-msg "):
            out.write("<MSG>: " + line[5:] + "\r\n")
            line_ofs += 1
        elif line.startswith("-check "):
            out.write("<CHECK>: " + line[7:].strip() + "\r\n")
            opt, value = line[7:].split()
            if sock:
                sock.send(str_to_bytes("og %s" % opt))
                data = sock.recv(1024 * 512)
                result = data.decode("utf-8").strip().split("\r\n")

                if result[0] != "0 OK":
                    out.write("<CHECK ERR>: OPT<%s>, VAL<%s>, ERROR<%s>\r\n" % (opt, value, result[0]))
                else:
                    if opt[0] in 'ibe':
                        if int(result[1]) != int(value):
                            out.write("<CHECK ERR>: OPT<%s>, VAL<%d>, RET<%d>\r\n" % (opt, int(value), int(result[1])))
                    else:
                        if result[1] != value:
                            out.write("<CHECK ERR>: OPT<%s>, VAL<%s>, RET<%s>\r\n" % (opt, value, result[1]))

            line_ofs += 1
        elif line.startswith("-time"):
            out.write(time.strftime("<TIME>: %Y-%m-%d %H-%M-%S"))
            out.write(" %.9f " % time.time())
            out.write("\r\n")
            line_ofs += 1
        else:
            out.write("<CMD>: " + cmds[line_ofs] + "\r\n")
            line_ofs += 1
            if sock:
                sock.send(str_to_bytes('\n'.join(line.split("\\r\\n"))))
                data = sock.recv(1024 * 512)
                out.write(data.decode("utf-8").strip() + "\r\n")
    return line_ofs

if __name__ == "__main__":

    if "--help" in sys.argv:
        help_and_die()

    serv, port, lo, li, cmd, out = get_opts()
    if not cmd:
        help_and_die()
    if not out:
        out = sys.stdout
    else:
        out = open(out, "wt+")

    cmds = load_cmds(cmd)
    if not cmds:
        help_and_die()

    # do_cmds(cmds)

    loop_o = 0
    while loop_o < lo:
        loop_o += 1

        # Create a socket (SOCK_STREAM means a TCP socket)
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        try:
            sock.connect((serv, port))

            loop_i = 0
            while loop_i < li:
                loop_i += 1
                do_cmds(cmds, sock)
        finally:
            sock.close()

# vim: sw=4 ts=4 sts=4 ai et
