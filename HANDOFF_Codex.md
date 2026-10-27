# Codex handoff — current state, 2026-10-27

Continue DC-17/DC-18 and ensuing defects. Both remain open. Maintain the local
[TODO](ADAPTATION_TODO.md), tracked [progress](ADAPTATION_PROGRESS.md), and
[transfer-only ledger](Handbook/TRANSFER_OPERATORS_FINDINGS.md). Follow
[AGENTS.md](AGENTS.md): capability-first, connected changes, one heavy job,
nice19, one build worker, single-thread libraries, at most6 MPI ranks.

## Goal and unchanged acceptance

Accepted full M6 Euler adaptation, then requested BL/RANS, conservative transfer,
BDF/history, publication and restart. q>=.20, J>=.05, edge<=1.8, requested h0±10%,
500k metric complexity (not tet count), prescribed geometry/body/features/contact
and source/transfer gates. ADAP_HAUSD is an absolute mesh-unit length independent
of h0. Construction measures no q/J; metric/BL-aware improvement follows. Boundary
nodes may move on the prescribed geometry. No accepted M6 workflow exists.

## Implemented connected direction

Latest pushed checkpoint `e72901f3bd` on authorized `rois1995/SU2`, branch
`codex/native-3d-bl`, explicitly incomplete. Fixed original geometry remains
separate from current working associations. Canonical original-star/strip paths
replace history-derived internal paths; one shared reference PSLG proves jointly
admissible incident faces, positive maps and exact region replacement. Changed
retained traces require all incident faces and owning volume-row version updates.
Eigen replaces custom PCG. Existing cubic backend exercises a small geometry
interface; optional CAD plan is STEP, then faceted STL and IGES. No new geometry
library dependency/importers. See [architecture](Handbook/WORKING_SURFACE_RECONSTRUCTION_DECISION.md).

Build47 additionally queues current geometry demand, leaving hypothetical child
feasibility to the selected complete star. Accepted-face validity is reused by
provenance across containment/export/reconstruction/publication; active load
validation and new faces remain complete. First mobility activation now certifies
legacy subfaces; exact original faces inherit the authority certificate. Pending
associations are identified by ownership rather than MAX-valued IDs. Build48
fixes ReverseTrace sanitizing malformed intervals before validation.

## Validation and actual remaining failure

Build47 PASS1210.22s; build48 PASS48.15s,1293 unchanged pins. Build48 focused
4 cases/116 assertions PASS9.88s; atlas_checks57 cases/8363 assertions PASS51.89s.
Compiled47 filter audit selects239serial/216MPI/57atlas, all intended cases and
separately scheduled wide workflows;48 changes no test source/helper/names.

Latest repeated-overlap receipt passes16 events:7 points/10 faces,10–16 covers,
10–15 traces,54–55 bits,3.97–8.00m arithmetic work/event,5.76–6.27MB admitted
peak,0.592–0.640s/event; returning phases satisfy existing bounds. This is private
surface-move scope, not full adaptation. See atlas_checks_20261027T192923_3373582/
reference_cost_summary.json under staged57.

Complete mapped workflow still FAILS unchanged67,207,168 arithmetic work:
49 private edits/346 attempts,17 geometry progress,292 optimization trials.
Selection's empty counters follow phase reset, not acceptance. Captured153 cells/
57 nodes/90 boundary faces/revision40 contain0 explicit covers/traces. Independent
production-kernel census finds5 q/J-bad cells and10 oversize cells in2 disjoint
regions,0 nonpositive orientations. Receipts and exact link/source provenance:
`staged57/cases/mapped_work_trace_20261027T192656_3366658/`.
[Current diagnosis](Handbook/GEOMETRY_VERIFICATION_COST.md) includes work attribution.

## Latest M6 and live owned build

M6 build48 session40090 TERMINAL1853.13s, wrapper3391763 and all9 known
owned processes verified gone. Case `staged57/cases/m6_stage_20261027T193554_3391623`.
Native elapsed-time stop during metric update; original CFD state retained.
Diagnostic test exit0 is allowed, but wrapperFAIL1 correctly refuses incomplete
four-quality-window campaign (missing observation_stop snapshot). Source pins
unchanged. No accepted mesh/transfer. First quality-entry snapshot:667,993 cells,
198,390 size-bad,7,090 geometry-bad,64,554 quality-invalid/1.17m repair features;
q/J measurements were0 in construction. Actual movement/swaps were observed,
but accepted boundary movement still needs snapshot comparison, not counters.
Five-second FP and three-second DWARF software perf sampling used no debugger/
SIGSTOP. DWARF identifies generic ValidateCoverage/SplitAtlasEdges and repeated
PrepareCurvedAtlas/CheckSurface/MoveVertex work, not an MPI mismatch. Complete
quality-entry snapshots are being structurally validated/archived by Terra;
source_valid=false/exact_continuation=false flags prohibit claiming runtime
checkpoint or acceptance. Archive job is terminal:6 shards structurally decoded/stream-hash verified,
667,993 cells/133,738 vertices/34,857 physical vertices,7 archives each<95MiB.
See private_quality_entry_archive_receipt.json; raw data retained.

Earlier build46 was deliberately interrupted908.19s to implement the connected
change, but was progressing:66,336 accepted private edits at its last window.
Its interruption note is corrected; absence of a mesh snapshot did not mean no
progress. Earlier build39's timed GDB left a rank stopped: instrumentation confound,
not evidence of an MPI mismatch. Never repeat timed debugger attachment.

## Current connected implementation — build50 running

Phase/generation preservation is applied in CNativeEngine3D.cpp. Build49 passed
99.18s/1293 unchanged pins. Mapped runtime FAIL13.85s unchanged limits:82 accepted/
345 attempts,26 geometry progress,1 quality stage,0 construction returns;
136 optimization trials. Geometry arithmetic67m is exhausted; subsequent
refusals consume logical work4096. Private cached demands still include36
geometry cells and57 metric/height cells; no accepted return/transfer.

Build50/session94342 TERMINAL PASS96.17s. Stage checks6/7 cases,4439/4443
assertions; four failures are obsolete construction height_measured==0 expectations.
Tests now independently verify retained current heights/unknown counts.
Build51/session91720, ninja3540771 is LIVE: copied explicit descriptor certificate
reuse, projected volume-quality screening before full boundary reconstruction,
actual trace override validation and differential preview/movement tests.
Pinned C++/helpers stay unchanged. Build50 test updated from the obsolete
“yield after one frontier” contract to productive descendant generations without
q/J measurement. Keep pinned C++/helpers unchanged until terminal. Then run
focused stage/coarsening/construction checks and compiled filter audit. Actual
mapped workflow is still the known failing capability case; do not raise limits.

The implementation preserves current rows/edge/geometry measurements through
handoff, refreshing only missing shape/requested-height measurements. It checks
owned versions, ordered IDs and endpoint/field certificates; preserves repair
support/age/rejection state and full rollback reset. Productive non-coarsening
finite generations reseed current demand before quality; a stalled generation
may hand off. Work/time/stagnation and all final gates remain. It is not yet
validated as an integrated change and does not establish convergence.

Next cost findings: boundary smoothing repeatedly prepares the same accepted
star for19 proposals and reconstructs before scoring volume quality; exact copied
accepted explicit descriptors also disable proof reuse. Generic arrangement
verification is the measured M6 hot path. Assess connected proof ownership/
operator preparation/quality screening, not arithmetic tweaks or budget growth.
Reuse existing facilities and preserve frozen donor/source/load trust boundaries.

After integrated focused checks: appropriate broad serial/MPI and separately
scheduled wider workflows, fresh M6, repeated adaptation/publication/transfer/
restart. Use staged57 build_checkpoint.py/run_cases.py/audit_compiled_filters.py
and resumable receipts. Primitive passes cannot replace accepted workflow.

Other open findings include legacy uncoordinated perimeter normalization,
changing-frame gradation21/9393 worst1.406, normal-only M6 profiles above500k,
whole-field normalization, curved BL support/chord/quadrature, FIRST_ROW geometry
demand, fitted seams/automatic initialization, and archive arithmetic allowances
omitting implicit current-face count. See linked ledgers; no blanket budget raise.

Detailed history: [curved implementation](Handbook/CURVED_ATLAS_IMPLEMENTATION.md),
[staged adaptation](Handbook/STAGED_NATIVE_ADAPTATION.md),
[curved mobility](Handbook/CURVED_BOUNDARY_MOBILITY.md),
[older validation](Handbook/VALIDATION_HISTORY_THROUGH_BUILD32.md).
