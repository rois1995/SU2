# Test cases of PR #2941 (meanFlowFixes)

Meshes, configuration files and scripts of the test cases posted in the comments of
[su2code/SU2#2941](https://github.com/su2code/SU2/pull/2941).

| Folder | Fix | Configs | Expected result |
|---|---|---|---|
| `1a_nozzle_restart` | under-relaxation | `case.cfg` (restart from `solution_flow.dat`, 300 it.) | develop drifts away from the converged solution, meanFlowFixes stops at -12 after 10 it. |
| `1b_nozzle_from_rest` | under-relaxation | `case.cfg` (up to 20 000 it.) | develop stalls at rms[Rho] -2, meanFlowFixes reaches -11 |
| `1c_rae2822` | under-relaxation | `case.cfg` (up to 20 000 it.) | same CL/CD, -11 in fewer iterations with meanFlowFixes |
| `2_hllc_translating` | HLLC on moving grids | `moving.cfg` (still air + translating grid), `fixed.cfg` (reference) | develop does not converge and gives a CL 12 % lower; meanFlowFixes matches the fixed grid |
| `3_inc_pitching` | incompressible moving-grid Jacobian | `case.cfg` (30 time steps, 60 inner it.) | better inner convergence, CL follows the motion |
| `4a_sources` | source terms in one slot | `body50_gravity.cfg`, `body100.cfg`, `body59.81.cfg` (30 it.) | develop: `body50_gravity` gives the same history as `body100`; meanFlowFixes: `body50_gravity` stops with a configuration error |
| `4b_turbo_riemann` | `BC_TurboRiemann` | `case.cfg` (restart from `solution_flow.dat`, 2 000 it.) | develop: segmentation fault; meanFlowFixes runs |
| `4c_mglevel` | `MGLEVEL` above 10 | `case.cfg` (3 it.) | develop: `stack smashing detected`; meanFlowFixes warns and uses 10 levels |

## How to run

```bash
DEVELOP=/path/to/develop/SU2_CFD BRANCH=/path/to/meanFlowFixes/SU2_CFD ./run_cases.sh            # all cases
DEVELOP=... BRANCH=... NP=4 ./run_cases.sh 1c_rae2822 2_hllc_translating                          # some cases
python3 plot_cases.py                                                                              # PNGs of 1a-1c, 2, 3
```

Each config `<case>/<name>.cfg` runs in `<case>/develop/<name>/` and `<case>/meanFlowFixes/<name>/` (history,
`log.txt`). The fixed-grid reference `2_hllc_translating/fixed.cfg` runs with develop only.

## Builds used for the comments

- develop: `edbda6eb54`.
- meanFlowFixes: the PR branch (the code has not changed since the plots, only the regression reference values).
- MPI ranks: 2, except the develop runs of `1b`, `1c`, `2` and `3`, which used 4. With `NP=2` these develop histories
  differ slightly from the plots (the linear solver depends on the partitioning), but the behaviour is the same.
