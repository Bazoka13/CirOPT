#!/usr/bin/python3
import sys, os, time, re

pre_path = os.getcwd()
solver_path = pre_path+"/CaDiCaL"
WORK_DIR = pre_path+"/work/" 
BENCH_DIR = pre_path.replace("solvers","benchmarks/")
LOG_DIR = pre_path+"/log/"
CNF_DIR = pre_path+"/cnf/" 

def execmd(cmd):
    pipe = os.popen(cmd)
    reval = pipe.read()
    pipe.close()
    return reval
def delete_dir_if_exists(dir_name):
    if os.path.exists(dir_name):
        execmd('rm -rf ' + dir_name)

pss = "-passes='cgscc(inline),simplifycfg<bonus-inst-threshold=1;no-forward-switch-cond;no-switch-range-to-icmp;no-switch-to-lookup;keep-loops;no-hoist-common-insts;no-sink-common-insts>,early-cse<>,ipsccp,simplifycfg<bonus-inst-threshold=1;no-forward-switch-cond;switch-range-to-icmp;no-switch-to-lookup;keep-loops;no-hoist-common-insts;no-sink-common-insts>,early-cse<memssa>,simplifycfg<bonus-inst-threshold=1;no-forward-switch-cond;switch-range-to-icmp;no-switch-to-lookup;keep-loops;no-hoist-common-insts;no-sink-common-insts>,instcombine,aggressive-instcombine,simplifycfg<bonus-inst-threshold=1;no-forward-switch-cond;switch-range-to-icmp;no-switch-to-lookup;keep-loops;no-hoist-common-insts;no-sink-common-insts>,reassociate,loop-simplifycfg,simplifycfg<bonus-inst-threshold=1;no-forward-switch-cond;switch-range-to-icmp;no-switch-to-lookup;keep-loops;no-hoist-common-insts;no-sink-common-insts>,instcombine,gvn<>,sccp,bdce,instcombine,adce,simplifycfg<bonus-inst-threshold=1;no-forward-switch-cond;switch-range-to-icmp;no-switch-to-lookup;keep-loops;hoist-common-insts;sink-common-insts>,instcombine,globaldce,instcombine,simplifycfg<bonus-inst-threshold=1;forward-switch-cond;switch-range-to-icmp;switch-to-lookup;no-keep-loops;hoist-common-insts;sink-common-insts>,instcombine,instcombine,instsimplify,simplifycfg<bonus-inst-threshold=1;no-forward-switch-cond;switch-range-to-icmp;no-switch-to-lookup;keep-loops;no-hoist-common-insts;no-sink-common-insts>,globaldce'"

def buildCNF(bench_name):
    shell_path = WORK_DIR+bench_name+".sh"
    pure_aig = BENCH_DIR+bench_name+".aig"
    pure_bench = WORK_DIR+bench_name+".aag"
    pure_ir = WORK_DIR+bench_name+"_pure.ll"
    opt_ir = WORK_DIR+bench_name+"_opt.ll"
    opt_bench = WORK_DIR+bench_name+".bench"
    opt_cnf = CNF_DIR+bench_name+".cnf"
    execmd(pre_path+"/aigtoaig -a "+pure_aig+" > "+pure_bench)
    f = open(shell_path,"w")
    f.write("#!/bin/bash\n")
    f.write(pre_path+"/c2ir "+pure_bench+" "+pure_ir+"\n")
    f.write("opt "+pss+" -S "+pure_ir+" -o "+opt_ir+"\n")
    f.write(pre_path+"/ir2c "+opt_ir+" "+opt_bench+"\n")
    f.write(pre_path+"/c2cnf "+opt_bench+" "+opt_cnf+"\n")
    f.close()
    execmd("bash "+shell_path)
    return opt_cnf


def ciropt_test(bench):
    tmpb = str(bench).split('/')[-1].replace(".cnf","")
    log_path = LOG_DIR+str(bench).split('/')[-1].replace(".cnf",".log")
    opt_cnf = buildCNF(tmpb)
    cnf_list = [opt_cnf]
    for cnf in cnf_list:
        if os.path.exists(cnf):
            solver =solver_path
            execmd(solver+" "+cnf+" > "+log_path)

if not os.path.exists(WORK_DIR):
    execmd("mkdir "+WORK_DIR)
if not os.path.exists(CNF_DIR):
    execmd("mkdir "+CNF_DIR)
if not os.path.exists(LOG_DIR):
    execmd("mkdir "+LOG_DIR)
execmd("chmod +x c2ir")
execmd("chmod +x ir2c")
execmd("chmod +x c2cnf")
execmd("chmod +x aigtoaig")
execmd("chmod +x CaDiCaL")

b_list = os.listdir(BENCH_DIR)
for bench in b_list:
    tmpb = bench.replace(".aig","")
    tmp_cnf = CNF_DIR+tmpb+".cnf"
    ciropt_test(tmp_cnf)



