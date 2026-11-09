# Post-refinement metric cleanup — bounded investigation

Latest full289 is TERMINAL REFUSED after complete736323cell census with zero
q/J/LOCAL-geometry/size failures and no unknown measurements. Mandatory final
global two-sided geometry certification hit internal storage admission
(13,482,375,068 requested versus12,884,901,888 ceiling), then the time wrapper.
All24nonmanifest payloads are SHA-identical287. Integrated
cleanup273 NEVER ran in274,278,283,287 or289; its global leading-edge effect remains
unmeasured. All11actualHOST handles absent and all1520pins unchanged at terminal.
No run was interrupted for cleanup review. See HANDOFF_Codex.md and
[prepared-source investigation](ADAPTATION_PREPARED_SOURCE_CERTIFICATES_20261108.md).
Current source/reverse evidence demonstrates expensive proof traversal and
resolution-keyed target preparation/storage, not a new
cleanup eligibility failure. No cleanup extension selected in this certification cycle.

Confirmed: the original selection/zero-shape-debt admission gap permits short
edges to survive; bounded tests establish four local feasible point removals,
not the complete cause of leading-edge over-refinement. Existing273cleanup is
one conditional opportunity after stalled shape repair, NOT unconditional
post-refinement cleanup of an otherwise accepted mesh. Thus even a future
accepted mesh need not have undergone short-edge cleanup. No claim of mesh-wide
leading-edge improvement follows from its integration or passing fixtures.

The present demonstrated blocker is final geometry certification work/storage. Do not
widen cleanup policy or insert a competing experiment before resolving that
blocker. Reuse complete287capture if later outcome warrants a matched whole-field
cleanup assessment, including complete neighbors, directionality, strict q/J,
geometry, heights, work and net useful progress. A smaller mesh alone is insufficient.
The requested around-nose resolution and excessive spanwise resolution remain
separate; no blanket flow-metric minimum edge or insertion-distance gate is justified.

TWO accepted Euler50k meshes, conservative admissible transfer, finite adapted
CFD and actual isolated adapted restart remain required; then BL/MPI and RANS500k.
All gates, budgets, validity-first construction and CAD optionality unchanged.
Older staged notes below preserve chronology, not current job status.

R=`integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57`.
A262=`R/m6_runs/euler_build262_compact_capture_mach_only_real_cfd50k`.
Evidence `A262/post_refinement_cleanup_selection_v1.json`, prepared by
`R/select_metric_cleanup_contexts.py` in8.945476s with one low-priority process and
single-threaded libraries. This is saved-data inspection, not an operator run.

## Confirmed source behavior

Production CNativeRemesher3D.cpp:579 calls AdaptMetric, which invokes the
bidirectional Adapt lifecycle (CNativeEngine3D.cpp:5683). It starts with short-edge
construction coarsening. Coarsen/Refine/Repair public wrappers and the serial
RefineOptimize helper are separate entrypoints; RefineOptimize does Refine then
OptimizeMetricMesh, not another Coarsen. The production per-cavity optimizer is
OptimizedRepairPatch:1083–1097, reached from the quality/size repair ladder and
geometry-split shape reclosure. No unconditional mesh-wide post-refinement
OptimizeMetricMesh pass exists in that production driver.

General Improvement→Construction returns can select coarsening, but this does
not prove that M6 receives cleanup after geometry insertion:
- enter_phase:2824–2852 changes to collapse direction only when !refine_geometry;
  refine_geometry instead calls begin_length_generation(false), i.e. splitting.
- At3054–3063 a complete size/height-clean geometry residual selects that split
  return. construction_invalid includes geometry: it is initially computed from
  maximum edge at763, then AND !geometry_invalid at852. It is NOT only long-edge debt.
- The completely clean Improvement acceptance at3167–3169 does not require a
  subsequent short-edge pass. Short edges are not a final acceptance failure.
- Selection::Queue:263–269 does select edges<.9 when coarsening, but repair_event
  disables this queue. Size repairs cover oversized edges, not short edges.
- Preserved267 rank0 trace records one collapse pass and27split passes. Later
  shape/geometry returns take splits. This is actual opportunity evidence; it
  does not prove why any individual short edge survives.

MeshOptimizer::Run:6773–6818 selects shape/height-bad cells, with a separate
long-edge loop. It has no short-edge loop over otherwise acceptable cells.
TryCell:6628–6656 invokes Collapse through that bad-cell selection. With mapped
geometry, legacy same-facet BoundaryCollapse is not dispatched there; general
curved CollapseVertex is available in the distributed engine instead. Legacy
BoundaryCollapse also requires a planar single-facet fan and is not a general
curved cleanup substitute. CoupledBoundary:6679–6690 requires a shape-bad core.

In production ValidityOnly admission, DebtMode is true once requested heights
are satisfied (5888–5893). OldMerit/FreshMerit:5895–5907 use negative nonnegative
shape hinge debt. For an already shape-clean patch both merits can be zero, and
Progress:5909–5913 requires strict debt decrease unless explicitly length-driven.
Collapse:6118–6130 passes no length-driven allowance. In addition, Index:5822–5829
marks the cavity skin, and Movable:5937–5942 excludes those vertices from ordinary
Collapse. Physical boundary movement remains supported through SmoothBoundary;
this does NOT mean globally fixed physical coordinates. Fixing zero-debt merit
alone would therefore not enable curved boundary collapse through that dispatcher.
The existing generic CollapseVertex is the relevant production primitive to test.
Therefore a shape-clean,
shape-preserving collapse cannot satisfy that local merit gate. Absolute/height-
repair modes differ and must not be silently changed by a correction.

This establishes an eligibility/admission limitation, not admissible desired
collapses or the complete cause of over-refinement.

## Reproduced leading-edge delivery, before operator claims

A262/leading_edge_delivery_reproduction_v1.json reuses the existing actual-face/
production-edge analysis with explicit unknown-q/J handling. Same original-grid
station definitions as247: triangle counts262/247 are554/472,540/459,527/488,
560/527,566/549, i.e.3.10–17.65%more. Median span/nose geometric triangle extents
are1.537,1.526,1.509,1.582,1.725. Exact cached production face-edge medians are
span-dominant .1594,.1459,.1654,.1552,.1342 and nose-dominant
.1745,.1690,.1979,.1940,.1868. Repeated face edges are explicitly counted as such,
not unique global edges. Regional geometry/direction bins remain diagnostic.
The side's requested-spacing tensor ratios remain nodal restrictions rather than
new exact geometry certificates; no pointwise absence of metric drift is claimed.
Four unknown q/J cells remain explicit. The accompanying feature caveat preserves
feature_known: false near_feature alone does not establish no genuine features.
No reduction in geometric resolution is authorized by these distributions alone.

## Actual bounded clean contexts

The full262 mesh remains148608points/734315tets, four geometry-bad cells and four
UNKNOWN q/J cells. It is NOT globally shape-certified. The following contexts
were independently selected from its sealed full connectivity and paired cached
metrics; every cell in each selected context has known q>=.2,J>=.05, clean geometry,
clean size and no height failure. Full global four-vertex stars fit the existing64.

| Role | Core cell | Cells | Selected edge | Exact cached metric length |
|---|---:|---:|---|---:|
| Spanwise LE, y≈.4 |147878143|43|178273858–167233426|.1529790573|
| Spanwise LE, y≈.8 |148859138|20|190319017–196201389|.1298129068|
| Around-nose control, y≈.6 |148245385|46|189602444–191247152|.1217060958|
| Smooth wing clean control, y≈.6 |145854723|35|192234610–16709|.3100832908|

All selected face associations are WING3D/marker2,body0,sheet10154. Both endpoints
have recorded mobility dimension2. This is not a contraction permission proof.
Spanwise edge alignments are.912/.949; the around-nose control alignment is.349.
These directions are diagnostic physical projections, not geometry certificates.
The whole-context minimum q values are.205833,.201225,.201658,.241415; minimum J
values.065163,.050546,.057126,.109139. The LE contexts are close enough to gates
that volume-quality obstruction is a real possibility. Original exact edge
quadrature is retained; endpoint tensor estimates were used only for selection.

The existing42-cell geometry-residual control is separately qualified by the
source-faithful current-cavity fixture:39known q/J oracles and3unknown cached
oracles, with freshly recomputed local shape clean. Its one constructed collapse
fails J before retained-star optimization and q afterwards, while geometry bad
remains4. This is a negative control, not a feasible cleanup demonstration.

## Existing insertion checks and paper scope

Audited main split paths project/bind points, require distinct identities/realized
coordinates, exact-positive cells, complete interfaces, feature/body/geometry/
height preservation and changed-edge upper bounds. The local cone placement also
has endpoint-region and shortening bounds. No all-neighbor flow-metric minimum-
distance screen was found in those paths. These checks are not CDT3D's general
anisotropic proximity filter; they also do not prove its absence caused crowding.

CDT3D §§3.2–3.3 describes metric-space proximity and pre/post collapse; §4 keeps
geometry associations and distinguishes surrogate geometry from CAD association;
§5.4 combines flow and geometry demands. Its criteria are not our strict two-sided
geometry gates. Its iterated M6 result is not a first50k benchmark; AppendixA's
metric lengths do not justify a blanket flow-metric minimum-edge gate. Geometry
may require short around-nose edges. L2 recovery projection differs from the Lp
error objective. No CAD requirement, geometry representation or target change follows.

## Next bounded checks, after frozen271 is terminal

Isolated input preparation is complete inA262/post_refinement_cleanup_inputs_v1:
four cases, exact cached q/J/edge oracles, current global boundary bindings/skin
and the unchanged native reference. Existing preparer reused,734315paired rows,
34515boundary points/69026physical faces,46.351289s, all input hashes unchanged.
No operator/solver/test ran. This new directory does not replace any pinned
current_witnesses_v1 files. R/prepare_metric_cleanup_inputs.py records the exact
transformation and provenance.

Reuse the actual-cavity fixture and production current-nodal import/transport.
Use these four contexts and the existing negative control, current64/160/30s
support/work/time bounds, and immutable outside interfaces. No larger cavities.

1. Reproduce all known old q/J and exact edge oracles; freshly certify old geometry.
   Keep unknown values explicit and verify complete supports/actual associations.
2. Run unchanged optimization as a selection control, then both directed generic
   CollapseVertex proposals on each selected edge. Test existing reconnection/
   relocation and bounded retained-star optimization where relevant.
3. Distinguish no selection, zero-debt progress refusal, exact positivity/topology,
   association/body/feature/height, genuine geometry regression, conservative
   certificate refusal and changed-edge/metric-size regression. Source conclusions
   cannot stand in for these outcomes.
4. Join each result to the COMPLETE context and neighbors; transport current nodal
   tensors onto candidate connectivity through existing production APIs. Compare
   q/J removed/introduced, geometry bad faces/cells/excess, requested heights,
   exact directional edge distributions, point count, work and assessment time.
   A smaller mesh alone is insufficient. Do not credit overlapping timers twice.

A separate exact binary64-coordinate screen reused prepare_boundary202.exact_positive
on both directions of the four selected edges (A262/post_refinement_cleanup_positivity_v1.json).
Seven of eight proposals retain positive tetrahedra and have no duplicate whole-
context tetrahedra. Smooth-control192234610→16709 creates one nonpositive cell and
is therefore blocked before quality/geometry admission. Both spanwise LE edges
pass this limited screen in both directions. These are hypothetical endpoint
rewirings, NOT executed production operators or feasible certified replacements;
geometry/association/shape/metric/height/interface/contact remain untested.

At initial selection no feasible replacement was established; diagnostic272 below now proves four local removals. The existing production-source
fixture currently expects geometry-progressing cases; its test-only extension for
clean metric cleanup must preserve old behavior and wait for frozen source release.
Choose ONE production correction only if these outcomes plus271's actual terminal
blocker justify it. Reuse selection/ownership/reservation/invalidation/accounting/
publication; preserve acceptable shape without first creating artificial shape
failure. Otherwise retain this hypothesis and return to the demonstrated blocker.
Qualify affected serial behavior and ONE MPI milestone, then immediately full
same-seed TWO-event M6. No new configuration/scheduler, raised budgets or relaxed gate.

Shock target allocation remains separate: sampled demand≈4–6%chord versus existing
≈3–4%triangles. Cleanup does not establish sharper shock demand. Further recovery,
norm, gradation and geometry-metric hypotheses remain in the linked backlog.


## Frozen271 outcome and current272 diagnostic

271native event1refused on operator_stagnation atnative3378.395280s with735624cells,
1q/0J/0geometry/0size and completeflags. Soleq cell152833287 is root-adjacent
(y≈.035),interior withtwo boundaryvertices; q=.198870799615863,J=.060471062672186514.
Its14repairfeatures areblocked. This is not a speed/deadline/geometry-failure
terminal blocker. Four distant clean contexts cannot by themselves prove a cure
for this rootcell. Use feasible cleanup outcomes PLUS its actual state before
choosing production behavior.

All30cappedrawfilessealed;rank4ownedfilecapped,644221/735624rows present. No full
271mesh or globallycomplete current support may be inferred from this partial
archive. Cap remains128MiB/file. One exactowned stop sent onlyafter explicit
refusal/census/pins/raw evidence preservation; all11handles terminal. No repeatstop.
272changes ONLY the existing fixture's new hidden clean-cleanup mode. Production
CFD is byte-identical271. It retains the old fixture modes and uses complete262
contexts, fresh geometry, production nodal transport, old/new wholecontext gates,
exact directional/short-edge distributions and existing160work/30s optimization.
Diagnostic272 below proves four local replacements; connected production cleanup is still pending.


## Diagnostic272 result — local replacement feasibility established

Build272passed209.224824s; CFDbyte-identical271. Focused serial1case/43587assertions
passed98.641478s, allpinsunchanged and allhandles terminal. 144knownq/J oracles and
all864cell-edge oracles reproduced within1e-10; no unknown selected scores.
R/metric_cleanup272_diagnosis.json seals results/provenance. No production fix yet.

Allfour unchanged clean-context optimizer controls makeZEROtrials. Eight directed
collapse trials: sixconstruct,oneentity-support refusal,oneexactpositivity refusal.
Three raw contractions (lowspan reverse,highspan forward,smoothcontrol reverse)
removeonepoint with q/J0,geometry0,size/interfaceclean and strict Coarsen metric
validation. Lowspan forward initiallycreates1q/1J but ONEexistingoptimizertrial
recloses shape withgeometry0. This is a private combined candidate, not published
artificial deterioration. OrdinaryRepairmerit refuses all useful clean-clean
replacements for lack of shape/oversize-debt progress. Fourunique local removals
are therefore demonstrated; nonlocalcontact/distributedpublication remain untested.

Wholecontext short-edge deficits decrease39.824→36.077 (lowspan raw),
39.824→36.371 (lowspan compound),16.540→14.026 (highspan),6.951→5.462 (smoothcontrol).
These are DIAGNOSTIC sums max(0,.9-length)^2 over unique production metric edges,
not a new hard gate. Physical span/nose extent medians move1.45→1.88/2.11 and
1.53→3.32 for the two leading-edge cases; smoothcontrol1.11→1.28. Strict certified
maximum face errors stay<=1e-4. Opposite highspan and nose directions yield TWO
geometry-gate failures even where shape recloses; physical regression versus
conservative certification remains unresolved, so those candidates are rejected.

Next target is connected cleanup/admission REPAIRPOWER, not more time/work/RAM:
271stagnated with oneqcell, geometry/J/size closed, work2.49Mof19.41M and native3378s
below5400. Do not infer that far-away clean samples cure that rootcell. Reuse
existing selection/ownership/reservation/invalidation/publication/Coarsen metric
validation to supply justified clean short-edge opportunity without changing
validity-first construction or requiring published shape failure first. Keep
scope bounded and inspect the actual root dependencies where complete support can
be proved. Qualify affected serial behavior and ONE MPI milestone, immediately
full same-seed TWO-event M6. All strict gates and completion obligations remain.

## Connected production correction273 — implemented, not yet built/qualified

Missing capability: a completed quality census can have q/J demand with all
current shape features blocked while geometry/size/heights are closed, yet
known-clean short-edge stars have no service opportunity. 271 proves that terminal
state;272 proves admissible raw clean contractions on actual262contexts. These
remain separate evidence: no claim that those distant collapses cure the rootcell.

Use ONE finite improvement-only cleanup opportunity after shape choices exhaust.
Reuse existing directed short-edge selection, complete stars, full-vertex claims,
version checks, private mapped associations, clearance, parameter replacement,
current-nodal transport, accounting and publication. Keep Phase::Improvement so
q/J are measured; ordinary construction remains edge-only/validity-first. Only
this cleanup uses absolute fresh q/J plus ordinary Coarsen changed-edge size gates
and strictly decreasing complete unique-edge short deficit. This is an objective,
NOT a hard minimum-edge gate. Geometry/feature/body/height gates remain in their
existing callers. Raw candidates failing shape are refused; no new optimizer
trials or cavity expansion added. Compound272success remains a later measured
possibility, not an extra production fallback.

After this immutable-frontier pass, re-enter the existing quality lifecycle;
only actual changed supports reopen blocked repair features. A one-shot marker
prevents another cleanup cycle, budgets remain monotone, and no ownership change
is attempted under active claims. Existing quiescent quality redistribution
remains responsible for ownership; no new scheduling/partition policy. Strictly
clean meshes accept immediately without optional cleanup. This targets the
demonstrated stalled quality state and does not promise blanket LE triangle
reduction or solve shock target allocation.

Validation scope: shared gate against clean zero-debt contraction, fresh q/J
failure, oversized changed edge, missing measurement and short-deficit regression;
actual four272contexts now call that SAME production gate. One aligned no-progress
cleanup/strict-reassessment control uses a genuine-feature sliver and empty MPI
ranks, alongside a clean control. Existing curved/seam/body/height, reservations,
rejected-history, census/migration and resource cases form ONE MPI6 milestone.
Then full same-seed TWO-event M6 immediately, judging whole-field q/J/g counts
and useful removal/work/time rather than local point removal alone.

273buildpassed516.729s/allpins. Focused3cases terminal1/43684of43685assertions:
actual4M6contexts/sharedgate and alignedphasecontrols PASSED; sole newunitcheck
mutated a cached length but retained identical coordinates/identity, so shared-edge
cancellation legitimately bypassed it. Correct the synthetic check to move an
endpoint too. Production unchanged. Allfocusedownedhandles verified terminal
before this test-only correction. Preserve cleanup273_focused_diagnosis.json;
274will rebuild onlythat test/relink, then qualify the connected production cycle.

274test-onlyrebuildpassed27.108s/allpins, productionCFD BYTE-IDENTICAL273. Focused
retry PASSED43685assertions/3cases97.370s/allpins, allownedhandles terminal. Shared
productiongate retains3raw strictlocal removals plus compounddiagnosticcontrol;
alignedphase/refusalcontrol passes. Connectedserial LIVE tool72355; noMPIyet.

## Connected qualification and return to full M6

274qualification sealed inR/cleanup274_qualification.json: focused43685/3cases,
serial62978/83cases, ONE MPI6 117cases EACHrank. Allpins/actualterminalhandles
verified. Source-reviewed production change plus positive actual-cavity gates and
aligned no-progress branch are qualified; successful cleanup PUBLICATION/reclosed
quality remains an actualM6integration obligation, explicitlyfalse inqualification.
No gate orbudget changed. f2fd9c2c9d pushedthe implementation/focused checkpoint.

Fullsame-seed TWOeventM6 build274 NOWLIVE tool80377. All1520pins and83actualmapped
libraries verified; exact11ownedidentities inA274/owned_startup.json. SameconfigSHA
5ad2dc1d9a1cd8bff87f633617b191b50defbc261ab3d0b2ed0987a34722b89e.
verify_m6274_initial.py confirms all61330positions/22fields BIT-IDENTICAL256 and
qualified254target, SENSOR_MACH only. Source/binaries/helpers/runtime/inputsfrozen.
No actualcleanup invocation/outcomeoracceptedmeshyet. Do not interruptwithout
preserved authorizedevidence. Retainfulltransfer/adaptedCFD/isolatedrestartgoal.

For comparison, cleanup commits are included inlength_collapses/construction_edits;
PRIVATEqualitybusy/laddercommits retainexisting repair-onlyscope. Use stage labels
andfullwall/CPU/geometrywork counters to accountcleanup, rather than calling its
zero privatebusy idleCPU. Judgeglobalnetq/J/g and finalcertifiedoutcome, notpoints.

###Frozen274 long quiet tail — source-level hypothesis, NOT live-stack proof

After qualityentry25035/native3437.224s (1q/1J/0g/0size), no newercomplete
census was visible bywall4842s. SampledRSS grows similarly on ALLsixranks, even
near-emptyowners; jobhandle80377isstilllive, no stop/pressure demonstrated. This
doesNOTprove the workingmesh stillhasthatbadcell: finish() inCNativeEngine3D.cpp
1840 performs GeometryCertificates(atlas_budget,true) BEFORE exit telemetry1918.
BoundaryParameterState::GeometryCertificates at368 groups the ENTIREreplicated
workingphysicalsurface bybody/sheet/marker; NativeMappedGeometryBackend3D.cpp263
calls BoundSurfaceUnion forBothdirections. Thus finalglobalcertification is a
plausiblequietstage onceinternalshapehasclosed, but thereisNOdynamicproofyet.
Use the actualexitq/J/gflags, reasons and per-rankquery/guidance/optimizercounters
todistinguishfinalcertificationfromlongprivateqrepair. No perf/GDB/newbaseline.
Localcachedgeometry0 isNOTa substitute for this mandatory complete two-sided
proof. No geometrydirection/gate maybedropped onthebasis of thishypothesis.

### Actual terminal274 supersedes the quiet-tail hypothesis

Exit25036/native5342.591765s is COMPLETE734806cells with zero q/J/local-geometry/
size failures and all measurements known. Native refusal is elapsed-time exhaustion
in mandatory final whole-surface two-sided certification. Cleanup NEVER executed.
From entry25035 to exit each rank adds one full query/~1903.6s,~651–659M traversal
work but <.4s preparation. Rank3 alone commits one extra repair (+22work,+6tets).
Thus final traversal is demonstrated; preparation or continued polishing is not
the terminal bottleneck. Local geometry zero is not the global two-sided proof.

Full30file snapshot saved, all734806cells/metrics known, pins verified; exactly one
owned MPI stop, all11prior handles absent. Full rejected mesh+tensor export now
running; independently validate it before sharing. Goal still ZERO accepted.
Investigate existing N=3 finite-footprint bounds versus N=10 model-source hulls;
a rigorous flat-triangle plus Bernstein-deviation witness is only a hypothesis.
Retain original both-direction certification/fallback and source-envelope import
semantics. No broader cleanup changes justified by a pass that never ran cleanup.
