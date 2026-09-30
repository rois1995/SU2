# Cases of su2code/SU2#2939 (pressure-based solver, variable density)

Builds: develop `a6b7496b74` and branch [`rois1995/SU2:fix_pb_variable_density`](https://github.com/rois1995/SU2/tree/fix_pb_variable_density) (`e46fa88b4a`).

```bash
DEVELOP=/path/to/develop/SU2_CFD BRANCH=/path/to/branch/SU2_CFD ./run_cases.sh   # NP=2 MPI ranks by default
python3 plot_issue.py                                                            # pb_poly_cylinder.png, pb_bend_inlet.png
```

| Folder / config | What it is | develop | branch |
|---|---|---|---|
| `poly_cylinder/pb.cfg` | `pb_poly_cylinder.cfg`, 2000 it. | not converged (rms[h] -0.6), CD 3.95, heat flux -1828 | converged in ~90 it., CD 1.777, heat flux -171.9 |
| `poly_cylinder/db.cfg` | density-based `poly_cylinder.cfg` | CD 1.914, heat flux -175.0 | |
| `poly_cylinder/pb_const.cfg`, `db_const.cfg` | same cases with constant density | DB: CD 1.646 | PB: CD 1.576 |
| `bend_inlet/pb.cfg` | `pb_lam_bend.cfg` with variable density, energy, inlet at 400 K | inlet stays at 288.15 K | inlet 400 K, outlet 399.5 K |
| `bend_inlet/pb_pout.cfg` | `pb_lam_bend.cfg` with `MARKER_OUTLET= ( OUTLET, 100.0 )` | outlet pressure 0 | outlet pressure 0 |

Constant-density regression cases run to convergence with the same settings (rho = 1 inside the solver, so only the
strong-BC fix acts):

| Case | develop | branch |
|---|---|---|
| `incomp_pb_cylinder.cfg` | CD 2.8016 | CD 2.8932 (density-based, FDS without MUSCL: 3.2001) |
| `incomp_pb_NACA0012.cfg` | same history as the branch | |
| `pb_lam_bend.cfg` | converged (-17) | converged (-17) |
| `pb_rough_flatplate_incomp.cfg` | diverges after ~250 it. | same |
