# Native adaptation progress

Companion to [ADAPTATION_TODO.md](ADAPTATION_TODO.md), the acceptance ledger.
Updated: 2026-10-28. Detailed failures and provenance remain linked below.

## Current result

Connected entity-geometry implementation is now in the source tree; integrated validation is pending. It adds a two-sided tolerance certificate, semantic body/sheet associations, feature-only canonical routes, physical-source tubes, local vertex-link/owner-closure publication checks and explicit reference/payload version9 migration. Curved production setup selects this contract; legacy1–8 remains decode/validation compatibility. Fixed cubic geometry and final acceptance gates remain authoritative. No accepted curved CFD/M6 adaptation/transfer/restart exists; DC-17/DC-18 remain open.

Connected WIP checkpoint b35d3f626e is pushed. Full build86 PASS1297.729s with unchanged pins; the actual curved CFD run exposed feature-arithmetic scratch admission based on the global integer ceiling rather than its bounded one-dimensional operands. The scoped fix and regression built in build87 PASS66.159s. Actual mapped workflow87 FAIL72.020s: flat contexts pass; curved contexts each perform53 splits/16 collapses, construction q/J measurements0, then exhaust unchanged arithmetic67,207,168 before an accepted mesh. Twenty valid Euler split proposals were rejected by inherited local geometry-progress scoring. The entity construction path now omits that refinement veto and the old chart-ownership collapse screen; final geometry/quality/transfer gates remain. Build88 caught setup-failure handling scope errors and is terminal; corrected build89 is running, one low-priority worker. Do not edit pinned C++/helpers until terminal. Next is the same curved workflow, connected MPI publication/restart and M6 early. Primitive union84 PASS9 cases/215 assertions and syntax86 PASS11/11 remain supporting evidence only.

Build89 PASS45.156s, pins unchanged. Compiled-filter audit passes23 selections:260 serial/237 MPI/71 atlas, including new entity-v9 and feature-normalization cases. Actual workflow89 FAIL55.134s with unchanged pins: flat variants pass; each curved variant38 splits/6 collapses,1459 rejections (1312 local geometry coarsening vetoes,147 entity-orientation refusals), arithmetic67,207,168 and no quality stage. Removing refinement scoring alone did not deliver an accepted mesh. IDs repeat with evolving volume stars; unchanged physical dependencies are not established. Four captured cells share .0010990956390742213, not every bad cell; symmetry and conservative bounds prevent inferring a plateau or actual distance from that number. Build90 groups productive generations of the same operator before switching split/collapse, using existing finite frontiers/reservations and unchanged gates. The regression now requires the first collapse sweep to follow refinement quiescence; the prior collapse-before-size-completion assertion was an agent-selected scheduling expectation, not a user requirement. Build90 running; sources/helpers pinned.

Build90 PASS189.258s, source pins unchanged. Workflow90 FAIL72.964s, unchanged pins: each curved variant reaches295 splits/157 collapses and zero cached size/geometry debt, but coarsening verification exhausts67,207,168 arithmetic before quality; Euler recorded2588 rejections/579 final private cells. No accepted mesh/transfer/restart follows. The rejected-state diagnostic itself exhausted the event reservation while evaluating face errors; its complete binary shard is saved, but the text diagnosis is partial. Diagnostics must use fresh independently admitted work. Sweep scheduling resolves the previous early construction stall but does not resolve repeated coarsening verification. No cap or acceptance gate changed. M6 build90 euler_build90_entity_sweeps is now the sole owned heavy job (session57795); do not edit pinned C++/helpers until terminal. Prior mapped handles/processes are verified terminal/gone; the first M6 launch review rejection used stale live-job state and was resolved with process evidence.

M6 build90 was deliberately stopped after481.694s (solver -15, pins unchanged), before operators. All owned host PIDs are verified gone. A three-second userspace profile (120 samples, no lost samples) places100% of sampled stacks in PrepareMobility→WithEntityGeometry→GeometryCertificates→QuerySurfaceUnion; source inspection shows unconditional dense model approximation for the reverse direction before union queries. The profile does not distinguish that phase from later subdivision. This is a diagnostic interruption, not a native rejection or infeasibility proof. Source enclosures need model→frozen-skin bounds, while final acceptance needs both directions. The connected correction exposes direction in the geometry backend, omits the dense model approximation for source-only queries, and forces an uncached complete two-sided final query. Source-only results never claim within_tolerance. No arithmetic cap or geometric target was raised.

The first mapped90 refusal also exposed that RebuildFeatureTraces rejected empty candidate physical surfaces even for empty interior cavities. The shared helper now permits genuine interior reconstruction while still refusing removal of a physical surface. New focused coverage tests that distinction. Existing neighboring-move facilities now cover four16-event variants: clean/historical legacy and clean/historical→entity, owner publication, frozen source envelopes, schema8/9 reload and descriptor/work/storage receipts. The summary requires every variant/event. Optional transaction observation now captures the first local failed operator with matching private geometry and independently admitted fresh-work replay; final rejected-state diagnostics no longer reuse the exhausted event reservation. All changes await connected build91 and actual workflow/M6 qualification.

Serial final-state edge proposal `staged57/serial_replay74` now compiled, linked and executed without MPI initialization: SplitEdge16–6496 succeeds (2→4 positive cells), error bound0.00375→0.00125, area-weighted squared excess9.4542612e-7→3.9078593e-9; operator work1,528,800 and total with error queries1,653,301 under original67,207,168 work and16MiB scratch. All recorded inputs/static libraries unchanged. Sampled compile/link/run process-tree RSS maximum490,385,408 bytes is not allocation peak. This is a fresh proposal on final captured private state, not the exact earlier transaction, whole-mesh acceptance, publication or solution transfer. It establishes that useful refinement remains possible; no general construction impossibility follows from exhaustion.

Current-source recurrence audit: prescribed trace maps are const inputs to reconstruction, and publication stores those prescribed paths rather than microtriangle knots. No surviving shared-edge feedback was identified. Entry normalization already replaces legacy paths with incident-owner closure. Trace73 shows productive split passes through split17; the following zero-success collapse pass consumes 8,160,949 arithmetic work before the next split reaches the cap. Existing interior surface bindings disprove edge-only sampling. Final fallback reasons at exhaustion do not establish geometric infeasibility. Next decisive check: replay demanded edge16–6496 with fresh work and preserve the first operator reason before choosing a construction or scheduling change. Hash-backed diagnosis: `staged57/cases/mapped_work_trace_20261028T024728_354109/construction_diagnosis.json`; analytic child/error estimates are not an operator certificate.

Build74 PASS132.203s with 1293 unchanged pins. The coupled regression now seeds a legacy shared path, normalizes and publishes all affected owner rows, then runs both clean/legacy variants through 16 actual neighboring moves, source-envelope reconstruction and schema8 reload. The cost helper requires both labelled variants and checks each phase independently. New runtime/filter checks remain PENDING: automatic approval review cannot initialize its session store (No space left on device); host/MPI/Git publication are unavailable. Sampled build process-tree RSS maximum 2,721,198,080 bytes is not allocation peak. Local review checkpoint is `staged57/review_checkpoint74.{patch,json}`; it is not a pushed Git commit.

The unbuilt source74 endpoint-curvature sizing proposal was removed after review: it was not a necessary Hausdorff veto. The current audit found no remaining trace-feedback recurrence; coordinated legacy normalization is implemented, while joint network recovery and full curved workflow qualification remain open. Source73 SAT and coverage changes remain; full workflow qualification is open.

Source73 atlas checks PASS 65 cases/18,668 assertions in 127.973s, pins unchanged. Mapped runtime `staged57/cases/mapped_runtime_20261028T024037_335719` FAIL12.873s: curved variants reached 17 splits, no collapses, 18 split and 18 collapse passes, one quality stage and the unchanged work 4096/arithmetic 67,207,168 limits; flat completed 31 edits (27 splits/4 collapses). Full curved workflow remains unqualified. The next integration check combines legacy normalization with repeated neighboring edits; no new M6 run until the construction cause is understood.

M6 build72 `euler_build72_pair_lifecycle` TERMINAL REJECT1862.171s, solver exit 0, pins unchanged and nine owned processes gone. It completed two split passes and one collapse pass, with 6,287 splits, 2,025 collapses, zero quality stages and 44,861 geometry rejections; atlas arithmetic summed 369,648,840,116 across ranks. First split/collapse occurred at 117.756/1447.32s. Sampled process-tree RSS maximum 3,529,699,328 bytes is not allocation peak. Hash-backed `m6_checkpoint_summary.json` preserves rank counts and inputs; no accepted mesh or transfer follows.

Source73 independent mathematical review found no concrete blocker. Build73 PASS141.2348s, 1293 pins unchanged and all build processes gone; atlas_checks73 passed as above. The rejection-only original-point versus complete proposed boundary screen uses existing SAT. In shared ValidateCoverage, CoverageBoundaryEdges cancels conforming active interiors after original-path agreement and exactly splits residuals; positive pieces, path keys, signed unit boundary and original-adjacency embedding disk remain checked. Publication/export/load and curved RebuildTraceCoverage share this validator; source consumes validated state. Tests cover hanging fans, broken paths, holes/double cover, bounded grid work, low memory, and SAT margin/union behavior. Full curved workflow and M6 remain open; no caps or persistence format changed.

Build71 PASS99.169s, pins unchanged and all build processes gone. Bidirectional serial passes 3 cases/558 assertions in 5.804s; MPI6 passes 3 cases per rank in 5.785s (466/118/118/70/70/70 assertions). Both plain and composed tight-window variants cover input immutability and final gates. Controlled plain fixture comparison (source71 uses stagnation window 1; configurations differ): 60 accepted cells, 19 splits, 2 collapses; admitted work fell from 1556 (build67) to 21 and directory elections from 38603 to 610. Deferral counter semantics changed, so raw counts are not comparable. Mapped runtime71 `staged57/cases/mapped_runtime_20261028T014634_194809` FAIL10.944s: flat completes at work 31; curved reaches work 150, 14 accepted and 136 rejected before the same arithmetic cap, without quality. Trace72 identified rejection reasons; M6 build72 terminated rejected, as above.
ServiceClaimedVertex retires measured local frontier directions using existing
ChangedVertices notifications, without new cache, state or communication; Directory
still guards unmeasured, remote and expanded support. Deferred-frontier progress
prevents stagnation fallthrough; both legacy generic/stagnation quality handoffs
are disabled for bidirectional mode, leaving pair quiescence as the construction-to-
quality handoff. A tight-window (1) plain/composed fixture requires both pass counts
before quality. Review found no blocker. `length_pass_deferred` now counts selected
deferrals plus bulk-retired potential directions, including balanced edges; raw
counts cannot be compared as equivalent work to earlier runs.

Build69 PASS1198.313s, 1293 pins unchanged; all owned build processes exited.
Audit passes 23 selections (atlas 63, serial 252, MPI 229). Atlas checks pass
63 cases/12,847 assertions in 132.018s, including actual smooth/curve/unpublished
endpoint collapses. Mapped runtime FAIL10.806s: curved flows reached 14 splits,
zero collapses, 136 rejections/274 geometry trials and the unchanged 67,207,168
arithmetic cap (global work 744); 153 early constraint filters and 19 orientation
vetoes in 420 checks. Flat workflow passes with 31 edits (27 splits, 4 collapses;
work 282). The connected construction blocker and accepted M6 remain open.
M6 source69 `euler_build69_incident_collapse` terminated rejected after 1876.115s (solver exit 0, pins unchanged; all nine owned processes gone). Each rank reports zero completed length split/collapse passes yet one quality stage: a stagnation-window branch entered quality while pass-deferral scans changed neither attempts nor mesh progress. There were 289,853 directory elections per rank and 66,365 deferred candidates summed across ranks. Sampled process-tree maximum RSS was 3,757,772,800 bytes, not an allocation peak. The initial working mesh had 303,359 cells; 146,862 is source-envelope metadata. The next connected MIS bulk-retirement and lifecycle-gate design addresses this without weakening quality gates or caps. No M6 acceptance or transfer follows.


Build67 PASS99.196s with unchanged pins. Focused bidirectional serial passes
3 cases/300 assertions in 5.934s; MPI6 passes 3 cases per rank in 6.774s
(`staged57/cases/bidirectional_20261028T001524_4140966` and
`bidirectional6_20261028T001541_4141896`). Mapped runtime
`staged57/cases/mapped_runtime_20261028T001819_4149269` FAIL12.774s,
195/197 assertions: curved flows reached 13 splits, zero collapses and the
unchanged arithmetic guard; flat workflow passes. No accepted M6 results.
Smooth-seam collapse integration is in progress; see
[metric-length design](Handbook/METRIC_LENGTH_SATISFACTION_DESIGN.md).


**DC-17 and DC-18 remain open. No full M6 adaptation has passed final acceptance.**

Build63 PASS66.1366s; all 17 build PIDs gone. Nineteen compiled filter audits
pass (245 serial/222 MPI selected). Normalization passes serial 745 assertions
in 15.9078s and MPI6 in 16.796s (727/721/721/721/715/715 assertions); all nine
MPI PIDs gone. Repeated 16-event MPI publication/source-bound/schema8 test passes
75.9713s; all nine PIDs gone. Its cost receipt records 12–20 face pieces, 13–19
traces, 56–109 bits, 30,540,601–67,213,767 work excluding decode and 10,058,008–
46,571,848 bytes aggregate admitted workspace. Sampled process-tree RSS ~214 MB
is not peak allocation. Source-envelope checks are not CFD solution transfer.

Broad serial `staged57/cases/serial_full_20261027T230545_3953311` FAIL176.0948s:
243/245 cases and 1,134,704/1,134,707 assertions pass. Two failures are the known
curved runtime; the third is a stale quality-stage query expectation (6 vs 12).
Broad MPI63 `staged57/cases/mpi6_20261027T231001_3964969` FAIL177.1677s
with unchanged pins: each rank passed 220/222 cases, with the same three failed
assertions; the empty-rank fixture passed and all nine processes exited. Source64
corrects the stale fixture expectation and strengthens row/version/content checks.
Build64 PASS96.1853s; focused quality_cache serial and MPI6 pass in 6.8561s and
6.8769s respectively, all owned processes gone. All 21 compiled filter selections pass in build64. These focused passes do not qualify the failed
broad campaigns. No complete M6 or CFD transfer/restart is accepted.
Source61 build PASS36.124s; source60 reference I/O PASS 5 cases/96 assertions.
Source61 broad MPI aborted exit134 after 24/25 cases per rank: fixture point-0 read
on an empty rank at CNativeImport3D_tests.cpp:1743. The nonempty-rank
broadcast fix passed in the source63 broad MPI run.
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

Source60 per-rank bounds failed; Directory hashes IDs before modulo, so stride
alone did not fix home balance. Source61 aggregate workspace bounds pass. Local
index/closure and full M6/CFD transfer remain open; geometry-validation checkpoint 9ae6b94886.


Build54 PASS1225.30s, 1293 pins unchanged; single-certificate checks PASS
4 cases/131 assertions in 11.873s. Mapped runtime FAIL13.858s, 201/203
assertions: each curved flow reached 88 private edits/317 attempts, 29 geometry
progress, one quality stage and 85 optimization trials before the unchanged
67,207,168 work cap. Flat flows accepted and exercised transfer.

Build55 PASS36.14s. The new neighboring-volume test connects 16 actual moves,
directory lookup/fetch/reservation and owner publication, schema8 decode
continuation and native source-envelope bounds; this does not exercise CFD
transfer. Its first run failed at event 0 after 21 checks because the test
omitted old-patch parameter export; production GeometryPatch already exports.
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


The local support-index draft is not merged; the global normalizer was rejected
for locality and accounting. Latest pushed checkpoint: 9ae6b94886.

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
errors for original faces, probe children and explicit coverage. Build54 subsequently completed; current results follow.


Construction is validity-first and q/J-free; metric/BL-aware improvement follows,
with all final q/J/size/height/geometry/transfer gates unchanged. The connected
coarsening closure, large-star movement, clipping correction and collision paging
fix passed build17 integration: serial179 cases/1,121,211 assertions (61.68s),
MPI6 all156 cases/rank (43.79s), unchanged pins. These include curved Euler/RANS
transfer/reload, repeated curved restart, and distributed history/recovery.
[Transfer-only ledger](Handbook/TRANSFER_OPERATORS_FINDINGS.md).


Build41–45 diagnosis: the captured failing mapped runtime has 88 cells, 30 boundary
points and 56 faces with zero explicit trace/cover descriptors. Build44 still failed
after 30 private edits and 42 attempts with eight geometry cells and no queued geometry
edges. Build45 recorded 457 projected-edge probes: 133 invalid facet-interior
bindings at boundary points and 324 parameter-authority refusals; all threw before
progress scoring. Build46 subsequently completed with shared query corrections. See [geometry verification cost](Handbook/GEOMETRY_VERIFICATION_COST.md) for receipts and unchanged gates.
No complete accepted mesh or transfer/restart qualification follows.

Build32/session33069 PASS78.14s/950 unchanged pins. Clean working-reference
reconstruction, coordinated shared paths and the native backend interface now
pass focused atlas_checks/session40070:45 cases/7561 assertions,38.79s,
unchanged pins. Actual two-event movement,16-event reconstruction with stable
normalized descriptors, nonseparating features, region history/holes, metadata
publication, native archive and source checks pass. Compiled-binary listing
verifies all14 intended new-file cases selected (45 total). Earlier test-tag
omissions and all failures remain documented; those earlier passes did not
qualify the omitted remapper cases.
Six-rank M6 build32/session90654 TERMINAL FAIL54.3039s, pins unchanged; largest sampled individual RSS392314880 bytes. Setup work174193 passed, but import rejected “Native3D physical triangle leaves its immutable oriented marker facets.” before meshing. Evidence: `staged57/cases/m6_stage_20261027T144634_2539145`. Full accepted mesh, broad serial/MPI, adaptation, transfer and restart remain unqualified.
The import failure is a missing reservation on read-only accepted-state face lookup.
Build33/session45827 PASS57.15s,950 unchanged pins. Focused atlas checks
PASS38.85s:45 cases/7610 assertions, including exact accepted lookup without a
reservation and rejection of altered coordinates/orientation/marker. M6 retry/session31910 TERMINAL REJECT190.56s with unchanged pins after814
private edits; unchanged trace-only metadata incorrectly invoked an empty
face-region proof. CFD state retained. Build34/session16636 PASS87.15s/950 unchanged pins. Expanded build34 serial_full/session69002 TERMINAL FAIL94.85s, pins unchanged: 226/227 cases and 1129578/1129580 assertions passed. Mapped native runtime curved Euler/RANS failed after 29 private edits at the unchanged atlas work budget 67207168. Accepted mesh, transfer and restart remain unqualified.

The fixed-reference compatibility probe now completes16 neighboring moves on both
a planar disk and a faceted M6 wing star with stable current capacities and
settled overlay counts. This qualifies representation/API scope only. Geometry
Central does not provide the required prescribed CFD graph recovery or movable
feature retirement through the inspected APIs; it is not selected as a complete
production network engine. The next connected candidate uses native shared-PSLG
reconstruction and a collective embedding certificate; independent canonical
edge proposals are never sufficient admission. See the
[architecture decision](Handbook/WORKING_SURFACE_RECONSTRUCTION_DECISION.md) and
[probe measurements](integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/reference_network_probe_v1/probe_summary.md).
No full accepted mesh, transfer or restart has been established.
Native build37 passed78.18s with1293 unchanged pins, including vendored Eigen.
Build38 passed111.93s/1293 unchanged pins. Its focused run failed8/53 cases
(7799/7807 assertions passed,24.94s, pins unchanged): region-boundary
comparison confused segmentation with geometry, and one new cubic fixture
swapped two controls. Build39 corrects both and the focused run passes. The connected candidate also adds owner closure:
metadata refresh must replace its owning volume rows so existing cell versions
and notifications invalidate dependent work. Donor-generation tracing found no
separate freeze/persistence mismatch. Build39 now passes the53-case focused run (8247 assertions,47.83s, unchanged
pins), including16 overlapping moves. Six-rank M6 build39 diagnostic was interrupted after a timed debugger left
one worker stopped; -15/1295.38s with unchanged pins, all descendants now gone.
The apparent collective skew is instrumentation-confounded, not a demonstrated
native defect. A fresh unattached M6 run is required. Build40 adds numeric event
receipts and resumable workflow selection; full adaptation/transfer/restart remains
unqualified.
Build40 cost_checks passes all16 moves with saved numeric per-event receipts:
7 points/10 faces,10–16 cover pieces,10–15 trace pieces,54–55 bits,
4.29–11.15m arithmetic work,5.76–6.27MB admitted peak,0.60–0.66s per move.
Returning phases meet the count/work/storage bounds. Broad serial build40 still
fails234/235 cases (1130211/1130213 assertions,102.73s, unchanged pins): curved
Euler/RANS mapped runtime reaches the identical67207168 work cap after29 private
edits. This prevents capability acceptance. Build41 adds diagnostic attribution
and raw rejection capture; no work limit or final gate is changed.

Build35 and build36 passed with 950 unchanged pins. Build36 serial_full/session68944 TERMINAL FAIL99.7804s, pins unchanged: 226/228 cases and 1129639/1129642 assertions passed; largest sampled individual RSS145432576 bytes. Evidence: `staged57/cases/serial_full_20261027T153436_2673844`. The overlapping-star test exhausted atlas work on event 5 (sixth move); curved Euler/RANS runtime again exhausted 67207168 work after 29 accepted private edits and 35 attempts. Retained path history is an architectural blockage not resolved by discarding interior cuts. This motivated the connected strategy above; accepted mesh, transfer and restart remain unqualified.


Build19 serial182 cases/1,121,344 assertions and MPI6 all159 cases/rank passed;
its four-window M6 diagnostic worsened early quality debt and did not establish
boundary freedom or convergence. This prompted the connected chart-mobility work.
[Stage comparison](Handbook/M6_QUALITY_STAGE_PROGRESS.md).

Build18/session62205 PASS726.79s/933 unchanged pins. Focused serial stage_checks/
session45262 PASS10.5101s/6 cases/4419 assertions; six-rank stage_checks6/session51405
PASS11.5294s. M6 stage capture/session11197 is TERMINAL PASS in623.327s: pins
unchanged, six complete rank shards, four quality windows and original donors
unchanged. Broad serial/session28247 and MPI6/session24105 pass. This validates quality-stage observations and
complete private-state capture; it does not establish full convergence, exact
continuation or final mesh acceptance. Focused checks compare observed/unobserved mesh,
work and field queries, independently check shape/height debt, and exercise
collective observer failure, memory admission and serialization. Then the exact
build13 frozen M6 field is captured through four quality-progress windows, with
per-rank original donors and matching boundary parameters. Python receipt checks
pass valid/empty shards and reject truncation, changed donors, missing workers and
mismatched rounds. The completed short capture shows mixed debt over only248
rounds; see [design and quantitative table](Handbook/M6_QUALITY_STAGE_PROGRESS.md).

The difficult M6 build13 run remains TERMINAL REJECT:1802.45s native,774,314 private
cells,275,115 cached-invalid cells and1,240,202 repair demands after112,024 private
publications. Coarsening never started. Solver exit0 was not adaptation success;
CFD state was retained. Its counters alone do not prove opposing quality debt,
boundary pins or parallel throughput caused rejection. The new capture addresses
that evidence gap before changing quality admission or coordination.

Latest pushed `17902da07b`: quality-staging WIP; build/focused checks pass, M6 diagnostic complete; broader serial/MPI integration passes.
`64953f3b69` preserves completed diagnostic receipts and enrichment diagnosis. `f27662eb31` preserves the
passing build17 integration receipts. Prior failures, fixes and limited witnesses
remain in [archived validation history](Handbook/VALIDATION_HISTORY_THROUGH_BUILD17.md)
and [staged implementation](Handbook/STAGED_NATIVE_ADAPTATION.md). Full M6,
curved chart mobility and the remaining metric/BL scope are still required.
External-SSD references are [archived locally](Handbook/ADAPTATION_REFERENCE_INPUTS.md).

## Checkpoints

| Checkpoint | Change | Validation status |
| --- | --- | --- |
| `1a5745fe3e` — WIP | Preserve concurrent rejection messages; complete M6 transaction replay and fixed-vertex obstruction | MPI2 diagnostic regression passes; both private repairs fail strict shape. Surface placement is test-only, full M6 remains unaccepted. |
| `fdb6ad2683` | Persisted curved import and original-source runtime queries | Small cubic Euler/RANS transfer and SU2/CGNS reload pass at this baseline. |
| `79bfba6f8f` | Reuse collision searches, retaining exact contact checks | Connected checks pass; full M6 validation41 rejects. |
| `e398807a2c` | Reactivate curved demands; admit compatible concurrent proposals | 105 serial cases, MPI2 and all-worker runtime pass; full M6 validation43 rejects. |
| `c02010b08d` — WIP | Connect validity-only construction and bounded point placement | Validation44/45 failures preserved; not an accepted capability. |
| `3deed0ca3c` — WIP | Bounded topology repair and weighted radial integration | Validation46 fixes integration; cubic runtime still fails. |
| `4933586627` | Charge actual optimizer work and refund unused reservations | Validation47 serial passes; later six-rank qualification exposes further failures. |
| `895ab7274a` | Handle empty initial FVM graph ranks | Four-point import passes on six ranks. Distributed repair failures remain. |
| `7113fd984d` — WIP | Couple bounded candidate repair to strict construction admission | Builds; validation50 fails runtime and a fixed-cell-count assertion. Corrected in validation51. |
| `b214ff210b` — WIP | Retain one candidate for bounded repair after ordinary construction | Validation51 serial and six-rank flat runtime pass; cubic six-rank runtime remains incomplete. |
| `2a214b1058` — WIP | Geometry-first distributed conflict priority | Serial and priority/conflict tests pass; six-rank cubic runtime retains two geometry violations. |
| `4ffe4e507d` — WIP | Coalesce replicated concurrent edge demands | One-budget election passes; curved refinement completes, but subsequent coarsening exhausts remaining work. |
| `e9263060cf` | Order distributed proposals by geometric benefit and metric length | Serial 127 cases, MPI6 118/rank and all-worker Euler/RANS transfer/reload pass. Full M6 rejects at its time limit. |

`72c4684264` preserves the repair-accounting evidence and links its checkpoint.
Documentation-only commits may follow an implementation checkpoint.

## Evidence and remaining failures

| Campaign | Result | Consequence |
| --- | --- | --- |
| Full M6 validation43, MPI2 | Rejected at1800s:26,567 private edits;218,099 cached invalid cells;238,305 intermediate shape refusals. | Scheduling alone does not complete construction. Original CFD state is retained. |
| Validation46 | 21/22 focused cases pass. All selected integration cases pass, including positive sphere weights and second moments within the65536-point cap. | Cubic Euler/RANS still exhausts work4096. |
| Validation47 | 12 focused cases/2,183 assertions; connected serial125 cases/1,082,524 assertions pass. | Actual work accounting fixes serial runtime. MPI6 then aborts on a four-point sphere import with empty graph ranks. |
| Validation48 | Empty-rank reproducer passes on all six ranks. Connected MPI passes114/116 cases per rank. | Cubic runtime exhausts4096 with3 active workers. With all six active, affine runtime stalls with1 bad cell and cubic runtime exhausts4096. A collapse-filter counter fixture also fails. |
| Validation49 diagnostic | Whole-mesh topology leaves the affine bad cell; one insertion/flip removes it. Cubic45 bad cells become34 with topology and88 with full interior repair. | Larger late repair alone is insufficient. Diagnostic meshes are not runtime acceptance. |
| Validation50 | Build passes; focused14/16 cases,2,227/2,233 assertions pass. | Early candidate optimization starves ordinary joint placement:69 calls/0 accepted. Valid star repair creates32 cells; its fixture assumes12. |
| Validation54 | Focused 4 cases/31 assertions and all-six-worker 11 cases per rank pass. | Flat and cubic Euler/RANS complete with transfer/reload; curved total work 1,975/4,096. Connected serial127 and MPI6 118 cases/rank pass; full M6 rejects at its time limit. |
| Validation53 | MPI6 passes 9/10 cases per rank, including one-budget shared-edge election. | Curved refinement completes at work 4,066; coarsening exhausts its remaining 30. No accepted workflow yet. |
| Validation52 | Serial 16 cases/2,532 assertions pass; MPI6 passes 8/9 cases per rank. Flat transfer/reload and priority checks pass. | Cubic improves to zero shape/size failures and two geometry failures; work limit remains unchanged. |
| Validation51 | Build and16 focused cases/2,532 assertions pass. All six flat runtime and collapse-filter checks pass; cubic runtime rejects. | Retain one candidate for bounded repair after ordinary construction. Preserve all final gates; replace the star fixture's count restriction with an explicit volume check. |

The connected implementation reuses swaps and interior insertion, retains candidate
boundary/parameter bindings, reserves separate new-node identities, and refunds
unused work immediately. The global work limits and final gates are unchanged.
The collapse-filter fixture keeps its known local witness on two ownership groups
while every MPI rank participates; its original filter and mesh assertions remain.

Validation53 reuses duplicate-edge election for concurrent refinement before
import and work charging. The one-budget shared-edge regression passes on all six ranks. Refinement now
completes every mesh gate at work 4,066, but coarsening exhausts the remaining 30
units. The rejected private mesh has 209 cells and zero shape/size/geometry
violations; transfer/reload is still not reached. The documented coarsening
completion contract remains in force. Next: carry the full serial demand ordering
through distributed conflicts, including geometric benefit and metric length.

The short M6 CPU sample shows substantial MPI collective waiting alongside
active candidate reconstruction and donor queries. This is a five-second sample,
not whole-event timing. The full counters show 76,739 shape refusals and only
115 validated optimizer calls out of 2,113. Preserve the failed evidence and
review the difficult-region construction before adding further local fallbacks.

## Next acceptance steps

1. Resolve the focused runtime failures; verify final geometry, quality, volume,
   solution transfer and reload.
2. Pass connected serial and six-rank MPI qualification, including all-worker
   concurrent curved transactions.
3. Reassess full M6 under the agreed constraints. Accepted Euler adaptation precedes
   the remaining RANS/BL integration and qualification.

Other open requirements: changing-frame gradation, M6 complexity normalization,
curved BL support/chord/quadrature consumers, FIRST_ROW surface-error demands,
fitted-plane seams and automatic curved initialization. The normal-only relaxation
contract remains in force while tangential relaxation outside the protected band
is an unanswered question.

## Detailed records

- [Current handoff](HANDOFF_Codex.md): latest validation and immediate next step.
- [Curved construction and failure history](Handbook/DC18_CURVED_SOURCE.md).
- [DC-17/DC-18 completion handbook](Handbook/DC17_DC18_COMPLETION_2026_10_25.md).
- [Validation47](integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/validation47/summary.json).
- [Validation48](integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/validation48/summary.json).
- [Validation49 diagnostic](integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/validation49/summary.json).
- [Validation50](integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/validation50/summary.json).
- [Validation54](integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/validation54/summary.json).
- [Validation53](integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/validation53/summary.json).
- [Validation52](integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/validation52/summary.json).
- [Validation51](integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/validation51/summary.json).

The user approved pushing checkpoints to `rois1995/SU2`, branch
`codex/native-3d-bl`, on 2026-10-26. Push succeeded through `e9263060cf` on
[the fork branch](https://github.com/rois1995/SU2/tree/codex/native-3d-bl).
Validation53 is checkpointed and pushed with its incomplete workflow result.
Validation54 passes focused, all-worker and connected serial/MPI checks. Its
implementation remains pinned for the pending full-M6 campaign.

Current complete-workflow cost diagnosis and shared certificate design: [geometry verification cost](Handbook/GEOMETRY_VERIFICATION_COST.md).

Build46 PASS90.19s, focused temporary-association/geometry-certificate checks PASS93 assertions/2 cases. Mapped replay FAIL13.98s:318/342 probes score,9 geometry-progress edits,22 accepted/30 attempts,31 queued edges at unchanged work67207168. Zero-queue defect fixed; complete generation still fails. Fresh six-rank M6/session32081 was subsequently interrupted without debugger. Compiled filters237serial/214MPI/55atlas include all required cases.

M6 build46 intentionally interrupted908.19s without debugger, unchanged pins; no stage receipt, all6 workers active, all9 descendants verified terminal. Setup/source receipts retained; no native-failure/acceptance inference. Build47/session71724 now builds the connected lazy geometry-demand scheduling and accepted certificate ownership change, including first-activation proof and pending-association provenance. No gates/budgets changed.

Build47 PASS1210.22s. Lazy current-demand queue reaches49 accepted edits/17 geometry progress, but actual mapped workflow FAIL12.96s atunchanged67m arithmeticwork; a phase-reset emptyselection is not finalacceptance. Fourfocusedchecks initially114/116assertions; trace reversal was normalizing malformedendpoints. Build48 PASS48.15s, validatesintervals before reversal; all4focusedtests PASS9.88s. Profile/rawstate153cells90boundaryfaces0explicitcovers/traces; repeated-edit/publication/source/archive checks followed. Fullworkflow/M6 remainopen.

M6 build48 terminal1853.13s/nativeelapsedstopduringmetricupdate, unchangedpins; diagnosticdriverexit0butwrapperFAILmissing4qualitywindows. Retainedqualityentry667,993privatecells shows198,390sizebad/7,090geometrybad/64,554qualityinvalid and1.17mrepairfeatures. Noacceptedmesh/transfer; allownedprocessesgone. Non-stoppingDWARFsample identifiesgenericValidateCoverage/SplitAtlasEdges and repeatedboundarymovepreparation. Nextconnectedphase/generationdraftstillunapplied; no budget/gate changes.

Phase/generation change applied: build49PASS99.18s/1293pins; mappedruntimeFAIL13.85s atunchangedlimits with82accepted/26geometryprogress/1qualitystage and136optimizationtrials. Arithmetic67m exhaustedbeforelogical4096; both recorded, noacceptedreturn. Build50/session94342 updatesobsolete construction-handofftest toproductive descendantgenerations/qJ-free construction. No gate/budgetchanges. M6entry archives hashverified7files<95MiB each, private/notexactcheckpoint.
