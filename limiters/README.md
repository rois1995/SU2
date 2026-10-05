# Test cases of the slope limiter fixes (branch `fix_limiters`)

"develop" is `6db10127d1`, the base of the PR; "pr" is the PR branch. Each run folder holds the exact configuration (`run.cfg`) and its history file. The meshes (and the restart of the ramp) are in the case folders. All runs used 2 MPI ranks.

To rerun a case, for example the laminar flat plate with the 2 mm band:

```bash
cd wall_distance_laminar/pr_band_2mm
ln -s ../mesh_flatplate_65x65.su2 .
mpirun -n 2 /path/to/SU2_CFD_pr run.cfg        # /path/to/SU2_CFD_develop for the develop_* folders
python3 ../../limstats.py restart_wd2.csv      # minimum, maximum, mean and number of zeros of every limiter field
```

`limiter_stats.txt` in a run folder is the output of `limstats.py` for that run.

| Folder | Case | Runs |
|---|---|---|
| `ramp_r4` | `euler/ramp/inv_ramp.cfg` with `SLOPE_LIMITER_FLOW= NISHIKAWA_R4`, 10 iterations from `restart_flow.dat` (the new test `ramp_r4`) | develop and pr |
| `wall_distance_laminar` | `navierstokes/flatplate/lam_flatplate.cfg` with `SLOPE_LIMITER_FLOW= WALL_DISTANCE`, 50 iterations | band of 0.3 m (default `ADJ_SHARP_LIMITER_COEFF`) and of 2 mm (`ADJ_SHARP_LIMITER_COEFF= 0.02`), develop and pr |
| `zero_width` | `rans/flatplate/turb_SA_flatplate.cfg` with `SLOPE_LIMITER_FLOW= WALL_DISTANCE`, 20 iterations | `VENKAT_LIMITER_COEFF= 0.05`, `VENKAT_LIMITER_COEFF= 0` and `ADJ_SHARP_LIMITER_COEFF= 0`, develop and pr. With the pr the last two stop with the message in `error.txt`. |
| `nemo_outputs` | `nonequilibrium/invwedge/invwedge_roe.cfg` with `SLOPE_LIMITER_FLOW= VENKATAKRISHNAN` and the `LIMITER` volume output, 10 iterations | develop and pr. Compare the two `limiter_stats.txt`: the values are the same, the names change. |

The fields `Limiter_Density` and `Limiter_Enthalpy` of the compressible solver are zero in all runs, on develop and with the pr. This is an output problem only and this PR does not change it.
