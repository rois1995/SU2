# Points on the rotation axis of rotational periodic boundaries (branch `fix_periodic_axis`)

"develop" is `6db10127d1`, "pr" is the branch. Each run folder holds `run.cfg` and `history.csv`. 2 MPI ranks.

To rerun a case, for example the converged 30 degree sector:

```bash
cd pipe_axis/pr_w30_converged
ln -s ../pipe_w30.su2 .
mpirun -n 2 /path/to/SU2_CFD_pr run.cfg        # /path/to/SU2_CFD_develop for the develop_* and full_* folders
```

The histories of the converged runs keep only the iteration and residual columns. `python3 stats.py` reads the `restart.csv` of the runs and writes the statistics files listed below; `python3 plot_periodic.py` redraws the figure from `pipe_axis/axis_profile.csv`.

| Folder | Case | Runs |
|---|---|---|
| `pipe_axis` | Laminar pipe flow (radius 1, length 2), meshes from `pipe.py`: 30 degree sector with one cell around the axis (`pipe_w30.su2`, the mesh of the new test `periodic3d_axis`), 90 degree sector with three cells (`pipe_s90.su2`), full pipe without periodic markers (`pipe_full.su2`). The axis nodes are on both periodic markers. | One iteration from the same analytic field (`python3 pipefield.py pipe_<mesh>.su2 <N> <run>/solution.csv`, N = 12 for `w30` and `field12`, 4 for `s90` and `field4`): a central scheme without dissipation (`central`), ROE with MUSCL and `VENKATAKRISHNAN` (`limiter`), the same with least-squares gradients (`least_squares`). `vs_full_pipe.txt` in each sector run: differences to the full pipe at the same nodes (`pipecmp.py`). Converged runs (`*_converged`, first-order ROE): `axis_profile.csv`, `axis_summary.txt`. |

Figure: `axis_nodes.png` (`pipe_axis/axis_profile.csv`).
