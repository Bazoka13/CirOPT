## How to run the combination of CirOPT and SAT solvers

For each SAT solver, simply run the following command, which will invoke CirOPT to convert the circuits into CNFs and then call the corresponding SAT solver to solve them:

```shell
python3 CirOPT+*.py
```

the python file will automatically generate the following folders:

- `work`, which contains orginal and optimized LLVM IRs.
- `cnf`, which contains all the CNFs generated.
- `log`, which contains the execution logs of SAT solver.



