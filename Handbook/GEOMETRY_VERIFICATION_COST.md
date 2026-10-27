# Geometry verification cost: current diagnosis

2026-10-27. DC-17/DC-18 remain open. No accepted M6 mesh or complete adaptation/
transfer/restart workflow exists. This follows the
[reconstruction decision](WORKING_SURFACE_RECONSTRUCTION_DECISION.md), rather than
replacing its coordinated fixed-reference design with more arithmetic limits.

## Demonstrated complete-workflow failure

Build40 broad serial is terminal FAIL102.73s, unchanged pins:234/235 cases,
1130211/1130213 assertions. The curved Euler/RANS variants of mapped runtime both
exhaust67,207,168 geometry work after29 private edits/35 attempts, exactly as
before the neighboring-trace correction. Reported remaining debt is8 geometry
cells and no metric/height cells; this does not establish all final gates.

Build41 diagnostic replay is terminal FAIL14.89s with unchanged pins and unchanged
limits. Raw capture before failing rejection measurements independently decodes
88 cells/37 nodes,30 boundary points/56 faces, association generation23. Both
captures contain **zero explicit edge traces and zero explicit face covers**.
The failure therefore does not exercise accumulating explicit subdivisions.
Evidence: `staged57/cases/mapped_work_trace_20261027T181142_3162122/`, including
`work_attribution.json`, complete raw owned/parameter/authority files and manifests.
The existing stage decoder checks record count/length, IDs and coordinate bits.

## Measured callers

Opt-in AtlasOps scopes measure inclusive and exclusive work without changing
admission or mathematical checks. These exclusive totals combine both physics
variants and several budget scopes; do not confuse them with one event's limit.

| Operation | Calls | Exclusive work | Fraction |
|---|---:|---:|---:|
| ValidateCoverage | 3,144 | 48,322,792 | 36.13% |
| CoverageTraces | 12,576 | 28,812,208 | 21.54% |
| EqualTraces | 9,430 | 27,011,584 | 20.20% |
| BindingParameters | 55,358 | 24,674,392 | 18.45% |
| ResolveSingleChart | 15,334 | 2,433,980 | 1.82% |
| SurfaceError | 1,460 | 1,722,352 | 1.29% |
| ValidateCoverageReplacement | 44 | 776,046 | 0.58% |

Total recorded exclusive work133,753,416. Generic coverage/trace verification,
including repeated single-triangle arrangements, dominates. The accepted/private
representation remains implicit and has not grown a history of descriptors here.
The optional diagnostic is disabled by default; it is not a runtime cache.

## Connected verification change under validation

Use the shared `MappedFacetReference` certificate routines for every caller.
A one-piece map whose active triangle is exactly the ordered unit simplex has a
known tiling, so it needs no generic edge arrangement to establish that tiling.
Retain all endpoint admissibility/canonical-key checks, active/reference parameter
validity and orientation, marker/body identity and restricted cubic-net positive
projection. Derive its three boundary traces directly, using the same original-
seam spelling as the generic routine, and still compare all supplied traces.

Two valid single linear trace pieces with complete0-to-1 intervals are determined
by their canonical original-reference endpoint keys. Equality must first validate
both pieces; malformed intervals, collapse and discontinuity still reject. Different
original charts sharing both endpoint keys describe the same original edge.
Multi-piece paths retain full comparison, including active parameters and bends.

Global old/new region conservation, genuine-feature/interface coordination,
owner-cell versioning, contact, surface error, height and final q/J/size checks are
unchanged. `ContainsFace` alone is not a substitute: it lacks the restricted-net
positive-projection certificate. No extra cache, epoch, model hash, work-limit
increase, CFD-mode switch or alternate path selector is introduced.

Build42–45 retained the final gates and unchanged work limit. The central
single-piece/negative-Jacobian regression passes; its first invalid witness was
incorrect and was replaced with an analytically negative corner Jacobian. The
projection test exposed fixture setup errors (authority ownership, then accepted
rather than private context); these are not runtime capability evidence.
Build44 mapped runtime still fails after30 edits/42 attempts,8 geometry cells,
with no geometry edges queued. Build45 probe replay is terminal FAIL17.91s,
unchanged pins. All457 probes throw:133 invalid interior bindings and324
parameter-authority refusals. No probe reaches progress scoring.

The connected query correction uses canonical vertex/edge bindings when a facet
projection lies exactly on its boundary. Interior-only `FacetPoint` remains
strict. Temporary projected points have forward associations but no mesh IDs:
face containment derives their single-chart coverage and traces from bindings,
retains known-endpoint edge contracts, policy and positive-projection validation.
Error evaluation uses the same forward associations. Persistent ID-based paths,
publication and old/new region conservation remain unchanged. No fallback,
clamping of out-of-domain facet coordinates or limit increase is added.

Build46 terminal PASS90.19s/1293 unchanged pins. Focused certificates and
all-edge probes PASS6.96s,93 assertions/2 cases. Compiled filter audit passes:
237 serial/69 required,214 MPI/55 required,55 atlas/31 required; both new cases,
mapped runtime, M6 and the separately scheduled wide workflows are selected.

Mapped probe replay is terminal FAIL13.98s, unchanged pins/gates.318/342 probes
now score;22 still reject invalid interior proposals and2 reject authority.
Construction records9 geometry-progress edits,22 accepted/30 attempts,39 invalid
cells and31 queued edges when work67,207,168 is exhausted. The previous zero-queue
interface defect is fixed, but generation still fails. Preserve this distinction;
do not raise the limit or report a full-workflow milestone.

Fresh six-rank M6 build46 is running without debugger attachment. Broader
serial/MPI, independently resumable wide workflows and accepted M6/BL transfer/
restart remain required. Pinned code and helpers stay unchanged during the run.

Build46 raw captures independently decode60 cells/29 nodes/27 boundary points/
50 faces, revision20, with zero explicit traces/covers in both variants. Build41
had88/37/30/56, revision23 and likewise zero descriptors; these are different
trajectories, not matched event costs. Exclusive work across the two physics runs
and budget scopes totals129,734,316: BindingParameters34,218,138,
EqualTraces29,909,348, CoverageTraces28,630,540, ValidateCoverage23,020,230.
The remaining small-workflow failure therefore does not demonstrate renewed
persisted descriptor growth. The reviewable implementation checkpoint is
`6c067993cb`, pushed to the authorized fork/branch. No quality or resource gate
was loosened. Detailed receipt: build46 mapped case `work_attribution.json`.
