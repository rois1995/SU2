# Restored query schedule and frozen-M6 preparation audit

Branch codex/native-3d-bl; previous published checkpoint83fb0678232d191a90e52c3271a7f4cead0775bc.
Full Goal3 remains ACTIVE/incomplete. No production cache optimization is retained.

The shared query schedule, imports and acceptance are restored byte-for-byte to
that checkpoint except for field_builds++ after successful FrozenField construction.
BridgeStats and per-rank bridge profiles expose that count. A test-only preparation
path retains exact comparison against fresh imports for future connected changes.
It loads the archived owner-UPPER6 backend field, imports the actual M6 CFD geometry,
selects512 original cells by canonical node keys independently of MPI partitioning,
and checks all actual query coordinates, sensor tensors, composed BL tensors and
integer partition weights. This is not replacement/refitting of the hosting-rank
RAW9 integration archive. No CFD step or M6 mesh construction is performed.

## Rejected experiments

See rejected_comparison.json and [detailed report](../../../../Handbook/GOAL3_POINT_PREPARATION_REUSE.md).
Both variants preserve the field/correctness checks but fail performance:

| Strategy/ranks | Preparation | Fresh reference | Imports | Index builds |
|---|---:|---:|---:|---:|
| Whole AABB support /1 | .1561s | .0855s | 189 /125 | 122 /124 |
| Whole AABB support /2 | .1892s | .1123s | 189 /125 | 122 /124 |
| Whole AABB support /4 | .1829s | .0803s | 146 /95 | 123 /123 |
| Exact point support /1 | .5566s | .1538s | 189 /125 | 122 /124 |

No useful index reuse; cap resets cause additional imports. Exact contact filtering
does not cure this. Times are contended selected-cell observations; fresh reference
omits production admission bookkeeping. No stable full-event speedup/slowdown claim.
The original source, geometry, height, quality, positivity, fade, finer demands,
full composed edge and transfer constraints remain unchanged. Both executed source
variants are preserved as verified gzip patches against83fb plus source hashes,
input provenance, outputs and resource receipts in their sibling directories.
The invalid1-rank/four-worker fixture and test compile typo are preserved too.

## Current restored checks

- Existing bridge serial/MPI4 checks: current+BDF barycentric/conservative transfer,
  actual composition on sensor hits, returned metrics, memory refusal and failure
  preservation. No speculative cache pressure collective remains in production.
- Same512-cell M6 replay on4 ranks: all2560 query coordinate/sensor/composed answers
  and weights exactly match fresh imports. Weights also equal the preserved serial
  cold reference for all512 canonical cells. All physical inputs identical; only
  replay adaptation worker count matches its CFD communicator.
- Actual initialized RANS/SA BL construction/removal N4/M2: independent accepted
  meshes24->6 cells, exact volume1/no overlap, first-height error0, q minima0.70846/
  0.68723, J minima0.45142/0.40825, maximum full-target edge1.71991/1.73205.
  Conservative current+BDF integral defects<=2.75e-16, transported sensor error0,
  independent SA/restart and geometry/target-equivalent SU2+CGNS reload all pass.
  These are scoped initialized cube workflows, not evolved viscous convergence or
  accepted M6 meshes. The composed audit still reports44 violations/max24.5184;
  sensor gradation is not a composed-field certificate.

The first rejected strategy separately passed the same512-cell M6 set1/2/4 ranks,
dense-star/reset/recovery/empty-rank controls and curved-extension runtime. The
second was stopped after its serial pilot; no unnecessary broad MPI campaign.
Current native cases and resumed checks are in campaign.log/resume.log; provenance
pins source/build/binary/config/inputs. Resume verifies and reuses complete cases.

## Measured next lead

An offline count from the saved coordinates finds454 distinct nodal coordinates
for2048 nodal requests. Adding at most512 distinct centroids gives at most966
unique queries out of2560, so at least1594 queries repeat. That upper bound fits
the existing2048 sample capacity. audit_point_opportunity.py, its input hashes and
point_reuse_opportunity.json reproduce the result without another solver run.
This establishes a reuse opportunity, not its runtime benefit or a completed
stencil/sample cache. Whole donor-star retention was the wrong unit of reuse.

Inspect canonical original sample/stencil lookup and the existing regional point
cache before adding another cache. Keep immutable-source/geometry identity explicit;
BL composition still runs at actual queries. Wider shared-edge lifetime and
independent first-row transactions remain the separate requested package. The saved
M6 first-row/edge work, original-field reassessment/correction epochs, global error
calibration, difficult accepted construction and total lifecycle affordability remain.

## Reproduce or reuse

From the repository root, after building UnitTests/test_driver:

```sh
OMP_NUM_THREADS=1 OPENBLAS_NUM_THREADS=1 MKL_NUM_THREADS=1 nice -n19 python3 integration_evidence/native_3d_bl_v1/oneram6_uncertainty_v1/point_preparation_baseline_v1/campaign.py
OMP_NUM_THREADS=1 OPENBLAS_NUM_THREADS=1 MKL_NUM_THREADS=1 nice -n19 python3 integration_evidence/native_3d_bl_v1/oneram6_uncertainty_v1/point_preparation_baseline_v1/audit_point_opportunity.py
```

Changed native source/binary/config pins require a NEW sibling case folder; copy
the campaign there rather than overwriting its completed results. Links close inside
the checkout and target versioned inputs. The preserved serial reference and source
patches remain available for independent verification.

One owned heavy job/build-j1/nice19/thread1/MPI<=4;38–40GiB host RAM available.
Restored build6m20s/peak process2.39GiB; connected wrapper52.77s/peak process529.59MiB,
GNUtime0 swaps. These are process peaks, not aggregate MPI job RAM. Host swap803->
812MiB has no established attribution. Other sessions' CFD and ClusterResults were
untouched. All owned job handles are terminal. No cluster submission or new CFD solve.
