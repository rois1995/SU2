# Periodic points: heat solver volume, halo neighbors and MSW sensor (branch `fix_periodic_counts`)

"develop" is `6db10127d1`, "pr" is the branch. Each run folder holds `run.cfg` and `history.csv`. 2 MPI ranks unless the folder name says otherwise.

To rerun a case, for example the pin array with the heat equation:

```bash
cd pin_array_heat/pr_heat_yes
ln -s ../fluid.su2 .
mpirun -n 2 /path/to/SU2_CFD_pr run.cfg        # /path/to/SU2_CFD_develop for the develop_* folders
```

The histories of `periodic2d_jst` keep only the iteration and residual columns. `python3 stats.py` reads the `restart.csv` of the runs and writes the statistics files listed below; `python3 plot_periodic.py` redraws the figure from the histories.

| Folder | Case | Runs |
|---|---|---|
| `pin_array_heat` | 2D pin array (`fluid.su2` of `incomp_navierstokes/streamwise_periodic/chtPinArray_2d`), periodic flow driven by a body force, the configuration of the new test `inc_periodic_weak_heat` | `WEAKLY_COUPLED_HEAT_EQUATION= NO` and `YES`, develop and pr. `flow_difference.txt`: flow with the heat equation minus flow without. |
| `periodic2d_jst` | `navierstokes/periodic2D/config.cfg` (45 degree rotational periodic sector) with JST at CFL 20, converged | with 1 and with 2 ranks, develop and pr. `jst_rank_difference.txt`: largest difference between the two runs. |
| `msw_sensor` | `mms/fvm_euler` mesh `TriAdapt.su2` (triangles, two periodic pairs), `MSW`, one iteration from `solution.csv`: the vortex field of the case scaled to a pressure of 1e5 Pa, with the pressure doubled at one neighbor of a periodic point | develop and pr |

Figure: `weak_heat.png` (the runs of `pin_array_heat`).

The meshes `fluid.su2`, `sector.su2` and `TriAdapt.su2` are copies from the SU2 and TestCases repositories.
