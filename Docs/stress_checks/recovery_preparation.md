# Donor recovery: preparation and observed boundary obstruction

The two tiny native donors cannot be recovered through the present physical-field
interface without changing the experiment. Their corrections are nonzero at
physical boundaries where that interface requires exact zero.

## What was checked

`SU2_PY/stress_field/run_recovery_checks.py` generates the existing 81-node square,
runs SA-QCR and SST matched EPM for four iterations each, and exports their nodal
corrections. These short donors are not converged flow solutions. It then performs
two separate checks:

1. Fit the complete donor correction using the declared planar wall map and box
   support. Both targets are rejected. The maximum nodal mismatch equals the
   maximum donor magnitude: relative error 1.0.
2. Fit the same values offline without a wall/support map. Relative nodal errors
   are approximately 2.1e-16 for QCR and 1.3e-15 for EPM. Both resulting fields
   are deliberately submitted to solver startup, which rejects their nonzero
   boundary stresses before iteration.

The largest six-component stress norms are approximately 7.81e-4 for QCR at an
outlet node and 6.12e-4 for EPM at an outlet/top node, in internal units. This is
not a small roundoff mismatch. QCR also has wall values near 1.9e-57, but the much
larger inlet/outlet differences independently establish the obstruction.

The unrestricted artifacts are labelled OFFLINE_ONLY in their donor IDs and
are never used for a time update. No donor values were removed, clipped or tapered.
The field reader's exact-zero boundary guard remains in force.

## What this establishes

The observed failure is a mismatch between the native donor and the admissible
field family. Accurate nodal interpolation alone cannot overcome it. It says
nothing about convergence of a compatible receiver. Same-state exact diagnostic
replay remains a separate successful insertion check.

No field-based iterative recovery was run, because these targets fail the
representation gate. The finite iteration donors must not be described as
reference solutions or evidence of physical accuracy.

## Prepared next experiment

Before any recovery solve, select a nonzero donor whose correction satisfies the
physical boundary contract. An alternative would require a separately scoped
implementation of prescribed boundary tractions and energy work, with matching
tests. An accurate interior fit does not justify extending that contract.

For a compatible donor, the remaining sequence is:

1. Verify convergence of a tiny donor within a declared iteration/time limit;
   retain an explicit not-converged result if that limit is reached.
2. Capture exact face/source corrections and confirm same-state residual recovery.
3. Fit the physical field without altering the donor, then measure the residual
   difference against exact replay. Repeat field resolution before interpreting
   any nonlinear difference as a coupling bug.
4. Start the receiver from the saved donor and from a separately declared nearby
   state. Keep the final field fixed and compare the complete solution and selected
   outputs with fixed reference scales, recording failures and convergence status.

Steps requiring a compatible physical donor are prepared here, not represented as
executed. No production cases, MPI launcher or cluster are used by this check.

Reproduce with a newly created output directory:

```bash
python3 SU2_PY/stress_field/run_recovery_checks.py --solver /path/to/SU2_CFD --probe /path/to/stress_field_probe --output /tmp/stress-recovery-checks
python3 SU2_PY/stress_field/test_recovery.py
```

The runner allows only its fixed 81-node fixture, four donor iterations and
one-iteration configurations that must reject at startup, each with a 45-second
timeout. Inputs, output hashes, commands and timings are saved in the report.
The retained [check report](recovery_checks.json) and [helper test log](recovery_unit.log)
record the actual outcomes. The report identifies the executable used for the checks.
