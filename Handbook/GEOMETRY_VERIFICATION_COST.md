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
attachment. Setup/source passed; no mesh snapshot. All workers remained active,
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

M6 progress correction: stage_trace files exist even before mesh snapshots. Old
build46's last aligned window13504 at828.68s had66,336 accepted private edits,
4,447 geometry-progress edits and zero shape measurements. It was progressing
when deliberately interrupted. Fresh build48 at window3904/338.76s has23,173
accepted private edits and4,929 geometry-progress edits; all six ranks report
zero shape measurements during construction. These are private operator/cost
observations, not accepted meshes or repeated-event qualification.

Independent native-kernel quality census of both build48 small captures:153
cells,5 below q=.20,5 below J=.05,10 with edge>1.8,0 nonpositive orientations.
Worst q=.1430794209846515/J=.03201514774372057 at cell60497; maximum edge
2.637877935007608 at cell60498, uniform Tensor{4,0,0,4,0,4}. Saved sources,
original link recipe/dependencies, raw/decoded input and production-kernel hashes
are in the case-local native_quality_census.json. No geometry, BL, transfer or
acceptance verdict follows from this metric-only census.

## Next phase handoff change (reviewed draft, not applied)

Selection.Reset currently discards current cell/edge/geometry measurements at
each construction/improvement handoff. Publication already drops/replaces changed
rows, including metadata-owner cell versions. Preserve current rows, incidence,
frozen edge certificates and geometry scores; rebuild phase eligibility without
new geometry queries. Construction decisions ignore stored q/J. Improvement
measures only missing shape/requested-height values, with an in-place refresh
that retains geometry/edge support. Unknown rows remain unknown after partial
failure. Full reset remains mandatory after coarsening checkpoint rollback.

Freshness must check owned version, ordered vertex IDs and cached endpoint/field
certificates. Counters subtract/add exactly once; phase-facing valid/target_valid
must be recomputed separately from stored quality. Preserve repair support,
waiting age and rejection state; publication Touch owns reopening changed support.
Reserve repair-map growth before phase transition. No new cache/epoch or relaxed
acceptance/resource limit is involved. The uncompiled draft is temporary:
`/tmp/dc17_phase_census_draft.cpp` and `.patch`. M6build48 is running, so compiled
sources remain unchanged. Validate actual mapped workflow, stage-progress/partial
census, coarsening rollback, stale metadata, repeated events and MPI before reuse
is considered integrated; convergence is not implied by this design.

Latest build48 repeated-overlap cost receipt also passes16 events under its
independent summary:7 current points/10 faces,4–7 reference triangles,10–16 face
pieces/10–15 trace pieces,54–55 bits,3,965,883–7,997,772 arithmetic work/event,
5,759,072–6,270,752 admitted peak bytes,18,584–25,128 accepted resident bytes,
8,376–12,224 payload bytes and0.592–0.640s/event. Returning phases satisfy the
existing descriptor/width/work/storage bounds. Lifetime process RSS36,675,584 is
not an event allocation peak. Scope is private overlapping surface movement,
not full adaptation. Receipt: atlas_checks_20261027T192923_3373582/
reference_cost_summary.json.

M6 build48's first quality census at~737 engine seconds contains667,993 current
cells:198,390 size-bad,7,090 geometry-bad,64,554 quality-invalid and1,174,431 repair
features. This demonstrates premature quality handoff while productive target
construction still has substantial debt. First quality window records27 real
optimization moves and8 swaps, exercising mobility rather than fixtures alone.
All remain private observations, not accepted adaptation.

A five-second19Hz software perf sample of the six owned workers (528 samples,
no debugger/SIGSTOP) shows five ranks polling MPI and one in geometry interval/
exact-arithmetic work (Blossom, GCD/division and interval rounding). Frame-pointer
unwinding is unreliable, so this does not identify a specific caller or prove a
collective defect. Full command/data/reports are case-local. It motivates proper
lifetime/scheduling assessment rather than arithmetic tuning or larger limits.

Source-level follow-up: SmoothBoundary's active-mobility search enumerates a
metric mean plus18 axis proposals; each MoveVertex re-prepares the same accepted
star/atlas and reconstructs geometry before FreshScore rejects non-improving
volume quality. A future operator change should reuse preparation for a fixed
star and screen projected volume candidates before full surface reconstruction,
using the same point associations/projection outcome. It must not reproject a
realized point and assume the proposal is idempotent. This is a finding only;
finish the connected phase/generation change and validate actual workflow before
adding further operator machinery.

M6 build48 TERMINAL1853.13s, unchanged pins. Native elapsed-time stop in metric
update; original CFD mesh/solution retained. Testdriver exit0 permits that bounded
diagnostic outcome, but wrapperFAIL1 correctly rejects missing planned four-window
observation_stop data. All nine known owned processes verified terminal before
any next source edit. First quality-entry snapshots are retained; manifests state
source_valid=false and exact_continuation=false, so structural decoding is not a
runtime restart/acceptance certificate.

A second3-second9Hz DWARF sample of the active rank gives usable caller evidence:
ValidateCoverage~70% inclusive, SplitAtlasEdges~57%, with repeated CheckSurface,
PrepareCurvedAtlas/RebuildTraceCoverage/MoveVertex paths. Only23 samples; fractions
are diagnostic, not a benchmark. Full data/reports saved, no debugger/SIGSTOP.
This identifies generic arrangement verification during boundary-improvement
proposals, rather than demonstrating a collective mismatch or a polynomial
positivity recursion. Do not pursue an unobserved prover defect from leaf symbols.
Another ownership follow-up: exact copies of accepted explicit descriptors in a
private overlay currently suppress certificate reuse. A changed effective
contract must revalidate; a bitwise identical copied contract need not. Assess
this with operator preparation/quality screening after the phase/generation
change, rather than adding arithmetic shortcuts.

Build49 phase/generation change PASS99.18s; actual mapped workflowFAIL13.85s,
82 edits/345 attempts,26 geometry-progress reports,1 quality stage and136
optimization trials. Geometry arithmetic67m is exhausted; subsequent refusals
also consume native logical4096. No budget was raised. Build50 revised obsolete
one-frontier handoff test and passed96.17s. Focused stage checks6/7 cases,
4439/4443 assertions: four failures are the old expectation that construction
must erase already-known height values. The updated test independently checks
fully retained heights and explicit unknown counts; final height/rollback gates
remain unchanged.

Build51 pending: exact copied accepted explicit descriptors can reuse their
certificate; changed or malformed descriptors still revalidate. Atlas preparation
now validates actual supplied traces, not only traces derived from the coverage,
and its reversal checks input intervals. Boundary smoothing first projects the
raw proposal with the same atlas routine and computes native volume-quality
upper bound. Height can only lower that score, so a non-improving bound can be
rejected before full surface reconstruction. Every surviving raw proposal still
uses full MoveVertex/geometry/contact/height/source/publication checks. Preview
contexts are destroyed before full reconstruction; no extra retained atlas cache
or simultaneous workspace is introduced. Differential tests compare previewed
points/bindings/tet quality with full moves across seams and repeated states.

## Current shared error-query correction

Build53 passed; atlas_checks_20261027T205356_3596433 PASS52.007s with
unchanged pins. Mapped runtime_20261027T205308_3593869 FAIL11.87s, 201/203
assertions: curved flow exhausted 67,207,168 arithmetic work after 60 private
accepted edits/74 attempts and no quality stage; CFD state retained. Skipping
proof changes the exhaustion location, so the numerical difference is not a
causal comparison. Full mesh and transfer/restart remain unqualified.

Read-only review found the manual error-kernel duplication differed in binding,
ID and transient semantics. The pending correction routes already-validated
faces through shared PlanarReference::SurfaceErrorForValidatedFace and unchanged
ParameterError, with checked legacy fallback. Engine callers were audited for
accepted or successfully validated surfaces; differential tests compare exact
errors for original faces, probe children and explicit coverage. Build54 is live
(session3213, wrapper3599958); 1293 source pins remain frozen until terminal.

## Connected publication and archive validation

Build56 PASS36.145s. The corrected connected test passed event 0 move,
publication, schema8 reload and native source-envelope bounds, then failed at
event 1 in EncodeReference self-validation via DecodeReference, before explicit load: 235/236 assertions, 9.997s. Event 0 used
23,566,791 operator, 6,714,807 publication and 259,003 source work; 12 face
pieces, 13 traces, 56 bits, 10,728-byte payload, 21,804-byte resident state and
1.86475s. A nonstopping trace reproduced the decode-stage storage failure.
BoundaryAtlasBudget now uses the caller-provided storage ceiling instead of an
underestimated archive scratch cap; arithmetic limits and all certificates stay
unchanged. Build57 subsequently completed; its result follows.
Proof and full-pipeline cost remain unqualified; no accepted M6 or CFD transfer. Unless
otherwise indicated, these are private diagnostic observations.

Build57 PASS75.17s; its connected test failed again because the outer decoder
passed its preliminary estimate as a hard ceiling (requested 5,977,496 vs
5,866,824 bytes). Build58 PASS54.12s corrects the full decode/load admission chain:
internal decode reports attempted peak against the configured caller ceiling;
collective load accounts for retained caller and transport bytes and elects resource
refusal while malformed archives remain fatal. The public DecodeReference wrapper
preserves its invalid_argument API. Arithmetic limits and certificates are unchanged.

Build58 serial connected test PASS66.929s, 3488 assertions, 16 events; verified
publication_cost_summary records 12–20 covers, 13–19 traces, 56–109 bits,
30.54–67.21m work excluding archive arithmetic and 12.8–18.5 MB archive load peak
under the unchanged 2 GB ceiling. MPI6 completed all 16 operations, publication,
load and source checks but FAIL76.955s on strict phase peak bounds by a few hundred
bytes. Directory home mixes the ID hash before modulo; changing IDs altered owner balance rather
than establishing history growth. Build59 holds new node/cell ID residues constant
by world-size stride and adds one rank-0 deep-memory refusal at the prior measured
peak minus one with unchanged state. It also selects existing NativeReference3D
coverage in broad and reference_io1/6 cases. Build59/session61062 subsequently completed. The local index/closure remains open and unmerged; full M6 and CFD transfer
remain unqualified.


## Current MPI publication and M6 run

Source61 overlap_publication6_20261027T215151_3758648 PASS74.923s, all 16
events: rank 0 passed 3494 assertions, each other rank 3445; all nine recorded
processes are gone. publication_cost_summary.json verifies aggregate admitted
workspace and phase descriptor/work bounds, retaining per-rank statistics.
Archive arithmetic is excluded; source-envelope checks are not CFD transfer.

Full-CFD M6 build61/session99828 TERMINAL REJECT after 1864.11s: solver exit 0,
native event accepted=false, pins unchanged. The working mesh grew from 303,359 to 1,376,207 private cells (4.54×), with
877,469 metric demands (no requested BL in this run); quality was not reached. The 146,862 source-envelope
cells are separate source metadata, not the initial working tetrahedron count. All nine owned processes are gone. Largest sampled process-tree aggregate
RSS was 6,387,077,120 bytes; largest sampled individual RSS 1,096,867,840 bytes;
neither is a whole-job peak. Evidence: `staged57/m6_runs/euler_build61_working_surface`.
No accepted mesh or transfer/restart workflow follows.

Source61 aggregate admitted workspace comparisons pass; they do not establish physical whole-job peak memory.


Build61's final engine summaries report 755.685–755.727s/rank in contact work
out of about1729.85s/rank: 815,117,840 all-rank candidate visits and 59,287,969
checks. Directory elected1,629,519 times/rank; its526.194–584.781s nested time
must not be added to contact time. These are visits, not unique intersections.
This supports a contact/coordination cost concern, not an arithmetic-only cause.
Exact per-rank receipts and the unimplemented scheduling design are in
[metric-length satisfaction](METRIC_LENGTH_SATISFACTION_DESIGN.md).

## Source63–64 checkpoint

Source63 normalization passes serial and MPI6, and its 16-event MPI publication/
source-envelope/schema8 test passes with bounded reported phase costs. Broad serial
and MPI both fail on curved runtime and a stale quality-query fixture expectation.
Source64 corrects the fixture with row/version/content assertions; build64 and the
focused serial/MPI6 quality-cache checks pass. The broad failures remain; no full
M6 or curved CFD transfer is accepted. The bidirectional metric-length candidate is under build66 and unvalidated; see [design](METRIC_LENGTH_SATISFACTION_DESIGN.md). Details and receipts are in [handoff](../HANDOFF_Codex.md).

## Source69 mapped work trace

`staged57/cases/mapped_work_trace_20261028T010235_79656` terminated with the
curved-runtime failure (10.815s, unchanged pins). The two curved Euler/RANS
segments have identical counts/work; flat segments emit no geometry-work records.
Per curved segment, 31,222 records total 63,675,209 exclusive work. Dominant
operations are CoverageTraces 3,456 calls/16,005,528 exclusive; BindingParameters
17,977/15,197,296; CompareTraces 2,592/11,938,488; ValidateCoverage
864/8,555,428 exclusive (25,882,075 inclusive); ResolveSingleChart
5,129/7,772,505 exclusive (21,135,051 inclusive); SurfaceError 1,189/3,860,090.
Inclusive counts nest, so do not sum inclusive work. The trace sum covers emitted
scopes and is not the arithmetic limit itself. Both raw private manifests report
generation 15, 31 owned cells and 3,888 parameter bytes. Existing parameter
decoder verifies schema8, 22 points, 40 faces and a 960-byte appendix; the
WriteAtlas header reports zero explicit faces and zero explicit traces in each.
Thus these captures show repeated proof/retry work without retained explicit
descriptor history; they do not prove a universal history bound. The case-local
`parameter_descriptor_summary.json` records hashes and policy counts. These captures are not accepted meshes. M6 source69 `euler_build69_incident_collapse` terminated rejected after 1876.115s (solver exit 0, pins unchanged; all nine owned processes gone). Each rank reports zero completed length split/collapse passes yet one quality stage: a stagnation-window branch entered quality while pass-deferral scans changed neither attempts nor mesh progress. There were 289,853 directory elections per rank and 66,365 deferred candidates summed across ranks. Sampled process-tree maximum RSS was 3,757,772,800 bytes, not an allocation peak. The initial working mesh had 303,359 cells; 146,862 is source-envelope metadata. The next connected MIS bulk-retirement and lifecycle-gate design addresses this without weakening quality gates or caps. No M6 acceptance or transfer follows.

## Source72 rejection trace

Build72 PASS111.187s; mapped work trace72 FAIL10.834s with unchanged pins. Each curved variant has 136 rejected trials: 123 geometry-error gate, 12 degenerate/reversed single-chart and one arithmetic cap; 36 directed features. Feature (1,6496) recurs eight times with old-cell counts 4, 5 and 6, so feature identity alone cannot establish unchanged support or justify a blind cache. See the hash-backed `staged57/cases/mapped_work_trace_20261028T015054_207288/rejection_summary.json`. Audit72 is running and M6 pair-lifecycle is queued; no M6 status receipt exists yet.

## Source72 M6 and source73 candidate

M6 build72 `euler_build72_pair_lifecycle` TERMINAL REJECT1862.171s, solver exit 0, pins unchanged and nine owned processes gone. It completed two split passes and one collapse pass, with 6,287 splits, 2,025 collapses, zero quality stages and 44,861 geometry rejections; atlas arithmetic summed 369,648,840,116 across ranks. First split/collapse occurred at 117.756/1447.32s. Sampled process-tree RSS maximum 3,529,699,328 bytes is not allocation peak. Hash-backed `m6_checkpoint_summary.json` preserves rank counts and inputs; no accepted mesh or transfer follows.

Source73 independent mathematical review found no concrete blocker. Build73 PASS141.2348s, 1293 pins unchanged and all build processes gone; atlas_checks73 passed. The rejection-only original-point versus complete proposed boundary screen uses existing SAT. In shared ValidateCoverage, CoverageBoundaryEdges cancels conforming active interiors after original-path agreement and exactly splits residuals; positive pieces, path keys, signed unit boundary and original-adjacency embedding disk remain checked. Publication/export/load and curved RebuildTraceCoverage share this validator; source consumes validated state. Tests cover hanging fans, broken paths, holes/double cover, bounded grid work, low memory, and SAT margin/union behavior. Full curved workflow and M6 remain open; no caps or persistence format changed.

## Source73 connected checks

Source73 atlas checks PASS 65 cases/18,668 assertions in 127.973s, pins unchanged. Mapped runtime `staged57/cases/mapped_runtime_20261028T024037_335719` FAIL12.873s: curved variants reached 17 splits, no collapses, 18 split and 18 collapse passes, one quality stage and the unchanged work 4096/arithmetic 67,207,168 limits; flat completed 31 edits (27 splits/4 collapses). Full curved workflow remains unqualified. Source74 operator-reason diagnostics are pending; no new M6 run until construction cause is understood.

## Source74 architecture audit and pending validation

Current-source recurrence audit: prescribed trace maps are const inputs to reconstruction, and publication stores those prescribed paths rather than microtriangle knots. No surviving shared-edge feedback was identified. Entry normalization already replaces legacy paths with incident-owner closure. Trace73 shows productive split passes through split17; the following zero-success collapse pass consumes 8,160,949 arithmetic work before the next split reaches the cap. Existing interior surface bindings disprove edge-only sampling. Final fallback reasons at exhaustion do not establish geometric infeasibility. Next decisive check: replay demanded edge16–6496 with fresh work and preserve the first operator reason before choosing a construction or scheduling change. Hash-backed diagnosis: `staged57/cases/mapped_work_trace_20261028T024728_354109/construction_diagnosis.json`; analytic child/error estimates are not an operator certificate.

Build74 passes132.203s with unchanged pins. The combined clean/legacy 16-event regression compiles. The cost-receipt helper self-check passes old-format receipts and complete two-variant receipts and rejects a missing variant or phase cost growth. MPI runtime and compiled filter audit are blocked by approval-service initialization storage failure; neither is marked passed.

## Serial final-state operator replay and typed budget-stop candidate

Serial final-state edge proposal `staged57/serial_replay74` now compiled, linked and executed without MPI initialization: SplitEdge16–6496 succeeds (2→4 positive cells), error bound0.00375→0.00125, area-weighted squared excess9.4542612e-7→3.9078593e-9; operator work1,528,800 and total with error queries1,653,301 under original67,207,168 work and16MiB scratch. All recorded inputs/static libraries unchanged. Sampled compile/link/run process-tree RSS maximum490,385,408 bytes is not allocation peak. This is a fresh proposal on final captured private state, not the exact earlier transaction, whole-mesh acceptance, publication or solution transfer. It establishes that useful refinement remains possible; no general construction impossibility follows from exhaustion.

Build75 PASS1303.335s and corrected build76 PASS216.293s, pins unchanged. Serial replay76 PASS14.127s: a final-private-state edge proposal produces 2→4 positive cells, 1,528,800 operator work and geometry excess 9.454e-7→3.907e-9 with exhaustion false. A 100,000-work run refuses with zero output and exhaustion true; sticky multi-charge, divide-by-zero and NaN backend checks pass. This is not whole-mesh acceptance. Compiled filter audit76 failed before execution because approval-service initialization reported no space; collective-stop MPI and coupled legacy/32-event checks remain pending. All three owned handles are terminal; no jobs are running. The tracked-code WIP checkpoint is saved locally. Push was attempted but failed before execution because the approval service could not initialize; raw receipts and the serial harness remain local.

The connected candidate adds a sticky work-denial state to the existing shared AtlasBudget and charges the existing arithmetic work helpers through one admission method. Exceptions may still become bool/reason, but the typed state survives to collective engine handling. No work/memory/precision ceiling changes, operator alternatives, quality gates or persistence-format changes. Build75 passed; build76 passed after adding a final collective publication-refusal check, preserving a co-occurring original failure diagnostic, restricting delegated failure classification, adding the NaN guard, and limiting Dirichlet work marking to finite NoConvergence under a budget-limited iteration cap. Engine/MPI regression execution remains pending; the candidate does not eliminate repeated futile coarsening or qualify a full CFD workflow.
