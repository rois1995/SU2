# Codex handoff — start here

Worktree `/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated`, branch
`codex/native-3d-bl`; Goal2 base `c43a8a3907ea2655e4ebbb6286ab16c3fa625d8b`.
**Full Goal3 ACTIVE/incomplete.** Preserve the [full scope and gates](integration_evidence/native_3d_bl_v1/GOAL3_REQUIREMENTS.json).
AdapNoExt, foreign ClusterResults and dependencies remain untouched.

## Current connected work

Primary task: bounded uncertainty-controlled complexity initialization on original
coarse M6 sensor-only P1 plus actual-query geometric BL. Preserve original donors,
finer demands, fade, geometry/height/positivity/quality/transfer/full-edge gates and
noise0; never spread wall tensors across coarse cells or change target mid-stage.

**Current C++ batch complete:** native face/edge/vertex support sampling over the
house ADT, plus actual original-donor publisher routing. Shifted storage disproves
any cell-ID/rank shortcut. Native/Python PDFs agree<=3.64e-12 at all8192 points of
each M6 seed. Retained tensors agree direct/cached at the unscaled baseline;
MPI2/4 rows/normalization agree. Geometry/PDF~5.2s versus23.7s independent Python
check at the same points: kernel evidence, not adaptation speedup.21 serial cases/
10129 assertions,4 donor/regional cases1/4 ranks and two collective failure checks
pass. All owned jobs terminal; final donor-object refresh preceded every test.
[Implementation, evidence, resources and limits](Handbook/GOAL3_NATIVE_SUPPORT_PROVIDER.md).

**Runtime initialization batch validated:** CSolver collectively switches limited
strict faceted3D quadrature to a retained original-source sensor+BL-correction
estimate. Every new trial uses actual bounded/graded tensors/core.18 composition/
controller cases, planar solver accuracy, changed-field/anisotropy/fault controls,
whole frozen M6 MPI2/4 and one actual saved-state M6 initializer pass.
[Full evidence, rejected experiments and cost](Handbook/GOAL3_RUNTIME_COMPLEXITY.md).
Actual coarsest-field estimate790200.5 +/-27958.5 against100000 is provisional/
outside_estimated_bounds; no default5% tolerance, new adapted mesh or transfer.
**Geometry-aware preflight follow-up validated:** same-config actual M6 metric
415.97->16.47s, summed rank CPU1480.46->48.18s;7->1 trials/1218->1 sweeps. Same
coarsest estimate/uncertainty and byte-identical CFD restart. Nonplanar faceted
walls choose bounded integration immediately; planar accuracy gate remains.
Reachable-budget solver and original-field MPI2/4 controls pass. Cold support
preparation7.69s now dominates assembly; no full remeshing/scaling speedup claim.
[Exact comparison, evidence, resources and limits](integration_evidence/native_3d_bl_v1/oneram6_uncertainty_v1/production_complexity_geometry_v1/README.md).
**Next:** global uncertainty/error-directed work and same-original-field finer-mesh
reassessment/bounded correction epochs. Do not refit on adapted nodes, rerun the
unchanged rejected construction, add a limit ladder or claim an accepted M6 grid.
One owned heavy job at a time/build-j1/nice19/threads1/MPI<=4,38–40GiB host available;
build peak process2.37GiB/pilot577.72MiB, not aggregate RAM. All owned jobs terminal;
both test_driver and standalone SU2_CFD are current.

**Rejected predecessor:** whole-donor point retention was correct but slower on
M6; both AABB and exact-filter variants are withdrawn. Their executed source,
results and failed attempts remain in the
[rejected-cache report](Handbook/GOAL3_POINT_PREPARATION_REUSE.md).

**Canonical owner point samples validated at dca448703a:** existing owner ADTs
and bounded sensor memo replace per-batch indexes.512 cells exact1/2/4 and4096
cells MPI4 PASS;3–9x selected preparation gain, no full-event claim. Curved return
and independent RANS/SA N4M2/BDF/SU2+CGNS/restart PASS; all BL queries composed.
[Evidence, scope and resources](Handbook/GOAL3_OWNER_POINT_SAMPLES.md).

**Composed-edge reuse batch validated:** existing measurement maps now require
immutable full-field identity/exact endpoints; first-row alternatives reuse common
edges.73 serial controls, M6 wall32 exact cold/shared/repeat on1/4 ranks and affected
MPI/curved/RANS N4M2/BDF/SU2+CGNS/restart PASS.192->137 integrals, serial.611->.444s;
selected local replay only, rank0 requester, no complete M6/scaling gain. Original
field and mesh gates unchanged; all owned jobs terminal/current binaries rebuilt.
[Evidence, resources and limits](integration_evidence/native_3d_bl_v1/oneram6_uncertainty_v1/composed_edge_reuse_v1/README.md).

**Construction/budget constraint:** latest M6 attempt remains rejected at300s,
447 INTERNAL publications/1788 work, with rank2 geometry147.86s. No accepted mesh
or transfer; do not rerun unchanged construction or raise limits. Same-field
refinement cannot lower continuum mass; strong-BL lower estimates exceed100000
under conditional statistical assumptions, not machine certification.
[Preserved bounds, normalization and rejected experiments](Handbook/GOAL3_M6_CONSTRAINTS_AND_EXPERIMENTS.md).

**First-row transaction batch validated:** complete one-cell interiors can share
read-only outer vertices. Groups retain exact skins, bounded imports, versions and
atomic veto/publication; private work checks local deadlines then votes per group.
9 serial controls plus MPI/general/curved/RANS N4M2/BDF/SU2+CGNS/restart PASS.
M6 all303359 original donors/full wall,64 private initializations2/4 ranks: exact
batch1/8 geometry/markers, positive/exact volume and unchanged skin. First-row
6.93->6.05s /4.67->3.58s; tracked calls~82% lower, no full-event/scaling claim.
Synchronous-group variant had no elapsed gain; executed patch/evidence preserved.
Old driver baseline timed out; corrected rank-only fixture failure stays failed.
All owned jobs terminal/current binaries built. Metric evaluation still dominates.
[Evidence and rejected grids](integration_evidence/native_3d_bl_v1/oneram6_uncertainty_v1/first_row_transactions_v1/README.md).

## Remaining evidence and scope

No accepted M6 grid, rigorously qualified complete mass, calibrated global error/production
normalization, actual finer-mesh reassessment or lifecycle improvement yet. Need
defensible uncertainty/absolute work allocation before further mechanisms.
Reference sources/failed approaches remain preserved. Final suggestion-by-suggestion
report requested: keep [outcome ledger](Handbook/SUGGESTION_OUTCOME_LEDGER.md) current,
including its other-agent performance accounting; do not replace or discard it.
[Broader query/transaction package](Handbook/POST_COMPLEXITY_QUERY_TRANSACTIONS.md):
point-only lookup and measured composed-edge reuse are implemented; first-row interior publication batching is qualified locally. Wider repair-
campaign support/edge lifetime and general displaced-transaction batching remain open.

M6 initial120-step RANS/SA state, exact original owner/halo tensors and geometry
are saved. The subsequent construction-only attempts are provisional captured
fields, not final100000-budget normalization or flow/transfer validation.
[Inspectable M6 state](Handbook/GOAL3_ONERA_M6.md),
[exact frozen fields](Handbook/GOAL3_M6_FROZEN_FIELDS.md),
[thin-integration failures](Handbook/GOAL3_M6_THIN_INTEGRATION.md).

30P30N input extracted/validated at integration_evidence/native_3d_bl_v1/
three_element_input_v1 (64540 points/127914 positive triangles); supplied2D CGNS
was one hex layer. No airfoil adaptation/CFD run. The8.63M-hex3D input is outside
native tetrahedral volume support.

Prior-source qualified foundations remain: curved cylinder2877/eight fronts,
right-prism coordinated caps, original sensor transport, retained opposing-wall
removal and actual small RANS/SA N4/M2 current+BDF/SU2/CGNS/reference/restart.
They are NOT automatic current-source proofs. General features/junctions, exterior/
nonprismatic/interacting construction, repeated retained-BL updates, independent
varying full fields, viscous convergence and lifecycle affordability remain open.
Sensor gradation does NOT establish composed gradation; saved violations remain.
[Construction](Handbook/GOAL3_COARSENED_METRIC_FRONTS.md),
[whole caps](Handbook/GOAL3_WHOLE_CAP_CONSTRUCTION.md),
[selection/actual planar checks](Handbook/GOAL3_SELECTION_WORK.md),
[curved RANS](Handbook/GOAL3_CURVED_RANS_RUNTIME.md),
[prior full entry/history](Handbook/HANDOFF_ARCHIVE_2026_10_17.md),
[deferred physics questions](Handbook/FUTURE_PHYSICS_TRANSFER_QUESTIONS.md).

## Working rules and resources

Follow [AGENTS.md](AGENTS.md): substantial capability-first changes, known hard
case early, scoped/provenance-aware resumable checks, no broad suite per small
edit, no repeated unchanged failures/limit ladders, push reviewable work. Keep
handbooks compact with detailed linked topics. Announce full case folder paths.
One owned heavy job, low priority/thread1/build-j1, locally available MPI<=4.
Check host contention/RAM and live handles. Latest host38–39GiB available; recheck.
Current build peak process2.40GiB, MPI/audit wrapper647MiB;
recorded swaps0, not aggregate or whole-lifecycle peaks. All owned jobs terminal. Root/tmp nearly full: use build-integrated-v2/compile-tmp
on4TB. Do not kill unrelated processes or request cluster jobs overnight.
Goal4 inactive; full physical unsteady3D/general CAD/mixed volumes separate scope.
