# Existing projected-envelope tightening —191terminal analysis and next design

Status:193proof/serial/MPI6 PASS; full real-CFD M6 TERMINAL, natural time-budget
rejection. Complete EXIT8628q/5768J/3151geometry bad; no accepted adapted mesh.
All10owned handles absent/all1401pins unchanged before further analysis.
[Terminal analysis](ADAPTATION_GEOMETRY_CERTIFICATION_20261101.md) is current;
no further C++ change or measured speedup from the new alternatives.
Analysis/proof completed before implementation. One active connected
geometry/quality optimization cycle. Preserve191bounded memo retention,190ownership/
history and all latest validity-first construction improvements. No new options,
scheduler,repair slots/local passes,time limits or acceptance thresholds.

## Missing capability and measured blocker

191full two-event real-CFD M6 first event naturally rejected on ALL6ranks at the
unchanged5400s time budget (native max5403.563701s). Complete EXIT787840workingcells,
16191q/12965J/3509geometry,size0;0accepted meshes/transfer/adapted restart. Quality
4277.122739s,28245publications,37880netq/40412netJ removed. Queue192741,only9stuck
cells,missing0; active repair demand and positive yield remain. Next target SPEED
of useful closure, not a new construction fallback or a claim repair power is solved.

Whole quality traversal6.1464Bwork/4941.399summedranksec; upper queries596363/
5423.344ranksec. These timers overlap. Facet/scope preparation9.801Mwork is much
smaller. Boundary private smoothing218104upperqueries/3804.809ranksec vsinterior
0queries/137.412ranksec; private moves/debt are not published cavity gains. Remaining
q13743interior/2448surface; interior means no marked physical face, not no boundary
vertices. New exact union memo303285hits/293078misses; misses alone cannot identify
nearby positions versus evicted identical queries. Retention helped~3%traversal at
last exactly matched190state; it did not close M6. Source/binary/config/input and
all10handles audited terminal/all1401pins unchanged at1793482215 BEFORE any edit.
Evidence in staged57/m6_runs/euler_build191_real_cfd50k_numeric_memo: terminal_assessment,
terminal_geometry_service_assessment,terminal_three_build_comparison and terminal_handles JSON.

## Analysis first: reject the unsupported shortcut

BoundPhysicalSurface returns a rigorous ONE-SIDED physical-patch→model upper bound,
including when its value is above tolerance (then inconclusive). Exceptions produce
no result. Accepted-face association/coverage certificates are distinct from numeric
bounds. Existing exact numeric keys,prepared-facet/scope pools,old-side star merit,
shape/edge/height screening and nonnegative partial guidance screens already exist.

A prior upper U plus maximum paired vertex displacement D rigorously bounds the
new affine triangle against identical immutable support, by distance-to-set being
1-Lipschitz. This does not prove association/feature/body/height/reverse coverage.
Offline exact-dyadic parsing of the EXISTING12projectedM6queries found SIX same-
support/same-face prior pairs but ZERO certifiable newer queries: movements
1.51e-4..3.02e-4 exceed available slack1.98e-6..5.14e-5. No repeated reordered
identical geometry was found either. saved_query_motion_bound_analysis.json includes
wire/provenance checks and exact-rational squared-norm verification. Older168inputs
are not the current191terminal population; no production speedup estimate follows.
Do NOT add a motion lookup/index/cache on this evidence. Keep this alternative in backlog.

## Existing envelope and connected limitation

TriangleUnion::PlaneUnion::Upper ALREADY has a finite projected footprint, conservative
lift bounds, and a source-simplex box dual relaxation. Do not add a duplicate envelope.
It currently demands coverage of an axis-aligned projected RECTANGLE and relaxes
three coupled target halfplanes into separate barycentric-coordinate bounds. Both
are conservative but can force repeated physical-domain subdivision for a triangle
that is covered by the existing graph. Tighten this existing shared mechanism.

This reaches coarsening,splitting,quality and boundary smoothing through the SAME
BoundPhysicalSurface/QuerySurfaceUnion path; no new star/workspace API is required.
Existing prepared geometry/index ownership,scope identities and bounded pools remain.
Construction/operator decisions keep all existing validity/association/feature/body/
height checks; positions and metrics remain fresh. Rejected-star invalidation and
complete-census ownership/publication semantics are unchanged.

## Proposed proof and implementation boundary

1. Keep current inexpensive projected rectangle/segment certificate first. If its
   coverage fails for N=3, try the entire nominal projected SOURCE TRIANGLE using
   existing hole-aware FinitePlaneReference::ContainsFace and exact orientation.
   Do not test just vertices,fill a convex hull of target gaps or join bodies/seams
   by rounded proximity. Collinear/nonfinite/uncertain cases retain the old path.
2. Nominal source vertices C_i=BoxMiddle(B_i) are only an optimization device. Bound
   delta=max_i sup_(p in B_i)|p-C_i| with existing outward interval NormBound. Every
   convex source point has a nominal counterpart within delta. Compute the existing
   lift certificate on the nominal source hull and add delta OUTWARD exactly once.
   If this cannot be proved/admitted, use existing rectangle/traversal behavior.
   Keep N=10 curved-source/reverse-direction behavior unchanged.
3. Tighten the existing N=3 residual bound with the ORIGINAL relaxed halfplanes,
   not another geometric search. For feasible lambda>=0,sum(lambda)=1, every relaxed
   edge row a_e satisfies a_e dot lambda>=0. For ANY finite multipliers mu_e>=0,
       c dot lambda <= max_i(c_i + sum_e mu_e*a_ei).
   Evaluate this bound with outward interval arithmetic for positive and negative
   residual objectives. Minimum of valid upper certificates stays valid. Candidate
   multipliers may be approximate: only nonnegativity/finite values and outward
   evaluation matter for soundness, not exact optimizer solves. Use bounded one-
   and two-edge analytic candidates; nonfinite/ill-conditioned choices are skipped.
   Retain existing simplex-box bound and avoid extra work when it already certifies.
4. Keep all existing final checks,reference/support identities,reverse coverage,
   fresh associations/features/body separation and requested heights. No tolerance
   change or sampling-based acceptance. Stronger valid upper bounds may resolve
   previous inconclusive queries; that is not relaxing a geometry gate.
5. Account added local scratch/overlap and bounded work with AtlasOps; preserve sticky
   resource failure and all memory/work/time limits. Add no persistent index/cache or
   unbounded metadata. Reuse existing traversal/preparation/smoothing counters.

This is a connected candidate, not an established speedup. Check actual code/proof
again before editing. If saved-query work does not improve, diagnose this same
certificate limitation before a broad campaign; do not launch another old-code M6
baseline or add unrelated fallbacks. Unproven stronger graph/refinement alternatives
stay in backlog unless they address the demonstrated blocker.

## Acceptance and immediate complete attempt

Focused serial proof cases: a finite triangular graph whose source triangle is
covered while its rectangle is not; noncoplanar graph/piecewise-height residuals;
interval uncertainty and degenerate projection; holes,separate bodies,genuine
features; cached versus recomputed answers and work/storage exhaustion. Reuse the
12savedM6queries with exact payload/reference hashes to measure traversal and
preparation work; no new perf/GDB/baseline campaign. Passing those is not completion.

Then one connected serial/MPI6 milestone for affected geometry,curved associations,
heights,conflicts/empty ranks,dependency/publication,migration flags and cached census.
Immediately return to full TWO-event M6 with SAME saved real-CFD seed,50k/native5400,
strictgeometry1e-4,q>=.2,J>=.05,maxedge1.8/heights. Compare net whole-field q/J/geometry
counts and useful progress at actual matching work/stages,with host contention stated.

Existing phase plan: after complete q/J/height closure, enable residual size repair
if required; when size is clean, EnableGeometryRepairs queues geometry-only demand
with strict complete-patch guidance progress. Geometry-only repair or a valid
return through construction can change shape, so remeasure/reclose shape before
acceptance. This is existing behavior, not established M6 closure: build191 never
reached the geometry-only entry. Preserve all checks and cumulative budgets.

Shape closure → residual geometry/size closure → shape reclosure remain required
under SAME cumulative limits. Require TWOaccepted meshes/native_ref pairs,actual
conservative admissible transfer,finite continued CFD and isolated adapted disk
restart. Then dedicated BL/constraint/MPI qualification and corresponding RANS500k.
CAD optional; no-CAD geometry authority remains sufficient throughout.

Build192 focused proof PASS307assertions/2cases; all handles terminal/pins unchanged.
Existing12saved queries: traversal401486→384426 (-4.25%);8queries much cheaper,4more
expensive. Cached/recomputed results equal; no full-run speedup established.
Build193 refines only when coupled proposals are attempted: skip if existing
objective upper already certifies tolerance. Repeat replay, then connected
serial/MPI6 qualification and immediate full two-event M6. Plans and primary review
are R/geometry_envelope193_validation_plan.json and geometry_envelope193_source_review.json.

193qualification: proof307assertions/2cases; serial20643assertions/68cases; MPI6
216cases on EACH6ranks. All exact host handles terminal/pins unchanged before
full193M6. Saved-query traversal401486→379146 (-5.56%);4hardqueries still regress,
so net whole-field closure benefit remains unproven. Same saved CFD/actual original
continued restart/config/geometry/mesh verified against191. See current HANDOFF
for actual active handles and source freeze; never use historical IDs from this file.
