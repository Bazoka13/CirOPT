#!/usr/bin/python3
import sys, os, time, re

LOG_DIR = "./script_log/"
WORK_DIR ="./script_work/" 
BENCH_DIR = "../benchmarks/"
AIG_DIR = "aigtocnf"
SOLVER_DIR = "kissat"
CNF_DIR = "./cnf/" 



def execmd(cmd):
    pipe = os.popen(cmd)
    reval = pipe.read()
    pipe.close()
    return reval
def delete_dir_if_exists(dir_name):
    if os.path.exists(dir_name):
        execmd('rm -rf ' + dir_name)

def buildPath(bench_name):
    shell_path = WORK_DIR+bench_name+".sh"
    pure_aig = BENCH_DIR+bench_name+".aig"
    opt_cnf = CNF_DIR+bench_name+".cnf"
    f = open(shell_path,"w")
    f.write("#!/bin/bash\n")
    f.write(AIG_DIR+" --no-pg "+pure_aig+" > "+opt_cnf)
    f.close()
    return shell_path



def aig_test():
    for b in bench_queue:
        print(b)
        bench_and_log = b
        tmpb = bench_and_log[0]
        opt_cnf = CNF_DIR+tmpb+".cnf"
        shell_path = buildPath(tmpb)
        cnf_list = [opt_cnf]
        for cnf in cnf_list:
            if os.path.exists(cnf):
                watch_log = bench_and_log[2][0]
                initial_log = bench_and_log[2][1]
                build_watch_log = bench_and_log[1][0]
                build_initial_log = bench_and_log[1][1]
                delete_dir_if_exists(build_watch_log)
                delete_dir_if_exists(build_initial_log)
                execmd("./runsolver -C 10000 -w "+build_watch_log+" -o "+build_initial_log+" bash "+shell_path)
                delete_dir_if_exists(watch_log)
                delete_dir_if_exists(initial_log)
                execmd("./runsolver -C 10000 -w "+watch_log+" -o "+initial_log+" "+SOLVER_DIR+" "+cnf)


bench_queue = []
total_num = 0
log_list = []

execmd("mkdir "+LOG_DIR)
execmd("mkdir "+WORK_DIR)
execmd("chmod +x ./runsolver")

f=os.listdir("../benchmarks/")
for bench in f:
    if not ".aig" in bench:
        continue
    tmpb = bench.replace(".aig","")
    opt_cnf = tmpb
    build_log = LOG_DIR+tmpb+"_build.log"
    build_w_log = LOG_DIR+tmpb+"_build_w.log"
    opt_log = LOG_DIR+tmpb+"_opt.log"
    opt_w_log = LOG_DIR+tmpb+"_opt_w.log"
    log_list.append((build_w_log,opt_w_log))
    bench_queue.append([opt_cnf,(build_w_log,build_log),(opt_w_log,opt_log)])
aig_test()

cnt =0
tot_time = 0
for tmp_log in log_list:
    b_w = open(tmp_log[0],"r",encoding='windows-1252',errors="ignore").readlines()
    f_w = open(tmp_log[1],"r",encoding='windows-1252',errors="ignore").readlines()
    v_b = 0
    v = 0
    fl = 2
    for line in b_w:
        if "CPU time (s): " in line:
            v_b = float(line.split(' ')[-1])
    for line in f_w:
        if "CPU time (s): " in line:
            v = float(line.split(' ')[-1])
    for line in f_w:
        if "Child status: 0" in line:
            fl=1
    v+=v_b
    if fl!=1 or v>10000:
        v=20000
    if fl==1 and v<=10000:
        cnt+=1
    tot_time+=v
print("Solved Number: "+str(cnt))
print("PAR2: "+str(tot_time/len(log_list)))

