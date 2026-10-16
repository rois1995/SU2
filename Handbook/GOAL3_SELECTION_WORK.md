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


## Complete-star admission change under qualification

Directory.PlanBatch asks the existing authoritative lookup for the sorted vertex
union of every complete requested star, including remote cells. The same vertex-
claim election is shared with final ReserveBatch. It compacts selected dependencies
and returns original slots; missing/oversized stars and collective storage failures
remain failures. Deferred candidates remain queued. Both ordinary batched and
invalid-field one-slot sensor-only planar coarsening use it. Final fetched versions,
reservations, whole private mesh/metric validation and atomic publication are retained.
Curved/composed/repair coordination is unchanged.

The existing transaction-proposal/work limit remains4096; conflict filtering now
happens during metadata selection, like the earlier local disjointness filter, so
no payload is fetched for a deferred candidate. Metadata work is not free: considered
and deferred counts, total tracked transport and planning time nested in selection
are exported. Each nonempty planned round selects at least one proposal, so admitted
work bounds the number of rounds; each frontier has at most batch*workers candidates,
with the existing per-star and memory/time controls. No retry/budget ladder or silent
acceptance of an unfinished search. A remote-only shared-vertex conflict, two disjoint
stars, no payload before selection, stale final fetch, missing/memory refusal and
unchanged stores have focused controls. The complete saved MPI4 target and MPI2 remain
the real acceptance cases. The connected qualification below is complete in its declared scope; full Goal3 remains active.


## Complete coordinated MPI removal and actual backend qualification

`selection_work_v2_summary.json` is the authoritative scoped campaign receipt.
Focused serial5 cases/626 assertions and MPI4 all5-per-rank cases pass, including
remote-only shared-vertex conflict, no payload before metadata selection, disjoint
same-epoch edits, stale final fetch and memory/work/time/version/atomicity refusals.
The initial serial control failure exposed variable-length encoding on a fixed-size
World exchange. Separate fixed Reply/FootprintReply types fix the root; the normal
reply does not carry an unused vertex array. Failure/source/build records remain.

Retained1920-cell BL revalidation preserves the certified exact mesh/source model.
MPI4 strict removal completes250 cells/work1619 with zero final reservation conflicts;
independent exact finite geometry/manifold/links/no overlap, original sensor q.306655/
J.102558/L1.783027 and all normal slabs pass.5390 post-coalescing metadata candidates,
3771 deferrals and2704 duplicate suppressions are recorded. Planning.14455s is nested
in9.07222s local removal. The previous8.503s run stopped early and is not a speed
comparison. The transaction proposal ceiling is still4096; metadata candidates are
separately bounded/counted/timed. No bound4096 on all raw selection keys is asserted.

MPI2 engine stages also complete1920->230/work1344, with zero final conflicts and
independent q.203892/J.069845/L1.798121. Its original selected-suite receipt stays
FAIL: a separate policy control incorrectly required duplicate directed edges,
whereas two workers defer distinct conflicting stars. The corrected two-case control
passes37 assertions per rank. A companion certificate verifies every common production
source/build/library/solver pin identical, and exact unit-source comparison proves
its only C++ change is that counter expectation. It distinguishes mesh-only saved
input pins from input-free control pins. No successful mesh stage was rerun or failed
native engine promoted. Exact executed audit helpers are retained per case.

The reused actual planar RANS/SA policy fixture passes N4/M2 6->24->6 through the
production regional constructor (four fronts/work76), backend Refine-before-Coarsen,
current/BDF conservative flow transfer, original-sensor transport, native pointwise
SA bounds/history, SU2/CGNS/reference reload and real restart. Existing independent
alignment auditor was reused with an explicit folder argument; numerical routines
unchanged. Both complete finite embeddings, original-P1 composition/all full edges,
height/feet and full exact normal slabs pass. BL q.708459/J.451424/L1.719911;
sensor-only q.687230/J.408248/L1.732051. Maximum flow/BDF integral defect2.75e-16,
projection summary defect5.33e-16; transported sensor residual0. Native turbulence
checks do not establish exact post-floor SA conservation. Initialized affine fields,
not CFD evolution or viscous accuracy. Four first normal edges exist; centroid
max tilt7.45356 is separately recorded, not orthogonality. Independent sensor gradation
passes, but44 composed probes fail, max ratio24.5183963 against1.3; no metric reset,
post-freeze regularization or coarse wall spreading was introduced.

All owned jobs terminal. The next missing capabilities remain true tangential
anisotropy/localization, nonprismatic feature/interacting/exterior regions, evolved
viscous/M6 accuracy and complete lifecycle affordability. This bounded class is not
scalable parallel generation, arbitrary geometry or full Goal3 completion.
