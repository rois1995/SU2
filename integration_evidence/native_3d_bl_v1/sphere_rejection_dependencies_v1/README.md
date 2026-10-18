# Accepted repeated spherical BL adaptation — retry lifecycle

Full Goal3 ACTIVE/incomplete. Base d4d531c0f6e20516178be98c7f954f715b2de075.
This is a complete accepted initialized RANS/current+BDF/SA repeat on N4/M2 under
unchanged constraints, not a converged CFD or affordability result. Historical
failures and the test/timing corrections remain in DESIGN_AND_FAILURES.md.

## Underlying limitation and complete change

Progressive measurement calls Selection::Add/Queue for newly measured unchanged
cells. Queue previously reinserted rejected directions even when the existing
blocked-edge map still held them. Block also discarded a rejection routed to an
owner that had not measured that edge. Thus measurement alone reopened searches,
bypassing the documented topology-change rule.

Use the existing blocked-edge map in Queue, and retain routed blockers before edge
measurement. Actual Directory changed-star notifications still invoke Touch and
reactivate directions, including unchanged candidate edges with changed remote
stars. No general reconstruction-failure cache, new hash/key/lifecycle, cap increase
or new configuration. Additional blocker packing is admitted before allocation.

The expanded timing control exposed an earlier query-reuse accounting error:
imports inside seed/update were also counted as exclusive donor time. The existing
RAII timer now optionally excludes its elapsed time from its reported parent;
query imports are charged once. An obsolete exact diagnostic assertion now matches
the SAME incident-reference-facet failure, with INFO. Neither correction weakens
functional or numerical assertions. Both initial failed receipts are preserved.

## Actual difficult workflow

[Complete run and viewable grids](actual_update), historical accepted cold producer
at ../sphere_rans_lifecycle_v1/n4_m2_v2/construct. The same CGNS geometry/reference,
current+BDF/SA states, original identity sensors, h0=.05, growth1.2, thickness.2,
geometry ceiling.04/effective.025, q>=.2, J>=.05, full edge<=1.8,6144work and300s
are preserved. No Hessian, normalization, CFD advance or original-donor refit.

| Quantity | Previous query-reuse attempt | Current dependency lifecycle |
|---|---:|---:|
| Native outcome |atomic work rejection|accepted|
| Input cells |5440|5440|
| Accepted output cells |none|3768|
| Global proposal work |6144|4511|
| Global private edits |344 uncommitted|344 committed|
| Native remesh seconds, rank0 status |134.280|88.499|
| Complete fixture seconds, rank0 |incomplete|89.390|
| Rank0 donor seconds, coarsening |41.248|25.043|
| Rank0 tracked native bytes |179,088,200|144,074,952|
| Rank0 tracked native collective calls |789,281|583,281|
| Modeled native work bytes |196,219,942|196,219,942|

Current rank0 coarsening suppresses6997 queue insertions and retains116 rejected
edges before local measurement. Same344 edits with complete surviving-cell coverage
support the retry diagnosis; identical private coordinates/topology of rejected
runs were not exported, so byte-identical meshes are not claimed. The old event
rejected; differences in work prevent a clean same-work kernel speed comparison.
Both are contended local runs.88.5s adaptation alone is not CFD affordability.

All four ranks preserve the original CFD arrays before actual replacement. Current
transfer/BDF/history checks, retained geometry authority, both mesh outputs/reload
and real restart INPUT pass. Full native composed-edge/shape/height gates pass.
This fixture writes future checkpoints but does not advance CFD or launch a second
new-output restart driver; the separate current strict/opt-in controls cover that.

## Independent accepted-mesh/state proof

The retained audit_update.py is now executed for the first time on this complete
repeat. Exact positive volume, manifold topology, connectivity,783vertex links,
first height and overlap pass.2704 tetrahedra retain exact original geometry;
40699 unchanged-tet pair certificates reuse the independently audited producer.
62804 other pairs receive fresh exact overlap tests,2409 shared faces are certified
through oriented topology. Maximum height relative error3.11e-15, zero foot deficit.

Actual current+BDF flow/SA integral defect1.87057e-14; transported original identity
sensor residual0. Both SU2/CGNS grid/reload signatures agree. Fixed curved-domain
volume matches the input, with no new fill/loss. The fresh geometry audit takes
111.85s; complete independent audit116.91s, separately from native adaptation.
Its scope explicitly retains spherical full-field shape/edges as NATIVE, not an
independent spherical BL tensor oracle or global composed-gradation certificate.

## Connected qualification and evidence

Final serial30cases/1769assertions and MPI2/4 thirty cases per rank PASS, including
progressive field discovery/late invalid queries, repeated planar cycles, complete
changed-star notifications, immutable-facet/orientation filtering and atomic budget
refusal. Saved320-cell control proposal work797->350serial and1502->1226MPI4;
MPI2 current726. All require complete cached field coverage and the existing mesh
audit. Cached queue skips are positive at every rank count; MPI2/4 also exercise
unmeasured-owner blockers. These are scoped controls, not scaling observations.

Current complete strict/opt-in planar N4/M2 RANS/SA/current+BDF/SU2+CGNS/reload/
new-output restart controls and independent oracles PASS separately. The enabled
budget3/20 cases preserve lambda.875/0,16/240cells and independent masses
2.95498841845/19.9993054819. Receipts and summary.json preserve their exact scope. Initial timing/message failures,
three builds, debugger diagnosis, source capsules and host memory/load records stay.
One owned heavy job at a time, nice19/build-j1/threads1/local MPI<=4; no foreign jobs
or directories touched. GNUtime launcher RSS is not aggregate remote-job peak.

`python3 verify.py` verifies sealed scientific records and current source/build/
binary pins without CFD. Scientific outputs stay byte-for-byte; their whitespace is
not rewritten to satisfy source-format checks. All evidence links resolve inside
the repository. Check the final verification.json before treating the checkpoint
as qualified.

## Remaining full-goal scope

This resolves the recorded strict repeated-BL workflow; it does not close Goal3.
Keep original-P1 sensor interpolation, actual-query geometric composition, finer
requests, selected-profile identity, support/fade and all gates. The opt-in BL-budget
contract stays strict by default. Global absolute-error calibration/allocation,
complete enabled curved/interacting policy workflows,2D policy, independent wider
composed-field validation, viscous accuracy and complete accepted-event performance
remain. M6 budget estimates and its exact-facet/binary64 construction obstruction
are separate; do not retry the unchanged frozen M6 or change geometry authority.
Next substantial capability: qualify the full selected-profile lifecycle on curved
and interacting geometries, using bounded original-field normalization/reassessment;
do not introduce another local fallback or micro-cache merely because it is easy.
