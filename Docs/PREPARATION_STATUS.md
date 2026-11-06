# Preparation status

The avoidable 1C arithmetic mismatch is fixed. The expanded tiny solver suite
passes 39 checks and 22 comparisons at the original tolerances. The new SIMD
regression fails with the old calculation and passes with the fix. No production
or cluster run has been performed.

| Work | Actual status |
|---|---|
| Nearly uniform 1C mismatch | Fixed and regression-tested; shared SIMD gradient cancellation |
| Local parallel and repartitioned restart | 20 jobs prepared and input-checked; execution blocked by local socket restrictions |
| Donor representation | Two short native donors correctly rejected by the physical boundary contract; unrestricted offline nodal fits accurate |
| Iterative physical-field donor recovery | Not run: needs a nonzero compatible donor or separately verified boundary-stress support |
| First physical study | Design prepared with real nominal references, case groups, candidate fields and numerical gates |
| Runnable physical campaign | Not ready: experimental operating details, source uncertainty, meshes and calibrated parameters remain prerequisites |

## Prepared material

- [Physical study design](validation_study/README.md): attached flat-plate flow,
  Wieghardt calibration and two Österlund validation cases; 58 unexecuted definitions.
- [Donor checks and recovery sequence](stress_checks/recovery_preparation.md):
  exact evidence of the boundary obstruction, without modifying the donor.
- [Local MPI plan](stress_checks/parallel_checks.json): 81 nodes, one/two localhost
  ranks, at most four iterations and a 45-second timeout per job; ten comparisons.
  Regenerate its full input directories using the command below.
- [Verification report](CAMPAIGN_REPORT.md): tests, fixed bugs and current limits.

```bash
python3 SU2_PY/stress_field/prepare_parallel_checks.py --output /tmp/stress-parallel-inputs --solver /path/to/MPI/SU2_CFD --config-probe /tmp/stress-checks/stress_config_probe --field-probe /tmp/stress-checks/stress_field_probe
python3 Docs/validation_study/check_preparation.py
```

Both commands prepare or inspect inputs only. The MPI plan requires an actual
MPI-enabled build and working localhost launcher; the current environment cannot
supply execution evidence. Do not substitute a serial binary under mpiexec.

Before physical runs, resolve the source experimental conditions and uncertainty,
create matched domains and meshes, and complete the declared numerical gates.
Calibration selects parameters using only its reserved cases. Freeze that selection
before evaluating held-out responses. No calibrated lock or solution values were
invented to label the preparation complete. Current scenario ranges are not
validated error bars.
