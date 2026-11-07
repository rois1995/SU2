# Geometry-triggered complete-star visibility placement

Current status: build243 and actual-cavity check terminal/passing; build244 adds
only affected distributed validation. Production implementation not yet qualified
for full execution. This
is one connected continuation of actual237 residual-cavity diagnosis. No accepted
mesh, transfer, continued adapted CFD or disk restart has been established.

## Demonstrated limitation

[Exact240 geometry evidence](ADAPTATION_CAVITY_GEOMETRY240_20261107.md) establishes
real stored-surface violations on chord7868–7876, not merely loose upper bounds.
241tests the existing split/insertion/reconstruction paths on its complete2-cell
ring and52-cell neighboring context; all refuse.242tests four existing normal
strip displacements; all invert inherited cells. A bounded18-cell complete-apex-star
probe with proxy projection finds exact-positive represented children, suggesting
placement freedom exists but not proving native geometry admission.

Source inspection found PlaceSplitPatch accepted reserved complete-apex stars but
only tested a no-move split. Thus sequential splitting could fail while existing
complete-star support contained unused placement freedom.

## Connected change

Reuse the existing bounded convex clipping kernel (mechanically moved from the
CFD include tree into Common; original include forwards without API/math change).
For native-projected split coordinates, intersect positive-visibility halfspaces
for all affected inherited child cells and all retained adjacent cells. Propose
the finite kernel centroid for a free interior apex, using scaled local coordinates.
A maximum8-apex bounded coordinate sweep can propose coupled movement. A failed
kernel is not proof of infeasibility; clipping-capacity exhaustion fails closed.
The kernel only proposes: existing SplitSurfaceStrip exact positivity, surface
association, body/feature/interface and requested-height checks decide admission.
No q/J query, shape ranking, quality-progress veto or new operator is introduced.

Enable the existing geometry-triggered complete-star lookup/fetch/reservation and
placement path for free interior apices. Preserve existing64-cell/8-apex bounds,
work/time/memory admission, empty-rank collective sequence, full-vertex reservation,
versioned dependencies and complete publication. Reuse existing9 construction node
IDs for curved paths (split plus up to8 relocation identities). Geometric trials
use existing GeometryRefine accounting for intentional split+relocation semantics.
Surface-apex motion remains with existing surface operators; this search moves
only free volume support. Private curved parameters are reconstructed from the
current patch; native split rebinds output before validation/publication.

## Acceptance of this change

First assess actual18-cell motion support joined to all52 original neighbors:
native association, positive/conforming replacement, geometry excess and bad counts,
net whole-context q/J changes, and metric-size effects before/after existing polish.
Then affected serial checks and one MPI6 milestone, including empty ranks,
conflict/stale-dependency atomic refusal, curved associations, requested heights,
cache/recomputed census and migration flags. Reuse unchanged valid evidence.
A bounded fixture is not mesh capability acceptance. Return immediately to the
full same-seed corrected-L1 two-event50k attempt with native5400/outer14400 unchanged.
Judge actual geometry/size/shape closure and whole-field net bad-cell progress;
construction is allowed intermediate poor q/J but never invalid cells.

## Limits

A convex per-apex kernel and coordinate sweep need not find every feasible coupled
placement. Fixed clipping capacity can refuse useful proposals. No larger cavity,
new topology mechanism, quality-aware construction or geometry representation is
justified by literature or proxy feasibility alone. Exact admission prevents a
numerical proposal from relaxing any gate. Actual MPI integrated path coverage
must be checked before claiming it validated.

## Actual243 outcome

Build363.243376s, all1303source pins unchanged/both targets current/compiler handles
absent. Actual-cavity case93.042200s/pins unchanged/all sampled handles absent.
All eight old constructor refusals remain. New placement constructs18→20cells in
3trials/one free interior-apex move (0.006826s descriptive local wall time).
Joined context52→54cells: q-bad4→3,J-bad3→3,geometry-bad6→6; geometry excess
1.7369589027103044e-12→4.3986594049519444e-13(−74.68%). Native GeometryRefine,
replacement and prescribed-interface checks pass; raw quality Repair correctly
refuses. Existing optimizer on this candidate,46trials/1.366686s, gives59context
cells/1q/1J/5geometry and2.9580954127791946e-13excess(−82.97%). Maximum metric edge
stays0.55380422188452372; no added/removed oversized-edge excess. No global accepted
mesh or nonlocal publication claim follows. A237/visibility243_assessment.json
records exact inputs/status/results/limits.

244adds a test-only actual-cavity MPI publication check. Initial Owned lives only
onrank0; peers empty. All participate in stale-version rejection, full-star conflict
reservation, veto without mutation and complete publication. Candidate curved
parameters are rebuilt at use and replacement certified before publication. This
is direct directory/publication coverage, not proof that Engine selects this path.
Combined MPI6 suite also retains existing curved/height/cache/ownership coverage.
No further production change since243.

## Connected244 qualification and preserved failures

244build168.129617s, all1303pins unchanged/both targets current/compiler handles
absent. Only the test fixture changed since locally useful243production.
Connected MPI6 milestone101.983105s passes55cases on EVERY rank; six exact current
cavity results agree, initially empty peers align, stale/veto refusal and complete
cell publication pass. All10sampled handles absent; pins unchanged. Existing curved
runtime, heights, geometry, census, phase ownership, repair fairness and clipping
checks pass. The direct actual check does not exercise Engine selection or commit
boundary parameter edits. Full M6 remains decisive.

A mistakenly selected legacy serial_full campaign is preserved as FAILED:
305.529924s,344cases/336pass/8fail,1161529assertions/27fail,all pins unchanged/all
handles absent. Historical handbook explicitly warned against this generic filter;
no repeat or old-code baseline campaign. Known donor-count and scenario-count
assertions are documented in ADAPTATION_GEOMETRY_QUERY_REUSE_20261031.md244–258.
The zero-work support test cannot reach changed placement. The wall-policy RANS
fixture refuses at4096work, placement95trials/0moves; attribution to this change is
not established, and baseline equivalence is not claimed. Preserve this open BL
issue for the required qualification after accepted Euler. Do not weaken assertions
or acceptance gates. Individually passing affected serial cases are reused; the
broad campaign itself is not called passing.
R/visibility244_{qualification,broad_serial_failure,source_provenance}.json record
scope/results/provenance. Immediate next is full same-seed corrected-L1 two-event
M6 at50k, native5400/outer14400 unchanged.
