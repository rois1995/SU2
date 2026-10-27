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

Fresh six-rank M6 build46 was intentionally interrupted908.19s without debugger
attachment. Setup/source passed; no stage receipt. All workers remained active,
and all owned descendants were verified gone. This is neither failure nor acceptance. Broader
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

## Connected ownership and scheduling change (build47 pending)

Active constructors/loads and publication establish per-face validity, but
first mobility activation currently copies legacy state without the actual-
chord positive-projection proof. Close that trust gap before enabling reuse.
At activation, exact cyclic original-facet faces with valid dimension-zero
bindings and bitwise reference coordinates inherit the authority constructor
certificate, provided no explicit face/edge override applies. Other faces must
resolve their actual retained traces and pass policy/coverage validation once.
Thereafter reuse active-state proofs by provenance, without a cache or epoch. A private view must
pass its existing revision check. The oriented face IDs/anchor and all complete
BoundaryPoints (coordinates and binding fields included) must match accepted
records, with no applicable cover/trace override. Same IDs alone are insufficient.

Affected callers: ParameterContainsFace, ExportParameters and PrepareCurvedAtlas.
They may reuse an exactly matching accepted face's validity but must still
materialize coverage needed for reconstruction. Publication must retain accepted
before coverage for old/new chain equality, while avoiding re-proving its already
validated policy/projection/traces; inserted or metadata-modified after faces
remain fully checked. Constructor/load and donor restricted-envelope validation
remain complete trust boundaries. This is certificate ownership, not permission
to skip tests of new candidate geometry. It does not prove the remaining work
reservation sufficient.

Acceptance must include unrelated private edits reusing accepted faces, same-ID
moved records and explicit overrides refusing reuse, stale revision rejection,
coordinated edge-path changes, publication/source/archive tests, actual mapped
runtime and fresh M6. Pending-point provenance must be explicit rather than
assuming every UINT64_MAX identity is transient; reserve-ID enforcement is not
currently local to BoundaryParameterState. M6 was intentionally ended and every owned descendant verified terminal before
implementation. Build47 now pins the connected change; sources/helpers stay
unchanged until terminal.

Scheduling belongs to this same connected change: Selection::Add currently
projects all three hypothetical splits of every demanded face and certifies
both child faces before it can enqueue anything. The chosen complete-star
operator later repeats projection, validity and before/after geometry progress.
Replace the eager custom feasibility screen with a queue of current finite
geometry excess, prioritizing physical edge span. Existing selected-operator
GeometryRefine progress, edge retirement, collective publication and final
gates remain authoritative. A serviced/rejected edge leaves the construction
frontier; incident Touch does not retry it within that generation. Later frontier
reseeding owns retry, so this design does not assume immediate reactivation. This reduces scheduling work to current demand
enumeration plus selected cavity validation; it is not an additional operator
fallback or a larger arithmetic limit. Convergence still requires actual workflow
and M6 evidence. The connected draft has been applied after M6 termination. Build47 is pending;
no validation or capability claim follows from applying it. Tests cover the
legacy activation gap, exact accepted-face reuse, overrides and real MAX-valued
mesh identities. Existing stale-view/publication and source/archive tests remain
required, as do the actual mapped workflow and fresh M6.

Build47 terminal PASS1210.22s/1293 unchanged pins. Filter audit PASS239serial/
216MPI/57atlas and all4 focused cases. Focused tests:3/4 pass,114/116 assertions.
Activation rejection and prior query tests pass. The real MAX-ID test found that
ReverseTrace replaces the final interval endpoint without first checking it,
allowing malformed input to become valid. Build48 validates nonempty, strictly
increasing complete0-to-1 intervals before reversal; containment returns false
for this malformed contract. No gate is relaxed.

Actual build47 mapped runtime FAIL12.96s unchanged pins/limits:49 accepted edits/
346 attempts,17 geometry-progress edits,292 optimization trials/2 validated,
3 quality-progress reports. At exhaustion it reports0 cached-invalid cells,
0 queued edges and repairs, but this is not a final complete census or an accepted
return. Do not infer all final gates passed. Work remains67,207,168. Profile the
remaining candidate/improvement/publication cost next; no budget increase.

Build48 terminal PASS48.15s/1293 unchanged pins. All4 focused cases PASS9.88s,
116 assertions. Mapped trace FAIL13.84s unchanged pins/limits. Current raw capture:
153 cells/57 nodes/47 boundary points/90 faces, revision40;0 explicit traces/covers.
The selection is empty after a phase reset; its zero counters are not a successful
census. Exclusive work103,793,158 across scopes: BindingParameters22,181,942,
CoverageTraces18,559,022, EqualTraces16,712,204, ValidateCoverage14,558,124,
RegionBoundary10,697,586. This still does not demonstrate descriptor accumulation.
Next: existing native-quality replay for the captured current mesh, repeated-edit/
publication/source/archive regression, and fresh M6 on the connected scheduler.

Build48 atlas_checks PASS51.89s,57 cases/8363 assertions, unchanged pins. This
includes overlapping repeated movement and publication/source/archive obligations
within fixture scope. Complete mapped adaptation remains failed. A bounded
case-local native-kernel census is being prepared to distinguish actual metric/
quality debt from empty selection statistics, reusing the compiled production
Measure/Orientation implementation rather than another quality formula.

Separate persistence-bound review remains open: BoundaryAtlasBudget derives its
work/bit allowance from encoded atlas descriptors/policy, while implicit current
point/face counts are not inputs. FromPayload and Rebind still validate every
current face. A fixed original policy with growing implicit mesh therefore has
constant archive arithmetic allowance. This is a source-level bound mismatch,
not a demonstrated current-case failure; do not hide it with a blanket increase.
Any correction must account for current mesh/descriptor complexity and retain
full constructor/load proof. It does not explain the stage's separate67m cap.
