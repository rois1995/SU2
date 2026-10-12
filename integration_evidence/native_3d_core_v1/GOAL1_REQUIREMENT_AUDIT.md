# Goal 1 requirement audit — 2026-10-12

Repository: /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated.
Current implementation branch: codex/native-3d-transport-accounting.
Audit baseline checkpoint:7f76fd9a279cf384a37f8ae137dc4dc26212ce82.
Update: the current conservative donor prefilter is separately qualified in
BOX_FILTER_VALIDATION.md/json, with complete source/binary/audit pins.
It preserves exact mesh/state/work results and avoids2344837exact donor tests
in the matched size8 diagnostic. All remaining performance/calibration gates
below still apply; a single elapsed sample is not their completion.
Goal1 ACTIVE. Goal2 not activated. This audit retains the complete original Goal1
scope in NATIVE_3D_DEVELOPMENT_GOALS.md; it does not redefine it around passing tests.

| Requirement | Current evidence | Verdict / remaining proof |
|---|---|---|
| Opt-in tetrahedral backend, separate branch, unchanged2D/MMG | Current native3D factory/support gate; query_packing_build_v1 full68-object build; NativeBridge2D and serialMMG controls in packing MPI matrix; earlier2D reuse checkpoint retained | Implemented/tested within declared scope. Packing touches only3D donor code/header/test. AdapNoExt untouched. |
| Orientation/SPD/canonical IDs/incidence and frozen tetrahedral criteria | CNativePredicates3D/Metric3D, selected reference/mesh tests; packing97-case MPI1/2/4; q>=.20/J>=.05/L<=1.8 independent audits | Verified for tested faceted/manufactured envelope, not arbitrary industrial meshes. |
| Sensor-only original-P1, bounded reuse and actual-query composition | FrozenField caches <=2048 sensor samples and <=256 donors; Query composes at each point/cache hit; distributed original-P1/composition and query-footprint controls | Verified supported sensor-only remeshing and composition-hook behavior. Production3DBL/composed edge integration explicitly rejects; not certified as available. |
| Split/collapse/move/face-edge reconnection, complete dependencies and atomic publication | Full97-case selection includes all29 directory/donor/engine plus3partition declarations; seven exact distributed audits; stale/rank-veto/memory/duplicate/rollback cases | Verified targeted repairs and repeated cycles; finite operator envelope remains. No proof of arbitrary cavity escape. |
| Adaptable planar triangles, feature/marker junctions, coupled volume reconstruction | Planar/coplanar/repair/bridge suites and whole n4N4M2/n8 cycles; exact facet/domain/manifold/no-overlap audits | Verified declared planar faceted domains. Curved authority/CAD is Goal2. |
| Tetrahedral cost-weighted partition, M=N/M<N and return to CFD ranks | WorkWeights uses metric volume, six-edge refine/coarsen demand and q/J demand; two-constraint partition controls; actual N4M2 mesh/solver/current+BDF replacement | Implemented and correctness-tested. Weight coefficients are a static proxy explicitly marked shortcut; actual rank-cost prediction/calibration remains open. |
| Manufactured aspect/size/direction matrix, first failure and residuals | Current packing cube/rotatedthin/non-dyadic/full driver checks; n8 exact18artifact equivalence; historical thinH10/H20/aspect2 source-pinned audits | Bounded evidence preserved. Current production512 limit remains identified; historic broader matrices belong to earlier pins, not automatically rerun/current-source proof. |
| SU2/CGNS writing/reload, current+BDF transfer, accepted-state preservation | Packing bridge and n4 fresh independent audits, n8 fresh actual outputs/exact states; six actual metric/Hessian/CFD/BDF controls | Verified declared paths. Historical persistent-restart checks remain separately pinned; no full physical unsteady or force accuracy claim. |
| Metric/Hessian/gradation/complexity, graph/ParMETIS/migration, query/cavity/commit/return/rebuild/transfer accounting | Native runtime/engine/partition/driver wall and CPU scopes, current counters, independent closure checks; actual lifecycle and query subcosts | Instrumentation closes on tested paths. Frozen sensors are distinct from actual CFD metric preparation. Optional/unexecuted stages and rejected3DBL need no fabricated timings. |
| Cache/accepted/rejected/communication and per-rank work/imbalance | Current logs; immutable payload/collective/work comparison; core/comparison parsers | Counts verified. Tracked bytes include self and exclude vendor/direct MPI. MPI API elapsed and CPU/elapsed difference are not pure wait/wire. Need cluster rank-cost calibration. |
| Peak RSS/admitted buffers, migration/private scratch and practical memory | Saturated admission/atomic refusal tests; runtime native_bound/process_peak_rss | Requested-storage bounds and lifetime RSS observed, not a calibrated allocator bound or practical large-size/rank envelope. |
| Repeated timing, useful accepted work/sec, detail/profiler overhead, size/M/rank limits | Closed reusable SGE packages, fixed-source pairs, three paired repetitions and OFF/ON controls prepared;31core/13comparison checks | NOT achieved. No actual new cluster comparison submitted/results available. Local single samples are diagnostics. |
| No unexplained unbounded scan/growth/retry | Donor ADT broad-phase candidate counts and exact filtering identified as dominant;512 transaction limit and1024diagnostic outcomes preserved | Known broad boxes can admit many original donors; Mask packing reduces bytes. The qualified projected-box filter avoids exact tetrahedron tests while retaining the same ADT candidate and final donor counts. Larger practical costs/scaling still need repeated measurement. |
| Evidence/handoff/push and compact closed dependencies | Owned receipts/source/meshes/states/failures; strict four-file pins and closed regular-file cluster bundles; current pushed checkpoint | Retained. No cluster requests while user unavailable; no local performance campaigns. |

The remaining performance measurements are required before Goal1 completion.
The size8 production512 failure is a work-budget limit: current trajectory needs
more accepted/rejected transactions, while exact1024diagnostic feasibility is
proved. It is not266 invalid cells and is not fixed by payload packing. Any
algorithmic change must preserve the declared metric/geometry/height/state/cap
contracts rather than relabel the diagnostic budget as production success.

Available independent work: deeper donor/cavity cost diagnosis within the preserved
contracts, stronger non-reconstructible-cavity
replay/diagnosis if a genuine failure is found, and completion of missing
calibration tooling. Goal2 cannot start while the complete performance gate lacks
proof. R<1/R<=.20 physical-CFD affordability remains later physical workload work,
not a manufactured frozen-grid claim.

## Subsequent offline coarsening diagnosis

collapse_bound_replay_v1/REVIEW.md verifies940historical full-star proposals against
the retained source-matched trace, including709exact accepted topology matches.
Every proposal removes cells, so216combined no-removal/oversized rejections mean
oversized changed edges. Exact original-P1 directional bounds identify76and47
doomed proposals in epochs0and1;12scalar component bounds identify35and24with no
accepted false rejection. Five second-epoch shape failures have provably length-safe
changed edges. The weaker envelope does not establish512completion or MPI payback
and is not introduced into production. No new actual MPI/CFD or performance result;
current qualified box-prefilter implementation and all open gates above remain.

## Current stronger whole-case qualification

BOX_FILTER_ENVELOPE_VALIDATION.md/json supplies fresh current-binary whole-case
proof for aspect2 turning-direction size4 wedges on N1M1 and N4M2, plus H20
thin N4M2. All three actual three-event replacement campaigns pass complete
independent geometry/metric/current+BDF/SU2CGNS audits. Twenty-four geometry/donor
frames and nine state files match old controls exactly; eight normalized H20
frames match the current unscaled control. Transported metrics are exact and
the box filter is exercised. This closes the current-source proof gap for these
three stronger cases; H10/serialH20/persistent-restart historical matrices remain
separately pinned. Production512 large-case completion and all repeated cluster
performance/calibration requirements above remain open.

## Independent private-mesh proof at the production512 rejection

coarsen512_private_audit_v1/REVIEW.md supplies a fresh full exact audit of the
retained historical517-cell/153-point private coarsening export. It passes exact
geometry/domain/markers/original-P1 quality/sliver/full-edge gates, including
L<=1.8 without relying on an auditor tolerance. qmin=.20433538142016777,
Jmin=.08825346092225352,Lmax=1.7946315664648058,volume=1/2. The rejection is
queue exhaustion under512rounds with266pending directions, not failure of the
measured physical criteria at that exit. This private grid is not accepted CFD
output and lacks new state transfer/CGNS/driver replacement certification. No
completion-policy or guard change, current-source512rerun, performance evidence
or Goal1 completion follows. Existing distributed queue-completion and repeated
cluster performance/calibration requirements remain open.

## Current failure classification qualification

COMPLETION_DIAGNOSTIC_VALIDATION.md/json records current rebuilt97-case MPI1/2/4
plus fresh exact distributed/driver/state/SU2CGNS/MMG audits. Final engine
completion uses the same one integer maximum reduction and unchanged Boolean
validity, while reporting valid-but-pending coarsening separately from invalid
metric cells or missing measurements. Existing forced-budget fixtures verify
the first two causes globally and preserve input.169/271/391raw numerical JSON
records match the previous qualified binary;8336Boolean combinations preserve
completion truth.32core+14comparison checks and full compiled-source matching
retain older packages plus the new diagnostic-only source variant. This closes
the misleading-message gap but does not resolve default512completion or supply
cluster cost/scaling/overhead/weight/wait/memory calibration. No fullGoal1 completion.

## Saved active-load and reservation diagnosis

reservation_balance_review_v1/REVIEW.md verifies three pinned successful N4M2
logs (18operation events) and unchanged partition/directory/donor/remesher source.
In the aspect2 second coarsening, proxy weights959/979 and owner candidate work
649111/644747 are nearly balanced, while attempts130/64 and vetoes0/92 reveal
lowest-rank reservation preference. Active partition/proposal-outcome summaries
are now reusable in cluster analysis;34core+15comparison lightweight checks pass
and legacy recorded runtime fields remain raw-log verified. This identifies a
structural balance hypothesis, not calibrated rank CPU/wait cost or net benefit
of rotating priority/repartitioning. No production scheduling/weight/default
change or new numerical/profiling run. All full repeated performance gates remain.

## Isolated reservation fairness feasibility

rotating_reservation_probe_v1/REVIEW.md records one source-pinned isolated
directory variant on the actual configured driver, with untouched production
sources/objects/archives. Default512N4M2 wedge passes three actual replacements
and fresh full independent mesh/metric/current+BDF/SU2CGNS audits. Second
coarsening attempts78/78 and vetoes51/50 are more evenly distributed than the
standard106/54 and0/87, but accepted meshes differ. One extra epoch word per
vertex is carried and checked in existing votes. No fixed-work timing ratio,
full epoch/admission/stale/MPI1/2/4/stronger-envelope or repeated scaling proof.
The variant is not production and does not alter the prepared donor comparison.
This is useful feasibility evidence, not completion of any outstanding full
performance/calibration/production-limit requirement.

## Isolated protocol safety follow-up

rotating_reservation_protocol_v2/REVIEW.md supplies actual MPI1/2/4 compiled
shared/disjoint complete-star and priority-phase/memory-veto tests using the
exact earlier audited isolated directory variant. Unique winners rotate and
nonconflicting proposals stay concurrently selectable; rejected mismatched phase
or one-rank memory cases preserve both stores. Serial mismatch branch is NA.
Original source/header/binary/objects remain unchanged; first pre-MPI fixture
compile failure is retained. This closes targeted protocol gaps, not full97case
rotating-policy/general safety/subset/stronger-envelope/repeated-performance
qualification. The prototype remains isolated and no Goal1 completion follows.

## Full isolated rotating-policy small MPI regression

ROTATING_RESERVATION_FULL_VALIDATION.md/json verifies the entire configured97case
selection at MPI1/2/4 on the exact isolated directory variant, with fresh seven
distributed-record and actual geometry/current+BDF/SU2CGNS audits. Driver frame/
state/readback coverage12/24/48,6/12/24,6/12/24; original2D/MMG/default/atomic/stale/
repair/subset controls pass. Production source/binary/objects stay unchanged.
This closes the full small regression gap for the prototype; stronger whole
aspect/thin/restart/lifecycle/default512 and repeated matched-target cost/scaling/
traffic/overhead qualification remain before promotion. No Goal1 completion or
new fixed-work performance comparison follows.

## Isolated stronger whole-case and actual lifecycle qualification

ROTATING_RESERVATION_WHOLE_VALIDATION.md/json extends the isolated rotating-policy
qualification with the predeclared aspect2 turning size4, H20 covariant thin size4
and actual CGNS persisted-restart size2 N4M2 cases, plus six existing MPI1 actual
metric/Hessian/CFD/BDF controls. All new trajectories pass exact independent
geometry/metric/state/output audits under unchanged512/q/J/L/noise gates. Eight
H20 normalized records match the same saved unscaled variant; six serial controls
match standard geometry/metric records exactly. Strong-case reservation opportunities
become more balanced, but small restart coarsening uses20rather than16rounds,
so neither fixed-work speed nor universal reduction follows. Collector-only
legacy missing counter failure retained/corrected explicitly; production unchanged.
This closes these stronger whole/restart/lifecycle proof gaps for the prototype,
not the large default512limit or repeated matched-target cost/scaling/overhead/
traffic/wait/weight/memory gates needed before promotion/fullGoal1.

## Separate changed-trajectory cost campaign prepared

The new native_cluster_3d_reservation_comparison_v1 tooling prepares a same-build
standard/isolated-rotating comparison with independent exact references per
scheduler/N/M and paired operational-cost repeats. It does not weaken the donor
fixed-work harness or pretend later meshes/P1fields match across schedulers.
Actual login-preparer smoke verifies one private directory compile/link and
closed regular-file packages with unchanged production artifacts;16synthetic
orchestration checks and SGE syntax pass. The first Ninja-path failure and
correction are retained. Disposable local copied binaries removed with hashes
after verification; retained smoke folder is not a runnable cluster bundle.
No actual MPI/qsub/performance campaign or new numerical certificate. This closes
cluster-tool preparation for the fairness hypothesis, not repeated performance,
weight/wait/traffic/memory/default512or fullGoal1/promotion gates.

## Persistent counterexample eligibility replay

collapse_witness_replay_v2/REVIEW.md verifies bounded16/128-entry exact witness
models on940historical source-matched proposals. Length-failure-only shared bad
chord memo identifies7and20rejects with zero suppression of709accepted proposals;
only2matches cross collapse directions. Earlier oracle's7/23benefit includes
learning bounds unavailable after production's early shape check and is retained
separately. Partial local-star negative evidence is a feasible future pattern,
not an implemented rounding-safe/MPI cache,512completion or performance result.
Full615/1002tails, changed real trajectories/ownership/cache cost remain unproved.
Production/targets/guards/cluster packages unchanged and all complete Goal1
performance/default-budget/practical-limit gates remain open.


## Selected static findings fixed while Goal 1 is paused — 2026-10-12

Working repository SU2_NativeIntegrated; branch codex/native-3d-transport-accounting;
base 7746fd53fa2f. The user separately authorized fixes during the pause.
Global boundary validation no longer inherits cavity sphere/512 restrictions;
closed oriented manifold checks remain. Aggregate owner publication/retired-edge
scratch use byte bounds. Unused parent source/weights are skipped when no
repartition; collective controls are checked before conditional source calls.
Native3D docs and new100/legacy97 cluster guards are corrected.
101 mixed C++ cases per rank MPI1/2/4, fresh bridge and seven distributed audits,
serial MMG and 34 package checks pass. Actual core registration is100.
See integration_evidence/native_3d_core_v1/STATIC_ASSESSMENT_FIXES.md and
STATIC_ASSESSMENT_FIXES_VALIDATION.json for pins, exact scope and retained failures.
MPI1 v2 is reused; MPI2/4 v5. About53.5MB retained, no copied builds/binaries.
Runtime-only audit did not certify mixed logs; driver C++ checks pass and no
independent runtime certificate is claimed. All jobs terminal; oneCPU/nice19/j1/
thread1/MPI<=4 correctness only. Goal1 remains PAUSED, Goal2 not started.
No default512/high-rank/performance/curved/3DBL/physical-unsteady claim. Earlier
comparison packages are frozen oldsource and need refresh. AdapNoExt and foreign
ClusterResults untouched.


## Batched dependency lookup qualified — 2026-10-12

Repository SU2_NativeIntegrated; branch codex/native-3d-transport-accounting;
base679fe68ef414. Goal1 ACTIVE after the user resumed it. LookupBatch now returns
separate bounded complete vertex/edge/face stars through the existing ticketed
exchanges; missing/oversized candidates refuse individually, common protocol/
memory failures refuse the batch. Shared identities checked; legacy union/wire/
collective semantics retained. Targeted MPI1/2/4 PASS; current102-case mixed
regression plus fresh bridge/seven distributed audits and serialMMG PASS.
199/331/511 prior numerical JSON records match exactly. Five equivalent lookups
use30vs6World wrapper calls, same640byte serialized total, not MPI/network/wait
or elapsed remeshing speed. New core manifest101cases (legacy97/frozen100 retained),
34package checks and actual registration PASS. BATCH_LOOKUP_VALIDATION.md/json
pin scope/source/68objects/binary/archives/tools and all evidence. Current full
case folders batch_lookup_regression_v1/mpi1,mpi2,mpi4; initial targeted v1/source
snapshots retained. All owned jobs terminal; oneCPU/nice19/j1/thread1/MPI<=4,
correctness only. About52.8MB, no retained build copies. Runtime C++ checks pass,
no independent runtime certificate claimed on mixed logs. Engine still one
proposal/worker/round: next batched payload/reservation then private multi-edit
engine integration. Default512 and repeated practical performance/fullGoal1
remain open. No Goal2/curved/3DBL/physical-unsteady claim. Foreign work untouched.
