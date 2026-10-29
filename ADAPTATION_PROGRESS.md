# Native adaptation progress

Companion to [ADAPTATION_TODO.md](ADAPTATION_TODO.md), the acceptance ledger.
Updated: 2026-10-29. Detailed failures and provenance remain linked below.

## Current result

No complete curved CFD/M6 adaptation, transfer and restart workflow is accepted.
DC-17/DC-18 remain open. The selected foundation is SCOREC/MeshAdapt classified
cavities and transported nodal size fields inside SU2. Requirements, source-code
comparison, decisions and acceptance: [connected design](Handbook/ADAPTATION_ESTABLISHED_DESIGN.md).

Active target: accepted ONERA M6 Euler mesh adaptation and actual CFD → mesh →
CFD → mesh. The user explicitly authorized pushing every checkpoint; previous
checkpoints through77e5cd579d are pushed. Broader BL/RANS, history and restart
obligations remain; no complete curved/M6 workflow is accepted yet.

Full M6/build132 is terminal: first native event rejected at~1356s with exact
surface-union dyadic resolution exhausted, after158 private accepted collapses.
No adapted mesh or conservative transfer was accepted. The remaining attempt
was stopped after that known failure; source/runtime pins stayed unchanged.
Selection took731s and rank4 spent682s in nested collapse geometry. All cache
rows were complete; the selection-filter origin is inferred, not a captured stack.
Receipt: staged57/build132_m6_summary.json.

Build133 (93.226s,1295 pins unchanged) defers single-cell remapped-face deviation
to existing complete-cavity admission and strict final certificates. Its focused
new contract passes; one unchanged analytic test's stale last-rejection substring
failed (4/5 cases98/99). Build134 replaces that diagnostic assumption with the
actual round-budget stop/reason contract, retaining rejection/cache/input checks.

Full M6/build133 is terminal, unchanged pins: first event rejected at1801.84s,
`Native3D metric update exhausts elapsed-time budget`. It admitted319 private
collapses at work377, completed no length pass/quality stage, and accepted no mesh
or transfer. Selection fell to1.1s; update465s and other1117s dominate. Cache rows
are incomplete at the metric-update timeout. Two five-second low-frequency stack
samples show surface-union current-cell measurement and projected-plane setup;
DWARF decoding is partial, and the fixed-target-error-floor hypothesis is unproven.
The remaining old-mesh event was stopped after known failure; all handles are gone.
Receipt: staged57/build133_m6_summary.json.

The target evidence motivates conservative box pruning in shared planar geometry:
reject coordinate-separated point/edge/triangle queries before exact predicates,
including boundary-pair validation during projected reference setup. Touching boxes
retain exact checks; no tolerance, final gate, work/time limit or geometry changed.
Build134 passes162.397s with1295 pins unchanged. Focused hole/contact/filter
checks pass8 cases/190 assertions, including one-ULP outside/on/inside segments.
Full M6/build134 is terminal with unchanged pins: first event rejects metric-update
elapsed time after414 private collapses/work503, no length pass/quality stage or
published mesh/transfer. Selection1.66s,update474s,other1129s. The remaining event
on retained input was stopped; all owned handles are gone. Stack samples identify
current-cell measurement and planar setup; successful read-only debugger snapshot
identifies model preparation in complete-cavity GeometryGuidanceExcess and
nextafterl on the smallest long-double interval. Inputs are optimized out and the
brief debugger pause perturbs timing. Receipt: staged57/build134_m6_summary.json.

Build135 addresses repeated certification in that path: identical BoundPhysicalSurface
queries reuse certified bounds keyed by all supports, coordinates and tolerance on
one immutable authority. Variable keys share the existing admitted FaceMemo pool;
query workspace is admitted and failures are never cached. Final two-sided queries
remain mandatory. Exact zero/unit interval identities avoid artificial subnormal
rounding; all non-identity arithmetic retains outward enclosure and true underflow.
Build passes84.211s/pins unchanged. Focused24/25 cases703/704 assertions, with only
the old sheared-graph complete-section work exhaustion. New memo isolation/storage
checks pass. An unchanged-source extraction passes648 exact rational enclosure
checks, true underflow and nonfinite paths (probe corrected from C++11 to repo C++17).
No final gate, tolerance or work/time limit changed.

Full M6/build135 is terminal with unchanged pins, first event elapsed-time refusal:
760 private collapses/work965, no completed length pass or quality stage. Rank0
pending edges43024, update394s,other1330s, selection<1s. The remaining retained-mesh
event was stopped and all owned handles are gone. No mesh/transfer accepted.
Receipt: staged57/build135_m6_summary.json. A review found cached hits did not
respect sticky atlas denial or charge key work; engine collective checks and final
certification still prevented mesh acceptance after exhaustion.

Build136 fixes that API contract before lookup, charging the admitted key words.
Regression6 cases/123 assertions passes, including empty/sticky/overrun budgets,
support/body/tolerance/coordinate isolation and collapse contracts. Build passes
84.212s/pins unchanged. The interval block is byte-identical to135's648 exact-rational
checks; those proofs are reused. Existing sheared-graph complete-section work
exhaustion remains unqualified; no numerical final gate or target field changed.

User explicitly requested stopping build136 to implement reusable fixed-surface
representation. The owned run is terminal3202.123s, pins unchanged; no native event
result, geometric rejection or accepted mesh inferred. All handles are gone.
Receipt: staged57/build136_m6_summary.json. The latest perf sample catches
FinitePlaneReference preparation inside complete-cavity surface checks, while
other ranks wait in collectives.

The new path reuses the existing original control-hull index. Certified facet
approximations, finite projected footprints and local ADTs persist by immutable
facet/tolerance, independent of the candidate mesh. Queries filter exact requested
facets and pass their current AtlasOps explicitly; retained indices hold no expired
budget reference. Memory uses a pre-admitted geometry-size pool; query-local
storage handles an unretained entry under the same configured ceiling. Final
two-sided checks and requested geometry/q/J/size/height gates stay mandatory.

Build137 passes306.746s/pins unchanged. Different-triangle/overlapping-scope reuse
checks pass2 cases/32 assertions. Broader geometry/curved-publication campaign:
49/51 cases,17559/17574 assertions pass. Fourteen failures catch a two-byte cache
reservation increase after revision-text growth; one is the prior sheared-graph
work exhaustion. Reservation is now fixed from original curve count, with optional
admission against the configured storage ceiling. No assertion or acceptance gate relaxed.
Build138 passes123.302s; its undersized pool fails two reuse assertions. Build139
separately admits a geometry-sized quota at decode, preserving base construction
reservation; build passes1018.648s and reuse2 cases/34 assertions. Serial49/51:
20 stale fixed-peak assertions plus one prior sheared-graph exhaustion. The optional
cache can shrink with ceiling; test140 subtracts optional quota before mandatory
refusal, preserving failure and unchanged-state checks. Build140 passes84.250s.
MPI2 passes8 cases/rank,14937/14741 assertions,211.602s/pins unchanged. No production
change after139. Full two-event M6 build140 now running with5400s native/event,
14400s outer. Attempt euler_build140_prepared_surface/time2305296/mpirun2305298.
Server restart killed the Python harness, not solvers; recovery monitor2331422
verified pins/identity/CWD and resumes receipts without repeating work. Original
exec9413 no longer exists; use process identities and saved status/receipts.
The300s short profile shows PreparedSurfaceView distance-bound evaluation rather
than projected-reference preparation; no accepted mesh yet.
No M6 capability accepted; known sheared-graph work exhaustion remains recorded.

Historical build128 passes serial45/9611 and focused MPI6 five cases/rank. Its
residual size repair reaches a later complete-surface geometry blockage in the
smaller workflow; these old regression results are not build130 validation. The
[stop checkpoint](Handbook/ADAPTATION_CHECKPOINT_20261029_STOP.md) and
staged57/checkpoint128_euler_size_summary.json preserve earlier provenance.

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
