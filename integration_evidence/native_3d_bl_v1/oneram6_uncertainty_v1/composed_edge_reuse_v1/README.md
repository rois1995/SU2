# Frozen composed-edge reuse — validated connected batch

Base dca448703a982234e8ea923b0b13676ff28e2fdb, branch codex/native-3d-bl.
Full Goal3 ACTIVE/incomplete. One owned heavy job, nice19/build-j1/thread1/MPI<=4.

## Missing capability and design

The existing selection, retired-edge and streamed-private maps already retain
source-resolved edge lengths. MeasureMetricCells explicitly disabled all scalar
reuse whenever the target had geometric BL composition, even for unchanged edges.
The first-row background alternatives also remeasured their common edges. Saved M6
construction reports geometry preparation147.86s on one rank; it is a hotspot,
not proof that every query repeats or that caching alone closes construction.

Extend those existing maps, without another overlapping field/edge cache. Each
FrozenField gets a distinct shared immutable identity by default. One native engine
Adapt invocation explicitly shares an identity across its certified original-donor
imports under unchanged sensor/composition/core/geometry/fade/breakpoint/accuracy
policy. A new invocation or normalization stage creates a new identity. EdgeMeasure
retains the identity: its lifetime prevents a later allocation at the same address
from becoming a false cache hit. Reuse also requires matching edge IDs, exact endpoint
coordinates, and a finite positive previously successful full-target integral.
No node-ID-only, coordinate-tolerance, epoch-number or sensor-substitute shortcut.

A composed field must still declare geometry-resolved integration even if all
lengths can be reused. Every actual centroid/point query still composes BL; only
an already integrated scalar edge is reused. Changed/moved/new edges use the same
source tracing and strict full composed integration as before. First-row candidates
retain only the immediately preceding candidate's bounded edge map. Separate first-
row evaluation/hit counters preserve existing selection-counter scope. Memory models
using sizeof(EdgeMeasure) include the additional identity handles; padded private
scratch and bounded patch counts remain. No new config option or dependency.

## Required evidence

- Exact same/cross-import composed reuse; different target identities force fresh
  evaluation, even for sensor-only fields. Exact moved endpoints reevaluate incident
  edges. Nonfinite cached values reevaluate. Unsupported geometry policy and missing
  donor coverage still reject. Actual centroid composition remains active.
- Saved original M6 owner-UPPER6 sensor, original faceted geometry/core/fade,32
  canonical configured-wall tetrahedra on1/4 ranks: cold, shared-edge pass and
  repeated pass. All q/J/max/full edge lengths must match exactly; compare MPI by
  canonical cell and edge IDs. No target normalization or CFD solve.
- Affected first-row/disjoint engine controls, actual curved return and initialized
  RANS/SA N4/M2 construction/removal/conservative current+BDF/SU2/CGNS/restart with
  the existing independent geometry/full metric/state audit. Unchanged mesh gates.
- Retain complete source/build/binary/input/helper pins and bounded resource receipts;
  no broad regression per tiny edit and no unchanged rejected M6 construction rerun.

## Scope and remaining goal

No accepted M6 mesh, full-event speedup, global uncertainty calibration, finer-mesh
original-field reassessment or correction stage follows from a wall-cell replay.
Source-support reuse across arbitrary reconstruction campaigns and independent
transaction publication remain requested work. Existing regional service edge memo
is unchanged, rather than overlapped by a second cache. No2D behavior is changed in
this batch; inspect its existing reuse before extending a successful common policy.

## Achieved evidence

73 serial cases/6234 assertions PASS. New identity, cross-import/full-target exactness,
changed sensor stage, moved coordinates, malformed entries, missing bands and missing
new-edge coverage are covered. Existing first-row engine controls preserve height,
input state and exact mesh embedding; MPI4 includes disjoint first-row owners and
coupled construction/removal. This batch does not save a separate production first-
row hit/speedup comparison, and the complete M6 construction remains unqualified.

Same32 original M6 wall cells PASS exact cold/shared/repeated q/J/max/all edge lengths
on MPI1/4; per-cell/per-edge CSV comparisons also agree exactly across ranks.

| Ranks | Cold / shared / repeated time | Fresh integrals | Reused integrals | Actual compositions |
|---|---:|---:|---:|---:|
| 1 | .61096 / .44427 / .01958s | 192 /137 /0 | 0 /55 /192 | 62897 /48182 /32 |
| 4 | .87581 / .55143 / .03175s | 192 /137 /0 | 0 /55 /192 | 62897 /48182 /32 |

Times are maximum rank, counts are sums.55 shared edges avoid duplicate integration;
137 distinct edges remain in the first shared pass. The serial elapsed reduction
is27.3%, MPI37.0%, and actual compositions fall23.4%. The identical repeated pass
measures the available reuse of already known segments, not an expected whole-
adaptation gain. Its32 compositions are the actual centroids, not cached BL tensors.
Rank0 requests all32 selected cells; other ranks take matching collective service
calls. These checks establish consistency and empty-requester behavior, NOT scaling
or multi-requester load balance. Donor imports/index construction still run in all
three passes. No old-binary or complete M6 adaptation benchmark is implied.

Affected curved return and initialized RANS/SA N4/M2 construction/removal PASS the
existing independent audit:24->6 accepted cells, exact volume1, h0 error0,
q minima.70846/.68723, J minima.45142/.40825, max composed edge1.71991/1.73205,
transported sensor error0, flow+BDF defects<=2.75e-16, real SA restart and SU2+CGNS
geometry/target reload. This is the initialized cube workflow, not evolved viscous
accuracy or accepted M6 wing construction.44 composed-gradation violations remain
reported; no combined-field gradation correction/certificate is introduced.

One heavy job at a time, nice19/build-j1/thread1/MPI<=4. Host38–40GiB available.
Initial connected rebuild11m25s plus one engine safety refresh3m04s (identity allocation
is admitted and collectively caught); Ninja then reports no work. Peak process
2.375GiB. Serial pilot49.69s/1.628GiB; remaining resumed wrapper69.57s/529.83MiB.
These are process peaks, NOT aggregate MPI memory. GNUtime reports0 swaps; existing
host swap has no established attribution. All owned handles are terminal. Compile scratch stayed on4TB;
foreign jobs, AdapNoExt, dependencies and foreign ClusterResults were untouched.

## Reproduce

From repository root after building both current binaries:

```sh
OMP_NUM_THREADS=1 OPENBLAS_NUM_THREADS=1 MKL_NUM_THREADS=1 nice -n19 python3 integration_evidence/native_3d_bl_v1/oneram6_uncertainty_v1/composed_edge_reuse_v1/campaign.py --pilot
OMP_NUM_THREADS=1 OPENBLAS_NUM_THREADS=1 MKL_NUM_THREADS=1 nice -n19 python3 integration_evidence/native_3d_bl_v1/oneram6_uncertainty_v1/composed_edge_reuse_v1/campaign.py
```

The second command verifies/reuses the successful serial results rather than
rerunning them. Each case pins native sources/build/binary/config/inputs; summary
also pins the acceptance helper. Changed native pins require a new sibling folder.
Links close within tracked repository inputs. Raw profiles, logs, exact edge CSVs,
comparison.json and independent_mpi4_m2.json retain the demonstrated scope.
No CFD solve, unchanged300s M6 construction rerun or cluster job was launched.
