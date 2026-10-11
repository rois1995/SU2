# Native 3D recovery coordination and exact placement limit — 2026-10-11

Working repository `/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated`, branch
`codex/native-3d-core`, predecessor `a410dad7a47bee500d9ccc2d32d8d08250937470`.
Implementation/evidence checkpoint `3b84e6a3b9ba9ae1467da4f556dacab84e3ef073` pushed to origin/codex/native-3d-core; full remote SHA verified. Full Goal 1 stays active.

## Change and qualification

Refine previously stayed in repair mode after its single recovery queue was
seeded, suppressing ordinary splits that accepted recovery edits could unblock.
Selection now separates repair tracking from the current operation. After
recovery, workers collectively prefer an available split; otherwise they resume
the same dependency-tracked repair queue. Recovery is seeded at most once.
Measurement/update admission charges retained repair storage during split rounds.
This adds one scalar collective per incomplete post-recovery refinement round.
The default 512-round budget, immutable source/geometry, frozen q >= 0.20,
J >= 0.05 and L <= 1.8 criteria are unchanged. No 2D/MMG, metric, BL/fade,
interpolation or noise policy change is included.

`COORDINATION_CHECKPOINT_VALIDATION.json` verifies identical production archive
and thin-member identities across all controls/replays and the full configured
68-object test driver, receipt/artifact integrity and phase closure. MPI1/2/4
configured tests pass automatic Repair/Refine/Coarsen, single recovery,
strict completion, atomic input preservation, budget/refusal/retry, default
rejection and native2D/MMG controls. These C++ checks are not an independent
physical-flow certificate.

Six actual manufactured driver cases independently qualify three events each:
refine/coarsen/refine, worker return, ReplaceMesh, current/BDF2 transfer,
unchanged reference sidecars, geometry/marker/embedding/volume checks, frozen
metric residuals and actual SU2/CGNS output plus readback.

| Folder suffix | N/M | Accepted tetrahedra over three events |
|---|---|---|
| driver_n1_v1 |1/1|88 /36 /94|
| driver_n2_m1_v1 |2/1|88 /36 /95|
| driver_n2_m2_v1 |2/2|88 /36 /95|
| driver_n4_m1_v1 |4/1|88 /8 /70|
| driver_n4_m2_v1 |4/2|88 /9 /69|
| driver_n4_m4_v1 |4/4|77 /12 /70|

Every folder has prefix `coordination_`. Minimum qualified q/J are
0.258663/0.062176; maximum metric edge is 1.785100. Maximum transported-state
relative error is 3.84415e-16; transported sensor tensor error is zero.
Representative grids are `coordination_driver_n4_m4_v1/envelope_s0.vtu`
through `envelope_s2.vtu`, with actual SU2 and final CGNS files alongside.
The manufactured affine histories are visible in the VTUs; no CFD iterations
were performed.

`coordination_rank_phases.csv` retains 578 exclusive/nested rows. The manifest
records per-event parent/remesh maxima, max/mean, native requested bounds,
lifetime RSS and useful accepted operations per parent second. Parent includes
manual manufactured metric preparation, diagnostic observation and replacement;
requested output is separate. These single contended runs have different
partition trajectories; they establish neither fixed-work scaling nor a
speedup. Full MPI traffic/wait, allocator/ParMETIS/CFD memory calibration,
instrumentation overhead and repeated cluster measurements remain open.

## Larger replay still rejects

Both exact n3 replays retain the original 162-tetrahedron import, original P1
sensor and faceted geometry. The 512 probe accepts 64 private edits and rejects
448; the 1024 diagnostic allowance stagnates at round 631 with the same 64
accepted edits and 567 rejections. Its private mesh is byte-identical to the
512 result: 240 tetrahedra/81 points, qmin 0.201655/Jmin 0.091521, 22 oversized
edges touching 53 cells, maximum length 2.464292. One recovery is used.
The independently checked private volume/incidence/embedding/facet/marker gates
pass; strict edge completion fails. Accepted CFD mesh, state, reference and
source remain unchanged. Increasing the production budget is not justified.

Inspect `coordination_replay_r512_v1/replay_rejected.su2` and
`coordination_replay_r1024_v1/replay_rejected.su2`; audits list all coordinates,
lengths and incident cells. Engine wall/metric/reconstruction are respectively
4.67384/3.20623/0.75633 s and 4.47129/2.96480/0.81341 s. Nested edge trace
3.47988/3.23571 s overlaps exclusive phases. Changed trajectories and contention
prevent speedup claims; rejected events deliver zero accepted CFD work.

## Exact arithmetic separates two obstacles

`audit_placement.py` uses rational representations of stored binary64 values.
All 22 oversized edges have rounded midpoints off the exact edge. For 14,
a stronger certificate proves no binary64 interior point exists anywhere on
the segment. In a single binary64 binade, the varying coordinate is an integer
grid index k; other coordinates must be dyadic. Their odd denominators force
k to be a multiple at least as large as the complete segment's grid interval
count, leaving no interior k. Self-checks include dyadic splittable controls.
Absence of this sufficient certificate does not prove placement possible.

Crucially original edge 9–14 separates original facets 30/31 and markers
z_minus_b/z_minus_a. It connects (1/3,2/3,0) to (2/3,1,0), with the fractions
understood as the stored rounded doubles, and has frozen length 2.18657193.
It is an immutable marker-junction segment with no binary64 interior point.
Exact preservation plus L <= 1.8 is therefore incompatible for this segment;
a conforming boundary cannot silently replace that marker line by a nearby
one. This is a demonstrated limit of the exact binary64 geometry contract,
not a universal failure of anisotropic meshing or proof that all interior
residuals are infeasible. v1 certificates are retained; v2 adds original
marker provenance and names the single certified oversized original junction.

Interior unsplittable edges can instead be removed by privately retriangulating
a complete admitted cavity, with a representable insertion point and an
unchanged external skin. Existing Cone/ValidateFixedInterface are relevant
building blocks; ordinary split routing currently assumes two children per
parent and must be reconciled before accepting such proposals. Boundary
insertion needs a separately certified coupled surface/volume operation.
Nonconvex same-marker components also retain raw-facet seams (23 canonical
n3 groups versus 7 for n2); finite-union containment could remove artificial
seams, but cannot remove an actual marker junction.

Next: retain this original case as a feasibility/rejection adversary; add a
physically aligned marker fixture without replacing historical evidence, and
test bounded cavity insertion/reconnection against exact skin/metric guards.
Any eventual geometric error budget needs an explicit contract and its own
validation; no silent tolerance or target relaxation is part of this checkpoint.
Continue larger/direction-changing/thin/wedge/restart and practical rank/size
limits, complete profiling/calibration, then prepare closed SGE campaigns.
No overnight cluster launch is requested. AdapNoExt/foreign jobs/submodules/
ClusterResults remain untouched.
