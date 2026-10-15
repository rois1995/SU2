# Goal3 — support selection at the remaining transition blockage

Repository `/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated`, branch
`codex/native-3d-bl`. Full Goal3 remains active and incomplete.

## Preserved residual and continuation

The1110-cell result independently passes positive volume, closed skin, no
positive-volume overlap, declared first altitude/normal feet and arithmetic quality
checks from exported actual-centroid tensors. It has174 oversized edges of1512,
maximum2.126855:61 interior,113 physical surface/junction. Height error is below
1.67e-15 relative over48 wall faces. Full composed edge integration is native,
not an independent quadrature oracle. Read `interior_grid_independent.json` and
`interior_residual_classification.json` under wall_reconstruction_v1.

One further bounded4096-work invocation advances41 edits/40 insertions to1402
cells,137 oversized edges and squared excess3.482566→1.587451. This gives8192 work
and206 edits from the473-cell chain start. Both invocations retain the original
sensor/BL/fade/height/quality target and still reject. Mesh reload resets identities
and search state, so this is not proof that increasing one event's budget is enough.
No further whole-grid budget ladder was launched.

## Actual local limitation

A point-only replay of the worst remaining interior edge has four old tetrahedra.
After109 trials it retains shape loss .133918 and fails the unchanged gate.
Its source-pinned original/best-rejected patches are in `curved_reconnection_probe`.
The greedy free-apex cover then requests78 cells, exceeding the64-cell admission
limit. Independent exhaustive enumeration finds a two-apex cover requiring26 cells;
`diagnose_cover.py`/`cover_diagnosis.json` reproduce this on actual source data.

The shared cover selector now minimizes total complete-star cost within eight
apices. Branching on an uncovered opposite-vertex pair requires at most511 states.
Missing cost entries are unavailable candidates. Engine classification reuses
Directory::LookupBatch/FetchBatch: each candidate's complete versioned star proves
whether any authoritative physical face contains that vertex. Oversized stars
remain unavailable; stale/protocol failures remain event failures. Physical points
are not inferred from distance to CAD or a marker-name convention.

Only the final selected edge/apex union becomes a geometric proposal, and it must
still fit64 cells. Classification payloads are separate individually bounded stars;
existing batch storage admission applies. Work, field, geometry, first-height and
quality/full-edge gates are unchanged. New classification counters expose execution.

## Qualification so far

The focused primitive/cover control passes86 assertions. The actual26-cell coupled
patch passes464 assertions in132 trials and produces30 cells. Native actual-field
Refine and geometry/height gates pass. Independent exact local geometry verifies
positive volumes, no positive-volume overlap, fixed interface/coordinates, cavity
volume and unchanged original donors/reference (`cover_geometry.json`). This patch
has no physical wall face, so its zero local height error is not a new global wall
height qualification. The preceding1110-cell whole-wall audit remains scoped above.

The costed-source MPI1/2/4 checks PASS:533/596/549 rejected cells, classified
vertices7/6/8, placement trials333/482/409, each bounded512 work. Original donor
and geometry preservation is independently audited. These are protocol checks;
owner-dependent trajectories differ and curved commits remain serialized. Their
exact source/binary/helper/artifact evidence is in
`joint_protocol_costed_cover_audit.json` and `costed_cover_audit_sources`.

A cap-cover replay passes25→28 cells in two trials, but none of its two proposed
support apices moves. Trying the same cap with only its actual edge star also
passes3→6 in two trials/73 assertions, including an independent exact local
geometry audit. The engine therefore tries point-only placement early on eligible
flat non-wall edges; actual wall heights still require coordinated wall reconstruction.
Final-cap MPI2 protocol and before/after original donor/reference audit PASS in
`curved_joint_protocol_cap_mpi2`/`joint_protocol_cap_mpi2_audit.json`. The preceding
costed MPI1/4 binary is not relabeled as final-cap qualification.

## Complete-generation result and next limitation

The strict `curved_costed_complete` run still FAILS:1402→1411 cells, two accepted
insertions,133 remaining long edges, q .200176/J .050224; squared excess
1.587450761→1.445944476. It exhausts4096 work, with1062 placement trials and two
validated placements. The last local rejection says a joint point cannot see its
retained skin; that is not proof that the complete remaining target is infeasible.
This connected change repairs the reproduced support case but does not resolve
full curved generation. Do not repeat continuation budgets or infer robustness
from local success. Next, preserve actual rejected dependencies/proposals through
the existing rejection callback on the saved-grid path, then address the missing
coordinated geometry/connectivity capability on that evidence.

All owned jobs are terminal. Read `support_selection_summary.json`; all full
Goal3 gates stay open. This is not full curved generation, CFD/transfer/restart or
performance qualification. Read the original full requirement audit before closure.
