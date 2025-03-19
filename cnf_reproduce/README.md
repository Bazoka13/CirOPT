#### CirOPT
- Change AIG_DIR (Line 7) to the path of your local `aigtoaig` binary file
- Change SOLVER_DIR (Line 8) to the path of your local CNF SAT solver binary file
- Change OPT_DIR (Line 10) to the path of your local LLVM opt binary file
- Run ciropt_script.py
- The number of successfully solved benchmarks (xxx) and PAR2 time (yyy.yy) will be output after the run is completed.
    - For example,
        ```
        Solved Number: xxx 
        PAR2: yyy.yy
        ```

#### AIGTOCNF
- Change AIG_DIR (Line 7) to the path of your local `aigtocnf` binary file
- Change SOLVER_DIR (Line 8) to the path of your local CNF SAT solver binary file
- Run aig_script.py
- The number of successfully solved benchmarks (xxx) and PAR2 time (yyy.yy) will be output after the run is completed.
    - For example,
        ```
        Solved Number: xxx 
        PAR2: yyy.yy
        ```