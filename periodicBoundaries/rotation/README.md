# Rotational periodic boundaries: implicit solver, multigrid and limiters (branch `fix_periodic_rotation`)

"develop" is `6db10127d1`, "pr" is the branch. Each run folder holds `run.cfg` and `history.csv`. 2 MPI ranks.

To rerun a case, for example the sector without limiter:

```bash
cd periodic2d/pr_no_limiter_cfl100
ln -s ../sector.su2 ../inlet.dat .
mpirun -n 2 /path/to/SU2_CFD_pr run.cfg        # /path/to/SU2_CFD_develop for the develop_* folders
```

The histories of `periodic2d` keep only the iteration and residual columns. `python3 stats.py` reads the `restart.csv` of the runs and writes the statistics files listed below; `python3 plot_periodic.py` redraws the two figures from the histories and the statistics files.

| Folder | Case | Runs |
|---|---|---|
| `periodic2d` | `navierstokes/periodic2D/config.cfg`: laminar swirling flow in a 45 degree sector, ROE, implicit, from the free stream | no limiter at CFL 100; no limiter at CFL 20 with `MGLEVEL= 0, 1, 2` (`MG_MIN_MESHSIZE= 20`, with the default the mesh is too small for multigrid); the original case (`VENKATAKRISHNAN_WANG`, CFL 100). Develop and pr. |
| `limiter_annulus` | The same field on the full annulus (8 copies of the sector, no periodic markers, `replicate.py`) and on the 45 and 90 degree sectors, one iteration with `VENKATAKRISHNAN`. The fields are made from `field_sector.csv` with `python3 mkrestart.py field_sector.csv <mesh>.su2.map <run>/solution.csv`. | `annulus_develop` (the reference), `develop_sector45`, `pr_sector45`, `develop_sector90`, `pr_sector90`. `limiter_boundary_45.csv`: velocity limiters on the periodic points; `sector_vs_annulus.txt`: differences to the annulus. |
| `limiter_3d` | `turbomachinery/multi_interface/multi_interface_rst.cfg` (three zones, 90 degree sector, mesh and restart from the TestCases repository) with `MUSCL_FLOW= YES`, 5 iterations | `limiter_stats_develop.txt`, `limiter_stats_pr.txt`: limiters on the points of the periodic markers of each zone (`python3 stats3d.py <mesh> <zone> restart_flow_<zone-1>.csv`). `run.cfg` is the master configuration, the zone files are unchanged. |

Figures: `rotational_convergence.png` (the runs of `periodic2d`), `rotational_limiter.png` (`limiter_annulus/limiter_boundary_45.csv`).

The mesh `sector.su2` and `inlet.dat` are copies from the SU2 repository.

## Standalone branch checks, 2026-10-06

Final source: `fix_periodic_rotation` at `c39428c1da`, based on `6db10127d1`. Fresh release build with OpenMP, MPI, mixed precision and warning level 3. No new warnings from the periodic changes; repository pre-commit checks pass.

`validation_20261006.json` records the three MPI periodic2D reference comparisons (two ranks, one thread each) and the three existing hybrid cases (one rank, two threads). The new MPI comparisons are within the configured `1e-5` tolerance. Hybrid references were refreshed from these runs; CI uses different CPU flags, and aarch64 values still need CI verification.

The plots and longer convergence/limiter comparisons were produced with the earlier version of the same three rotational fixes. Final changes preserve identity-rotation arithmetic for translations and apply review style; the rotational calculations are unchanged. The translational AD check now agrees with all 80 printed develop iterations. Rotational adjoint smoke runs are not a complete sensitivity validation; default solver settings also fail to converge on develop.

Earlier combined-branch standard units passed 44 cases / 74913 assertions and AD units passed 4 cases / 29 assertions. The standalone OpenMP full unit driver aborts on the data-driven-fluid test when optional MLPCpp support is disabled; it is not counted as a passing suite.

## Aachen turbine MG follow-up

The completed 10,000-iteration develop/PR comparison and MG1/2/3 study, configs, input/restart bundle, raw histories and reproduction scripts are in [aachen-mg](aachen-mg/README.md). Remaining wall-function and convergence limits are documented there.
