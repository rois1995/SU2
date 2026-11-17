# N14 validation helper readiness

Scope is G1 and G2/GQ. The helper owner did not change production code, existing pinned helpers, or policy, and did not launch a build or numerical run. Build552 and focused units were completed by the primary: build exit 0 in 120.173935 s with 1,775 pins; focused tests passed 12 cases / 113 assertions in 4.862019 s.

## G1

`run_g1_552.py` uses the sealed fresh536 reports from `N4/section6_3_after_N11/reference_validation`, with serial (375), MPI2 (114), and MPI4 (228) exact selections. Identity is strict except for the six individually named wall-stage cases from the B1/B2 gate. The generated summary captures each changed case and its old/new assertion values; successful assertion totals are observation fields only. Each cohort readiness receipt pins the build552 artifacts and helper/reference inputs.

The next command is `python3 /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated/integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/N14/section6_12/validation/run_g1_552.py --cohort serial`, after the primary records `G1_serial_host_preflight.json`. `--self-check` passed without launching.

## G2/GQ

`run_g2_gq_prefix_552.py` reuses the existing N4 prefix controller. `validate_g2_gq_552.py` compares against fresh536 `G2_reference536` / `GQ_reference536` using the existing 6.8/6.9 census comparator. All old integer rows stay exact; the only timer-call adjustment is the rank/boundary-local delta from `NATIVE3D_WALL_LAW` lines. The timer summary storage value remains 4120. G2 also checks the exact first-collapse work/count anchors and zero upper refinements. GQ checks the third quality-window row on every rank and the passing G2 prefix. Both wrapper self-checks passed; no candidate prefix was launched.

## R3 setup corrections

The inherited G1 reference verifier expected an older manifest state and report filenames. The helper now verifies the selected fresh536 manifest and case counts, then compares to its sealed reports. The first `REPO` depth calculation was one directory too shallow; it now uses `STAGED.parents[3]`. The inherited build label was 535; the private candidate loader now binds it to build552. Reference report aliases are validation-local symlinks to the immutable P files, and per-cohort readiness receipts record those targets.

Exact helper and input SHA256 values, self-check results, gate assumptions, and next commands are in `validation_helper_setup_receipt.json`. No G3/full/restart evidence is covered by these helpers.
