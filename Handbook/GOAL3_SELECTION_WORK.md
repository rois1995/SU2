# Goal 3 — useful MPI proposal work and local front interval closure

Full Goal3 remains active. Source before this connected change is c032d926d0.
Evidence folder: integration_evidence/native_3d_bl_v1/interacting_walls_v1.

## Audit before changing code

The work/round controls intentionally reject even valid fields when collapse
candidates remain. NativePolicy3D explicitly requires a zero-work refusal with
valid cached cells, and the anisotropic cycle requires round-limit refusal with
pending candidates. Do not redefine COMPLETE or promote the saved496-cell MPI4
state. Keep those refusal/atomicity tests and all geometry/height/q/J/full lengths.

The actual MPI4 removal admits4084 proposals but attempts1172;2912 lose reservations.
Many ranks hold the same edge because generated cell ownership is distributed.
Exact duplicates are safe to coalesce before budget charging and star payload
import; distinct directed collapses and overlapping distinct stars still require
the existing reservation/proof protocol. This is not proof every conflict is a
duplicate or that coalescing alone will close the event.

## Connected implementation

A bounded table of at most eight directed keys per worker elects the lowest
(rank,slot) for each identical collapse. Reversed directions remain distinct.
Both scalar and batched sensor-only planar coarsening use it; composed/curved
paths retain their existing serialization. No queue is blocked merely because
another rank submits the same proposal; the existing owner-routed rejection and
changed-star invalidation still decide retries. Candidate memory/transport is
admitted collectively, and unique proposals enter the unchanged work limit,
complete dependency reservation, metric proof and atomic publication. Added
proposal_duplicates counters distinguish saved speculative work from real edits.

The prismatic planner also closes every local normal interval after cap
refinement. Maxima of cumulative column integrals need not bound their differences;
front_interval_counterexample_v1.json proves it exactly with positive densities.
Keep initial balanced fronts, insert a shared metric-balanced front in any
violating free interval and close cap demands at new planes. All original columns
and the final original field are checked; prescribed rows stay fixed, planning/
cell/time admission remains finite and target tensors/fade/source are unchanged.
No local cavity fallback, normal reset or tolerance relaxation is introduced.

## Planned qualification

- native_prism_single_v1: regional h.1/g1.2/T.3 single-wall fade and finer scalar
  original-P1 sensor; old single-wall control did not enter the regional path.
- native_prism_crossing_v1: same wall with original-P1 diag(4,4,4+96*(1-|x-z|))
  on the six ordered cube tets. Different columns control normal demand.
- native_opposing_resume_mpi4_v1: reuse the independently certified1920-cell BL
  with original global IDs, version0 and id-modulo-rank ownership, remeasure it
  under the unchanged actual field and then test strict removal. This saves
  repeated whole-region construction and tests retained-BL idempotence.

NativePolicy3D remains included, covering zero work, elapsed refusal, successful
coarsening, inconsistent controls and duplicate accounting. Passing fixtures alone
do not establish actual backend/CFD/current-BDF/restart, tangential anisotropic
alignment, wider geometry or affordability. The connected build and the three planned native cases are terminal. Two serial
fields accept; MPI4 removal remains rejected. No speedup or full-goal claim.


## Executed results

`selection_work_v1.json` pins native receipts and independent complete-field audits.
The scalar/fade case accepts6->576->228 cells, work670/252; BL q.433066/J.184448/
full L1.664467. Crossing-normal accepts6->390->195, work673/183; BL q.365266/
J.117200/full L1.720318. Both independently pass complete exact finite geometry,
no overlap/every vertex link/volume1, original-P1 fields, every full numerical BL
edge and prescribed height/normal edges. Numerical error indicators are not rigorous
enclosures. The scalar collector expected3 instead of4 tests because NativePolicy3D
selects two cases. Its original FAIL receipt is preserved; explicit hash-verified
adjudication records native exit0/all4 cases passed without a native rerun. Collector
counts are corrected for subsequent runs. Executed independent helpers are retained.

Saved1920-cell retained BL revalidates MPI4 with no regional reconstruction/work.
Its full auditable geometry/source input is byte-identical to the certified serial
construction; explicit receipt/equality/callback checks reuse that proof. Coarsening
suppresses2399 exact duplicate directed proposals, yet2689 distinct-star reservation
conflicts remain.1402 attempts consume4091 admitted proposals; strict4096 work gate
still rejects. The319-cell private mesh independently has q.220002/J.070456/full
L1.783027 and exact geometry/links/no overlap/volume1. Keep native FAIL and existing
refusal/atomicity contracts. This is progress diagnosis, not a complete MPI removal
milestone or speedup proof. Earlier496-cell rejected input remains preserved.

The remaining cause is candidate coordination: workers select disjoint local stars,
while full remote stars overlap. The next design should schedule complete authoritative
star metadata before importing/reserving proposal payloads, record bounded selection
work, and retain final versioned reservations. Do not simply stop charging conflicts,
raise budgets, serialize every edit or introduce another collapse fallback. Actual
prismatic backend Refine-before-Coarsen/current-BDF/CFD/restart and arbitrary tangential
anisotropy remain separate full-goal gates.
