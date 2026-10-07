# Streamwise periodicity (branch `fix_streamwise_periodic`)

"develop" is `6db10127d1`, "pr" is the branch. Each run folder holds `run.cfg` and `history.csv`. 2 MPI ranks.

To rerun a case:

```bash
cd streamwise_pipe_slice/pr_heat_integrated
ln -s ../pipe1cell3D.su2 .
mpirun -n 2 /path/to/SU2_CFD_pr run.cfg        # /path/to/SU2_CFD_develop for the develop_* folders
```

| Folder | Case | Runs |
|---|---|---|
| `streamwise_pipe_slice` | `incomp_navierstokes/streamwise_periodic/pipeSlice_3d` with the energy equation and `STREAMWISE_PERIODIC_TEMPERATURE= YES`, 3 iterations (the configuration of the new test `sp_pipeSlice_3d_dp_ihf_tp`) | wall heat as 1000 W/m2 (develop, the reference); the same heat as integrated value with `INTEGRATED_HEATFLUX= YES` (develop and pr); `MGLEVEL= 1` (develop crashes, pr stops with an error: `error.txt`); energy equation off (the `SWHeat` column). `other_checks.txt`: the turbulent source term, the recovered pressure and `SWHeat`, from runs that are not in this folder. |

The mesh `pipe1cell3D.su2` is a copy from the TestCases repository.
