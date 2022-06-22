from fabric import Connection
from multiprocessing import Pool
from itertools import repeat
import os
from run_constants import *
import sys

if len(sys.argv) < 2:
    print("Usage: python run_exp.py <exp_name>")
    exit(0)

what_to_do = sys.argv[1]

def run_cmd(c, cmd, pty=False):
    # print('running command on {}. cmd: {}'.format(c.host, cmd))

    result = c.run(cmd, pty=pty)
    # print(result)

def create_nsuser(c):
    cmds = [
        'sudo adduser nsuser',
        'sudo usermod -aG sudo nsuser'
            ]
    for cmd in cmds:
        run_cmd(c, cmd, pty=True)

def install_ns2(c):
    # the following commented set of commands cannot be run in parallel as they  
    # require interaction. Uncomment them if you need a full install on a machine
    # and run install_ns2 in a sequential manner.
    cmds = [
            # 'sudo apt install build-essential autoconf automake libxmu-dev',
            # 'echo "deb http://in.archive.ubuntu.com/ubuntu bionic main universe" | sudo tee -a /etc/apt/sources.list',
            # 'sudo apt update',
            # 'sudo apt install gcc-4.8 g++-4.8'
            ]
    for cmd in cmds:
        run_cmd(c, cmd, pty=True)
    with c.cd(REPO_DIR):
        run_cmd(c, './install > /dev/null')
        
        cmd = 'export PATH=$PATH:/home/{}/ns-allinone-2.34/bin:/home/{}/ns-allinone-2.34/tcl8.4.18/unix:/home/{}/ns-allinone-2.34/tk8.4.18/unix'
        cmd = cmd.format(USER,USER,USER)
        run_cmd(c, 'echo "{}" | tee -a ~/.bashrc'.format(cmd))
        
        cmd = 'export LD_LIBRARY_PATH=/home/{}/ns-allinone-2.34/otcl-1.13:/home/{}/ns-allinone-2.34/lib'
        cmd = cmd.format(USER,USER)
        run_cmd(c, 'echo "{}" | tee -a ~/.bashrc'.format(cmd))
    
def sync_files(m_ip, dir_to_sync):
    print('sync with {}'.format(m_ip))
    cmd = 'rsync -az {} {}@{}:/home/{}/ns-allinone-2.34/ns-2.34/'.format(dir_to_sync, USER, m_ip, USER)
    os.system(cmd)

def sync_repo_and_make(c, m_ip):
    run_cmd(c, "rm -rf /home/{}/ns-allinone-2.34/ns-2.34/".format(USER))

    sync_files(m_ip, REPO_DIR + "ns-2.34/")
    # with c.cd(REPO_DIR + "ns-2.34"):
    #     c.run("make clean")
    #     c.run("make -j64")

def killallns(c, m_ip):
    run_cmd(c, "killall ns")
    run_cmd(c, "killall rsync")


def show_stat(c, m_ip):
    print(m_ip, ":")
    run_cmd(c, "uptime")
    run_cmd(c, "echo -n 'wc output' && ps aux | grep spine | wc")
    run_cmd(c, "cat /proc/meminfo | head -n 3 | tail -n 1;")
    print("----------------------------")


def del_files(c, dir_to_remove):
    if (dir_to_remove == "./" or dir_to_remove == "/"):
        print("please try to be more specific about the address")
        return
    run_cmd(c, "rm -rf {}".format(dir_to_remove))

if __name__ == "__main__":
    connections = {}
    for m_ip in MACHINES:
        connections[m_ip] = Connection('{}@{}'.format(USER, m_ip))
    # For parallel runs
    with Pool(len(MACHINES)) as p:
        # p.starmap(sync_files, zip([m_ip for m_ip in MACHINES], repeat(REPO_DIR)))
        # p.map(install_ns2, connections.values())
        # p.starmap(run_cmd, zip(connections.values(), repeat("sudo pkill -f ns-allinone")))
        # p.starmap(del_files, zip(connections.values(), repeat("/home/{}/ns-allinone-2.34/ns-2.34/ns-test/exps/".format(USER))))
        if what_to_do == "sync":
            p.starmap(sync_repo_and_make, zip(connections.values(), connections.keys()))

        if what_to_do == "killall":
            p.starmap(killallns, zip(connections.values(), connections.keys()))


    # For sequential runs
    for m_ip in MACHINES:
        c = connections[m_ip]
        # print(m_ip)
        # killallns(c, m_ip)
        if what_to_do == "showstat":
            show_stat(c, m_ip)
    #     install_ns2(c)
    #     os.system('ssh-copy-id {}@{}'.format(USER, m_ip)) 