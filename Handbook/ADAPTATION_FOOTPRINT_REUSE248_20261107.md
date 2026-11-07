# Selected connected change: reuse proved footprint regions within a query

2026-11-07. Implementation LOCAL,compiled250;final validation pending;ZERO accepted meshes.
Current build247M6first event refused with1148q/484J/1044geometry-bad cells.
The immediate demonstrated blocker is deadline exhaustion with useful queued work.
Geometry/repair power,transfer,continued adapted CFD and actual disk restart remain
unproven;then dedicated BL/MPI qualification and RANS500k. All strict gates stay.

## Evidence and missing capability

ONE bounded seven-context actual-terminal probe passed on247,with exact q/Joracles.
`staged57/cases/current_compound_20261107T164754_308827` and
`m6_runs/euler_build247_canonical_mach_only_real_cfd50k/actual_terminal247_*.json`.
5241completed queries,4137hits/1104misses;identical repeats already hit.
Of5096073total counted query work,upper evaluation consumes3822194.
Within upper work,finite-footprint coverage uses1349783and lift1412491;
remaining upper work1059920. Existing prepared-facet/scope indices,cropped witness
and complete-support lower pruning are active. Small-motion pass-margin reuse
covers only2.6%of total work;simple lower-first potential onlyabout1.3%. Neither
is selected. No new envelope or geometry representation is justified.

`TriangleUnion::Upper` repeatedly calls `PlaneUnion::Upper` while subdividing the
SAME physical query. Each call proves a projected triangle or clamped rectangle
lies in the SAME immutable finite footprint using `FinitePlaneReference` indices.
A previously proved convex region can also cover a later subtriangle. Current
code does not retain that within-query coverage proof;all 3D lift calculations
must still be fresh. This is the selected reuse opportunity,not an established
speedup or a general cache for moved-face acceptance.

## Connected design and proof obligations

Change only the existing surface-union upper traversal in CNativeAffineReference.cpp.
Coarsening,splitting,quality and final mesh-side certification already share it.
Keep validity-first construction and every caller's admission/merit semantics.

Use an OPTIONAL query-local,owner/group-keyed bounded store of successful projected
triangle/rectangle coverage regions (four slots per existing plane group,matching
the subdivision fanout). A ticket exists only after the existing exact finite-
footprint predicate proves the COMPLETE region covered. Before reuse,form the NEW
nominal projected triangle and prove all three vertices are in the stored convex
region with exact/robust predicates (closed boundaries allowed). Convexity then
proves its whole projected triangle covered. Any uncertainty,degeneracy or mismatch
uses the existing full footprint check. Never infer coverage from samples alone.

Only skip that repeated 2D containment proof. Recompute padding,XY shift handling,
3D residuals,piece restrictions,lift/error bounds and all position-dependent data
as before. Association,features,body separation,heights,complete support and final
geometry certification remain unchanged. HasAcceptedFaceCertificate is NOT assumed
to provide an arbitrary numerical surface-distance bound.

Tickets live only inside ONE QuerySurfaceUnionAtResolution/directed query with its
immutable prepared witness held alive by existing active references. Reset across
new query/target/resolution;never use a ticket from another support/footprint owner.
No global cache/scheduler/config/geometry representation. No nearby-coordinate
face-certificate bypass. Keep the model-to-mesh branch unchanged unless the same
proof is separately established;the physical triangle path is the measured target.

Admit all memo allocation/growth/lookup/predicate work with existing AtlasBudget and
caller-byte accounting;bounded slots,constant lookup per plane. Lazy allocation may
avoid charging cheap witnesses that never need footprint traversal. On inadequate
transient admission,disable this optional reuse and execute the existing path.
Destroy buffers before restoring caller-byte accounting;preserve sticky exhaustion.
Do not enlarge any budget or change the native5400limit.

## Validation and return to M6

Before broad checks,validate real risks:contained child and boundary-touching cases,
holes/nonconvex footprints,separate bodies,changed target/group identity and resource
fallback. Cached/recomputed conservative results must agree under sufficient budgets;
fresh padding/lift cannot be skipped. Reuse existing curved/seam/feature/height/
accepted-face/trace/cache fixtures,adding only focused cases for the changed proof.

Rerun the SAME actual seven-context protocol once to compare exact query results,
complete-context q/J/geometry outcomes and coverage/upper work. No useful candidate
may be lost. Timings with verbose trace are not performance evidence. If useful
work savings do not materialize,do not sell a cache-hit counter as progress.

At the coherent milestone run ONE affected MPI6qualification,then IMMEDIATELY full
same-seed MACH-only L1/50k TWO-event M6 with unchanged gates/budgets. Compare net
whole-field q/J-bad removal and geometry counts at actual quality ages,explicit
mismatches and matching stages;publish all failures. Need shape closure,residual
geometry/size closure and shape reclosure,then conservative admissible transfer,
continued CFD and isolated adapted disk restart. Geometry power remains open even
if this speeds shape search;select its remedy from that run's actual outcome.

[247outcome](ADAPTATION_BUILD247_OUTCOME_20261107.md),[current handoff](../HANDOFF_Codex.md).

Local implementation retains four exact projected regions per plane in a lazily
allocated QUERY-LOCAL vector. All new triangle containment uses robust orientations
or exact closed rectangle comparisons. Only the physical mesh-side upper calls
receive the memo. Optional admission leaves8192bytes kernel scratch headroom;
insufficient byte/work allowance uses the original path. Dynamic bytes are charged
until the memo is destroyed,before caller restoration. Added checks warm coverage
then vary lift,holes/body scope,and constrained transient memory. Build248passed45.107960s,all1303pins unchanged,both targets current,all20sampled IDs absent. Focused serial validation is live;keep pinned source/fixtures unchanged.

First248serial run failed only the new low-memory fixture:37/38cases pass,
12/2044assertions fail because one shifted input selects a new unprepared witness
(16176scratch+9827caller exceeds16384). Preserved failure and all handles terminal.
The fixture now warms THAT exact scope using BoundSurfaceUnion,which does not
populate the upper-query result memo. The16384ceiling and production code remain
unchanged. Build249is LIVE;keep all pinned sources frozen until terminal.

Build249serial passed38cases/2044assertions in62.358936s after exact-scope warming;
all owned handles terminal. Final250production adds explicit3/2work charges for
triangle/rectangle proof stores. Build250passed33.103694s,pins unchanged and all
15sampled host handles absent. Same actual seven-context comparison is LIVE in
R/footprint250_compound_attempt.json;source/fixtures/helpers frozen until terminal.
No geometry work saving or final acceptance established yet.

## Actual250 outcome: rejected; smaller monotone cutoff selected

Same seven actual-terminal contexts passed103.756774s,all pins unchanged and six
host handles absent. All5241query identities/cache outcomes/bounds and166complete
result rows match247exactly(excluding time/work). Footprint reuse raises counted
query work5096073→5655099(+10.97%);2494hits save28422coverage units while40792misses
add585937. No timing benefit established. Production memo is REMOVED before251;
exact failed patch/evidence preserved as R/footprint250_rejected.patch and
footprint250_{actual_comparison,lookup_cost,lift_cutoff_opportunity}.json.
Focused hole/body/lift/resource assertions remain useful and are retained.

Within the SAME shared upper-certification optimization cycle,selected smaller
hypothesis:pass the current minimum upper witness into PlaneUnion::Upper and stop
its monotone maximum once it cannot improve that minimum. Baseline19163graph
calls end worse than the final selected upper,consuming548962lift units;this is
only opportunity evidence,not a saving estimate. Each lift updates bound by max;
remaining lifts cannot reduce it. Final padding is nonnegative. If partial bound
>=current best,return infinity as a losing candidate;the caller's min stays EXACT.
Otherwise preserve the existing complete graph evaluation. All coverage/support/
body/association/height/metric/final gates remain unchanged;no new state/config.
Validate exact actual-corpus results/work,focused serial resource/curved tests and
ONE MPI6milestone. Only then full TWO-event same-seed M6. No cache benefit claim.
