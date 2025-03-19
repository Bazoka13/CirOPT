#!/usr/bin/python3
import sys, os, time, re

LOG_DIR = "./script_log/"
WORK_DIR ="./script_work/" 
BENCH_DIR = "../benchmarks/"
XSAT_DIR = "csat"


def execmd(cmd):
    pipe = os.popen(cmd)
    reval = pipe.read()
    pipe.close()
    return reval
def delete_dir_if_exists(dir_name):
    if os.path.exists(dir_name):
        execmd('rm -rf ' + dir_name)



def xsat_test():
    for b in bench_queue:
        print(b)
        bench_and_log = b
        tmpb = bench_and_log[0]
        opt_cnf = BENCH_DIR+tmpb+".aig"
        cnf_list = [opt_cnf]
        for cnf in cnf_list:
            if os.path.exists(cnf):
                watch_log = bench_and_log[1]
                initial_log = bench_and_log[2]
                delete_dir_if_exists(watch_log)
                delete_dir_if_exists(initial_log)
                execmd("./runsolver -C 10000 -w "+watch_log+" -o "+initial_log+" "+XSAT_DIR+" -i "+cnf)


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
    opt_log = LOG_DIR+tmpb+"_opt.log"
    opt_w_log = LOG_DIR+tmpb+"_opt_w.log"
    log_list.append(opt_w_log)
    bench_queue.append([opt_cnf,opt_w_log,opt_log])
xsat_test()

cnt =0
tot_time = 0
for tmp_log in log_list:
    f_w = open(tmp_log,"r",encoding='windows-1252',errors="ignore").readlines()
    v = 0
    fl = 2
    for line in f_w:
        if "CPU time (s): " in line:
            v = float(line.split(' ')[-1])
    for line in f_w:
        if "Child status: 0" in line:
            fl=1
    if fl!=1 or v>10000:
        v=20000
    if fl==1 and v<=10000:
        cnt+=1
    tot_time+=v
print("Solved Number: "+str(cnt))
print("PAR2: "+str(tot_time/len(log_list)))

