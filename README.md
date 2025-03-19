## CirOPT Readme

Welcome to the homepage of CirOPT!

There are six folders in the root folder. 

### Source Code
`src/` is the folder containing the source code of CirOPT. More specifically, there are four files:
- `c2ir.cpp`, is code for the frontend translation component in CirOPT.
- `ir2c.cpp`, is code for the backend translation component in CirOPT.
- `func.ll`, is code for gates' patterns.
- `c2cnf.cpp`, is code for the Tseitin encoding component in CirOPT.
### Benchmarks
`benchmarks/` is the folder containing all the miter circuits in our experiment. We used all circuits from HWMCC'12/15, ITC'99, IWLS'05/22, and ISCAS'85/89. We also employed a set of arithmetic circuits. 
### Combination of CirOPT with Different SAT Solvers

`solvers/` is the folder containing the binary executable files of SAT solvers in our experiment, the binary executable files of CirOPT, and the implementations of combining CirOPT with the SAT solvers.

- `CirOPT+Kissat.py`, the implementations of combining CirOPT with [Kissat](https://github.com/arminbiere/kissat).
- `CirOPT+MiniSAT.py`, the implementations of combining CirOPT with [MiniSAT](https://github.com/niklasso/minisat).
- `CirOPT+CaDiCaL.py`, the implementations of combining CirOPT with [CaDiCaL](https://github.com/arminbiere/cadical).
- `CirOPT+MAB-DC.py`, the implementations of combining CirOPT with [Kissat_MAB-DC](https://github.com/Jinjin680/Kissat_MAB-DC).
- `CirOPT+Lingeling.py`, the implementations of combining CirOPT with [Lingeling](https://github.com/arminbiere/lingeling).
- `WORK.md`, the description of running the combination of CirOPT and SAT solvers.
### Reproduce
`abc_reproduce/` is the folder containing relevant files for reproducing the experimental results of ABC. For reproducing the results, users may refer to the instructions in `abc_reproduce/README.md`.
`xsat_reproduce/` is the folder containing relevant files for reproducing the experimental results of X-SAT. For reproducing the results, users may refer to the instructions in `xsat_reproduce/README.md`.
`cnf_reproduce/` is the folder containing relevant files for reproducing the experimental results of CirOPT and AIGTOCNF. For reproducing the results, users may refer to the instructions in `cnf_reproduce/README.md`.



Thanks!
