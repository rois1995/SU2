# Static assessment 2 — reconciliation and fixes

Repository: `/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated`.
Branch: `codex/native-3d-bl`. The supplied report read frozen b449b5b800,
excluded private work and all runtime evidence, and did not run any code.
Preserved source: [assessment](../integration_evidence/native_3d_bl_v1/static_assessment2_v1/assessment_source.md).

## Findings acted on in this connected patch

| Finding | Disposition and change |
| --- | --- |
| 3D-C2 / N3-C1, inherited 2D refusal | Agree. Collective admission now returns refusal without rebuilding or publishing. Both ordinary steady and unsteady callers skip replacement files; rejection never passes partial geometry to the solver. Native FP support remains rejected; its MMG-only caller requires complete results before altering the donor/window state. No extra window snapshot or unsupported FP recovery path was added. |
| Deadline after rebuild | Agree it wasted completed transfer. Deadline now admits replacement once before any rebuild; an admitted rebuild completes. It remains a soft safe-point budget, not a hard interrupt. |
| N3-P5 / N3B-M1 | Agree. First-row proposals now use existing complete-vertex reservations concurrently across owners, with disjoint rank/epoch identities and global proposed-work accounting. This removes the one-cell global election; no scaling claim from correctness runs. |
| N3-C7 | Defensive fix: require the one-cell star behind the five-node first-row reservation explicitly. |
| N3-M1 | Agree. Initial facet replication uses the existing typed round all-gather, eliminating P copies of the facet list on each sender and the P-times-boundary admission term. Per-rank replicated reference storage and O(P) control buffers remain. |
| N3-C4 / N3B-C3 | Agree. Whole reference components keep connected sphere/disk and vertex-link validation under their construction storage admission; local cavities keep their existing face cap. Large planar components no longer silently lose canonicalization at 512 faces. |
| 3D-M6 | Agree. Remove the unrelated 1 MiB file cap; admit parsing/construction bytes before allocation. Default standalone reference decoding/loading still has a 2 GiB storage ceiling. No unbounded parsing or removal of input validation. |
| N3B-C4 | Agree with the floating-point obstruction. Keep metric-balanced placement when certified; physical proposals also have a rounded geometric midpoint/projection fallback on geometric refusal. Forcing all wall splits to midpoints regressed the previously passing RANS fixture and was rejected, with evidence retained. Final full-field edge checks are unchanged. This cannot guarantee arbitrary oblique exact dyadic chord representability. |
| N3-C2 | Split-placement numerical failure blocks the local candidate rather than rejecting the event. Strip/proxy exceptions still need independent recovery; donor/memory errors remain separately guarded. |
| N3-C3 | One-cell collision import donor-cap refusal blocks that proposal. Other import/setup failures retain collective handling. |
| N3-P1 / N3B-P1 | Partial fix: unchanged oriented cavity skins do not require rebuilding a global collision index. Actual changed-skin curved proposals retain full clearance. An incremental distributed index remains necessary for large curved adaptation. |
| N3B-C5 | Agree. Admission counts unique validated physical distance roots, not duplicated/discarded feature roots. Existing 65536 valid-cut and band limits remain explicit safeguards. |
| N3B-P2 | Agree. Geometry-only volume rules reuse their points across complexity trials. Hidden-support rules still invalidate against sensor/core because their angular refinement uses density. Each trial recomposes current sensor/BL at actual retained quadrature queries. |
| N3B-M3 | First sampled/predicted BL window prepares immutable geometry authority before metric composition, as the ordinary metric entry already does. |
| N3B-C10 / N2-1 | Agree. Shared MMG BL application exceptions are elected collectively; 2D subdivision refusal throws through its existing collective wrapper. No metric formula or subdivision threshold is changed. |
| N3-C6 / 3D-C4 | Partial: optional coarsening is skipped when no proposal work remains after a completed refinement. Failed started coarsening still refuses the event; it does not authorize accepting arbitrary partial state. |
| 3D-C14 | Update public scope and describe work/time controls and refusal behavior. |

## Findings that require larger work, or are acceptance contracts

Agree that curved clearance, expanded BL/curved serialization (N3-M2), dense
exchanges, owner imbalance, map storage and conservative memory envelopes limit
scalability. Removing reservations or making them concurrent before complete
expanded dependencies are admitted would introduce correctness failures. Those
architectural items remain open; no claim that this patch resolves 16–256-rank
or million-cell scalability. No such ranks are locally available.

Quadrature convergence/visit/point/storage ceilings (N3B-C1/P3/C15) can refuse a
coarse thin/ridge target. Their proposed failure frequencies and timing numbers
are hypotheses, not measured evidence. Removing bounds or accepting unresolved
integration is unsafe: a dedicated bounded convergence/recovery change is still
needed. The donor thread envelope (N3-P2), geometry-queue reactivation (N3-C5),
oversized planar stars (3D-C5), general projected junctions (N3B-C9), analytic BL
distance/normal support (N3B-C7), sidecar identity/durability (N3B-C12) and wall
reconstruction 65–128-skin admission (N3B-C6) remain open.

First-height and footprint arithmetic limits (N3B-C2/C8/C11) are real precision
limits, but changing the 1e-8 height contract to an automatic looser tolerance
would hide unresolvable geometry. Preserve requested heights and diagnose/support
resolvable coordinate/height scales; do not claim tiny translated layers now work.

Original-wall tangential sizes and sensor-only bounds/gradation (N3B-C13/C14) are
explicit current policies. Do not reset the normal, spread fine nodal BL tensors,
weaken finer sensor demand or change fade in response to reconstruction failure.
Composed-field gradation remains separately audited and unresolved. Strict final
shape/geometry/height/edge requirements (3D-C7) remain unchanged.

The claim that 2→3/3→2 reconnection is missing is too broad: the existing Cone
primitive and its tests already exercise those patterns. The pending private
2/3/4-cell operator combines that kernel under actual-field shape scoring; it is
not yet a production operator and does not solve the stronger target. Static
reading alone also cannot establish that accepted meshes are globally correct,
conservative transfers qualified, or costs meet the user's affordability goal.

Unchanged inherited 2D scaling/work-budget findings remain open apart from shared
recoverable replacement and collective exception handling. This is not a rewrite
of the qualified 2D remesher.

## Validation

Final build PASS. Serial22 cases/186116 assertions PASS, including initialized
RANS/SA construction/release, conservative current+BDF transfer and SU2/CGNS/restart.
MPI2/MPI4 PASS7 cases per rank, including MPI1/2/4 worker subsets, disjoint
first-row reservations, one-slot admission and one-rank refusal followed by
successful replacement. All stages share verified source/build/binary/object pins.
Failures and harness corrections are preserved. Exact provenance:
[focused evidence folder](../integration_evidence/native_3d_bl_v1/static_assessment2_v1/).
Serial/MPI2/MPI4 stages are bounded, sequential, reusable when their pins and
saved artifacts match. They exercise collective refusal/current+BDF preservation,
large reference/chart admission, first-row vertex reservations, actual BL edge
bands and density-aware quadrature invalidation. No full regression, real CFD
convergence, cluster submission or timing/scaling qualification is implied.
Goal 3 remains active and incomplete.
