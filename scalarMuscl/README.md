# Test cases of the turbulence/species MUSCL fixes (branch `fix_scalar_muscl`)

"develop" is `a6b7496b74`, the base of the PR; "pr" is the PR branch. Each run folder holds the exact configuration (`run.cfg`) and its `history.csv`. The meshes and the SA restart are in the case folders.

To rerun a case, for example the SST NACA0012 with the Wang limiter:

```bash
cd sst_naca/pr_muscl_wang
ln -s ../n0012_225-65.su2 .
mpirun -n 2 /path/to/SU2_CFD_pr run.cfg        # /path/to/SU2_CFD_develop for the develop_* folders
```

`python3 plot_muscl.py` redraws `muscl_fixes.png` from the histories.

| Folder | Case | Runs |
|---|---|---|
| `sst_naca` | SST NACA0012, Mach 0.15, 10 deg, Re 6e6 (`rans/naca0012/turb_NACA0012_sst.cfg`), from the free stream, `MUSCL_TURB= YES` | first order; MUSCL with `VENKATAKRISHNAN_WANG` and with no limiter, develop and pr. `develop_muscl_none` was stopped at 6919 iterations. |
| `sa_naca` | SA NACA0012, 10 deg (`rans/naca0012/turb_NACA0012_sa.cfg`), restart from `solution_flow_sa.dat`, `SLOPE_LIMITER_FLOW= NONE` | no limiter; Wang always computed; Wang with `LIMITER_ITER= 50` (develop and pr); flow `VAN_ALBADA_EDGE` with turbulence Wang (develop and pr) |
| `bounded_sa` | incompressible SA flat plate (`incomp_rans/rough_flatplate/rough_flatplate_incomp.cfg`, no roughness) with the density of water (998.2) and the viscosity scaled to keep the Reynolds number | `SCALAR_UPWIND` with rho 998.2; `BOUNDED_SCALAR` with rho 1 (`INC_NONDIM= INITIAL_VALUES`) and rho 998.2 (`DIMENSIONAL`), develop and pr |
| `sst_flatplate` | SST flat plate, TMR 137x97 grid (`rans/flatplate/turb_SST_flatplate.cfg`), turbulence intensity 0.0003873, viscosity ratio 0.009 | first order; MUSCL with no limiter, `VAN_ALBADA_EDGE` (pr only: develop stops) and `VENKATAKRISHNAN`; timing runs with first-order turbulence and `NUM_METHOD_GRAD_RECON= LEAST_SQUARES` (1 rank, run at the same time) |
| `sliding_bars` | `sliding_interface/bars_SST_2D` with `CONV_NUM_METHOD_TURB= BOUNDED_SCALAR` and `KIND_INTERPOLATION= WEIGHTED_AVERAGE`, 1500 iterations, 1 rank | bounded with the correction not weighted (the PR without its last fix), bounded (pr), `SCALAR_UPWIND` (pr). Run `SU2_CFD bars.cfg` in each folder after linking the two meshes. |
