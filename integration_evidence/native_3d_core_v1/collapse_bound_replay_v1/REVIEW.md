# Frozen-sensor collapse-bound replay — 2026-10-12

Repository: /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated.
Branch: codex/native-3d-transport-accounting. Production checkpoint:
8c3b40c9be1cca3919d627c7b3851b65bc05312a, unchanged by this investigation.
Goal 1 remains active; no Goal 2 activation or cluster submission.

The retained full_star_facet_trace_n8_v1 trace comes from bb87e97fe1d89ee69ae8deac8fb447d0996f2d5e.
Its original source-matched receipt and analysis certificate are verified before
replay. Current CollapseVertex and ValidateMetricChange source hashes also match
the corresponding files in that historical complete configured build receipt.
This does not claim the old trace is a fresh execution of the current binary.

## What the ambiguous rejection actually means

Reconstructing each complete old vertex star follows CollapseVertex: remove cells
containing both directed endpoints, replace the removed endpoint by the retained
endpoint in all surviving cells. All 940 saved proposals remove at least one cell
and leave a nonempty patch. For every one of the 709 accepted proposals, the
reconstructed cell identities/connectivity/coordinates exactly match the saved
fresh topology (new cell IDs are not used for this comparison).

Consequently, all 216 messages saying coarsening "does not remove cells or
introduces an oversized edge" concern oversized changed edges, rather than no
cell removal. The second truncated event has 60 such messages and 15 shape/sliver
messages. Its previously established lack of identical-full-star retries remains
unchanged. A shape failure can coexist with oversized edges because shape is
checked before changed-edge excess.

## Conservative sensor bounds

For a fixed chord d and original nodal tensors M_i, define
q_min=min_i(d^T M_i d), q_max=max_i(d^T M_i d).
Original P1 interpolation is a convex combination inside each original tetrahedron,
so the integrated chord length lies between sqrt(q_min) and sqrt(q_max). This
uses extrema only to reject impossible proposals; it does not spread nodal tensors,
modify interpolation, regularize a target, or replace the actual metric query.
All coordinates/tensors/dot products/comparisons here use exact binary64 rationals.
The squared threshold is the exact rational square of the production double 1.8.

| Saved coarsening epoch | Attempts / accepted | Exact directional bound identifies doomed proposals | Twelve-scalar component bound identifies doomed proposals |
|---|---:|---:|---:|
| 0 | 428 / 272 | 76 of 156 rejections | 35 of 156 |
| 1, truncated at production 512 | 512 / 437 | 47 of 75 rejections | 24 of 75 |

Directional bounds require access to all unique original tensors: 45 and 69 in these
particular events. This is not a bounded global MPI implementation for arbitrary
meshes. The inexpensive alternative stores each of six tensor components' minimum
and maximum (12 scalars). For each signed coefficient in d^T M d, select the
component extremum that gives a lower bound. Exact replay verifies this bound is
no greater than the nodal directional minimum. Lost component correlations can
make it negative even for SPD tensors; the adversarial self-check covers this.
A negative or uncertain bound must retain the candidate, not reject it.

Neither bound rejects any saved accepted proposal. Seven of the second event's
15 shape/sliver failures also have a certified oversized new edge. Five have all
changed-edge upper bounds below the length threshold, demonstrating an independent
shape blockage that a length-only filter cannot remove. Full witnesses retain
epoch/round/directed edge, changed edge endpoints and exact squared bounds in
analysis.json; component_analysis.json retains the constant-size witnesses.

## Decision and limits

Do not add the twelve-scalar MPI filter yet: it identifies only 24 of 75 known
rejections in the truncated second event and does not establish completion within
512 rounds. The complete separately certified1024 event requires 615 attempts, and
its third coarsening requires 1002; this replay does not cover the additional
attempts or predict a changed selection trajectory. Skipped attempts also change
reserved IDs and require fresh independent mesh/state certification. Batching
independent edits or improving ordering may offer more than this weak envelope;
those need evidence rather than a work-budget increase disguised as optimization.

Any future production predicate must prove outward/rounding-safe bounds, cover
empty owners and remote stars, account for its reduction/storage/selection cost,
and retain complete dependency admission, invalid-field repair, reactivation,
atomic validation, and actual-query composition. A sensor-only bound is not a
certificate for an arbitrary composition callback. No composed 3D BL support,
geometry/quality relaxation, performance/scaling or new MPI result follows here.

Both scripts pass their exact self-checks and all saved records. Runs are sequential
one-CPU/nice19/thread1 offline correctness analyses under observed contention,
not a local timing campaign. No SU2 executable is built or run; production, the
current cluster comparison recipe, the 512 default and all scientific guards remain
unchanged. Original trace/failure evidence is retained. Repeated cluster cost,
rank/weight/wait/memory/overhead calibration still gates full Goal 1 completion.
