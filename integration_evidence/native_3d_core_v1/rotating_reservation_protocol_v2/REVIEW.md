# Actual MPI qualification of rotating reservation protocol — 2026-10-12

Repository: /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated.
Branch: codex/native-3d-transport-accounting. Production remains at the published
86bd7f78243cf1e4b38d54009ed18e6bd62bbdc5 implementation; starting evidence branch
tip 2c20503dce26525b81c2ed50c9818320f4beba6b. Goal 1 active; Goal 2 not started.
No production source/header/config/object/archive or standard binary modification.

The exact isolated directory variant from rotating_reservation_probe_v1 is used.
It elects shared-vertex winners by a cyclic common priority and carries/checks
first_rank in the claim before the existing common approval vote. This word is
a priority phase, not a complete unbounded epoch count: counts differing by a
multiple of communicator size have the same order and are not rejected merely
for that difference. The matching-call lifecycle remains a precondition.

Three additional actual Catch/MPI fixtures reuse the qualified production test
helpers through a private supplementary test object. The original distributed
test object is excluded from the isolated link, and the link map verifies that
the original directory archive member is not extracted. All other 67 original test
objects and 774 archive members remain unchanged. Nothing is installed into SU2.

## Verified on MPI 1/2/4

1. Every rank repeatedly proposes the same complete admitted edge-star footprint.
   Over 2M reservation calls, exactly one rank wins each call, and each rank wins
   when it is first in the rotating order. Directory snapshot and mesh bytes
   remain identical after every reservation. A subsequent memory refusal on
   rank 0 alone rejects collectively and preserves both stores.
2. Every rank owns a spatially and topologically disjoint cube, with distinct
   node/cell/facet IDs. Each proposes its own complete edge star. Over 2M calls,
   all M proposals remain concurrently selected; snapshots and mesh bytes remain
   identical. Fair ordering does not serialize nonconflicting proposals.
3. On MPI 2/4, a test-only mutation advances only rank 0's reservation counter by 1
   before otherwise identical shared claims. Every rank receives the phase/rank
   mismatch refusal through the existing common vote. Both stores remain
   byte-identical. MPI 1 explicitly passes the serial not-applicable branch;
   one rank cannot disagree with another about its phase.

All three test cases pass on each actual MPI rank. The tests invoke the real
compiled directory/RecordStream/passive communication/common vote rather than
simulate reservation logic in Python. The extra epoch word's transport and its
existing memory admission are exercised. This does not establish full byte/wait/
allocator bounds, performance payback or generic lifecycle safety for arbitrary
external callers.

The first v1 build failed before any MPI execution because its disjoint fixture
used map-style `.second` on the actual vector<OwnedCell> returned by Rows().
Original source/compiler stderr/failed receipt are retained. v2 changes only that
fixture access. No numerical mesh or protocol failed in the v1 build. Both
completed isolated object/executable sets are removed only after retaining their
hashes/recipes/link proof; production objects and binary remain available.

## Scope and next qualification

Combined with the separately retained v1 whole-mesh probe, this supplies actual
shared/disjoint conflict and collective refusal checks alongside one N4M2
three-event wedge's fresh full mesh/metric/current+BDF/SU2CGNS audits. That prior
trajectory balanced attempts 78/78 instead of 106/54; it changed the meshes and is
not a fixed-work speed comparison.

This checkpoint does not run the entire 97-case selection against the rotating
variant, inject stale publication/version failures under the new order, test
all worker subsets, or validate stronger/thin/production-limit/restart/lifecycle
campaigns with that policy. Those are still required before promotion, together
with repeated matched-target cost/scaling/traffic and instrumentation controls.

Runs were sequential one-CPU/nice19/j1/thread1 compile/link and MPI<=4 correctness
checks after actual host/memory/space admission, with unrelated FSI work active.
No local profiling, repeated timing or cluster submission. All owned jobs are
terminal. The standard published remesher and prepared fixed-work donor campaign
remain unchanged. Prototype source, complete compile/link and original-library
pins, MPI logs and refusal/input-preservation assertions remain in validation.json.
Full Goal 1 performance/default 512/practical-limit gates remain open; no curved
geometry, production 3D BL, full-unsteady or general affordability qualification.
