# Terminal235 residuals and conservative geometry screening

The goal and all gates remain unchanged: two accepted same-seed corrected-L1
Euler50k meshes, conservative admissible transfer, finite adapted CFD and isolated
adapted disk restart; then BL/MPI and RANS500k. No accepted mesh. This continues
the terminal235 geometry/smoothing efficiency cycle, not a new operator strategy.

R=`integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57`.
A235=`R/m6_runs/euler_build235_geometry_coupled_real_cfd50k`.

## Bounded actual-cavity controls

The full terminal235 state contains5466shape-bad cells, of which14have two physical
faces: nine geometry-bad cores and five geometry-clean cores. All five clean cores
have complete four-star unions within the EXISTING64-cell bound (27–45cells).
Eligibility alone does not establish a missed repair. The optional production
four-star preflight excludes clean cores; ordinary single-star repair does not.

The existing build235 current_compound fixture tested all five clean cores and two
geometry-bad controls, using exact current nodal tensors and fresh reconstructed
associations. Cached q/J match exactly on all seven inputs; edge-oracle equality
is unavailable, while candidate edges are freshly checked. Session17179 terminal0,
135.958148s,40406assertions/1case, pins unchanged; all five owned host handles absent.

Useful ORIGINAL-star alternatives per clean core:3/4,3/4,4/4,3/4,3/4. Thus broadening
four-star preflight is NOT justified by this sample. Clean core212957481 has five
geometry-bad neighbors in its43-cell context; one useful candidate raises geometry
bad5→6 while reducing total certified excess. Preserve both counts and excess.

Neither geometry-bad control has a useful original-star or tested compound result.
Several shell/collapse/split/optimizer combinations improve q/J but increase whole
context geometry excess and correctly refuse. These alternatives overlap; do not
sum their improvements. No nonlocal publication or nonzero-height proof follows.
R/current235_residual_query_assessment.json retains every measured alternative.

## Query repetition and tolerance slack

Existing diagnostic tracing captures1304single-triangle,at-most64-facet misses;
all are distinct exact queries, across305support sets.918have the same support and
two fixed vertices as a previous query. Exact memo hits are absent from this stream;
28additional traversal completions have no traceable query header. Traced wall
times include instrumentation overhead and are not production timing evidence.

A sufficient motion proof is valid for a FIXED FULL immutable reference support:
if an old upper U plus maximum corresponding vertex displacement is<=tolerance,
the moved triangle is within tolerance. Association/feature/body/height checks stay
separate. This cannot be applied to moving restricted coverage or merely a cropped
prepared witness. It also cannot bound weighted excess above tolerance: area changes.

Exact rational squared-distance tests find17queries provable using their preceding
ROOT upper, only5215of17,941,808traced query-work units (0.0291%). This is a sufficient
subset, not a full upper bound on potential reuse. The existing raw-query replay
then tests six expensive nearby pairs,12exact inputs. Session76832 terminal0,
19.582161s,163assertions/1case, cached/repeated/recomputed answers equal; pins and
preparation inputs unchanged, all sampled owned PIDs absent. This short job ended
before start ticks were captured; no stop was sent. Raw replay does not test bindings.

NONE of the six pairs passes the full old-certificate motion proof. Displacements
11–85µm exceed positive margins0.0083–1.275µm (one old query is already outside).
Warm second-query work298329, traversal296498 (99.386%); exact repeats cost24–53work.
No slack cache or new retained geometry structure is justified from these data.

## Rejected bounded screening experiment236

The actual trace records279516finite-footprint graph attempts,81065covered;
coverage work8,875,210 and lift work3,508,298. These overlap query scopes. Source
review finds PlaneUnion::Upper always builds simplex-intersection constraints and
dual lift bounds even when its unrestricted plane residual already certifies a
piece within tolerance. Reuse that existing bound before the expensive tightening.

The small source experiment in CNativeAffineReference.cpp:

1. Compute the SAME outward-rounded residual/normal, horizontal-shift, approximation
   error and final nominal-triangle padding bound as the current final lift.
2. If this conservative bound is<=the unchanged tolerance, skip simplex clipping
   for that piece. Otherwise execute the existing complete calculation.
3. Charge the screening work; introduce no cache, allocation, config, query-identity
   change, association shortcut or construction/repair decision policy.

The unrestricted residual bounds the entire source hull, hence every source subset
that intersects this reference piece. Tightening is unnecessary if that bound,
including ALL error and padding terms, passes. A piece above tolerance follows the
old path. Max over pieces therefore preserves outside-bound values and gate decisions;
inside bounds may be looser but remain certified. A final inside score still has
zero geometry excess. Finite-footprint coverage, holes, seams, body/feature checks,
full support lower searches and final two-sided geometry certification stay intact.

Affected callers: the shared prepared TriangleUnion/SurfaceForest upper path used
by coarsening, splitting, smoothing/quality and final certification. Existing
BoundPhysicalSurface exact memo and transient/retained memory admission are unchanged.
Tests/exports/config formats need no new interface. Validity-first construction stays.

The SAME12query inputs were compared with saved235 answers/classification and counted
preparation/traversal work. All gate decisions match and both above-tolerance bounds
are identical, but work593437→594397 and traversal575272→576232 INCREASE slightly.
Session7208terminal0/163assertions,pins unchanged, all sampled PIDs absent. This
experiment is REJECTED and fully reverted to the exact235source hash before further
work. R/screen236_source.patch and screen236_assessment.json preserve it. No broad,
MPI or M6 campaign was spent on236. Wall-time differences are not a speedup claim.

## Narrow signed-bound redundancy experiment237

Source review of the same lift calculation identifies a smaller redundant step:
it always bounds both signed residuals, even if all three outward-rounded residual
intervals lie on one side of zero. For a source subset within the simplex, barycentric
weights are nonnegative and sum to one. If every residual upper is<=0, the positive
signed maximum is<=0; if every lower is>=0, the negative signed maximum is<=0.
The absolute bound already takes max with zero, so the irrelevant signed solve can
be omitted. Mixed-sign intervals use the original two solves. This requires no
motion approximation, cache, geometry envelope, changed candidate or new allocation.

Current237changes ONLY that selection around the existing maximum() calls, using
fresh intervals already computed for the source hull. Earlier236screening is absent.
Existing per-solve Work(3) is avoided only when the whole signed solve is avoided;
there is no counter renormalization. All support/footprint/association/padding/error,
requested-height, final geometry and construction constraints stay unchanged.

Build237passes,33.141690s/1302pins unchanged/both targets current, all compiler
handles absent. Same12input replay237passes163assertions,18.607487s,pins unchanged,
all owned handles absent. ALL12bounds are bit-identical235, including inside values.
Traversal575272→540961(-5.96%),totalwork593437→559126(-5.78%). Summed query timers
fall~7%, traversal~9%, but this short diagnostic is not whole-M6 timing evidence.
Keep this narrow correction for connected qualification: existing actual7cavity,
geometry/quality/failure-census serial groups now PASS:116cases/82379assertions,
serial237tool51486terminal0,372.369733s. All168actual-cavity outcome records match235
excluding timing/work. ONE MPI6milestone passes26cases on EVERY rank,tool19231
terminal0,6.322127s. Pins unchanged; all21serial and10MPI sampled handles absent.
R/signed237_qualification.json records scope/provenance. IMMEDIATE next is the full
same-seed two-event M6, which later finished237/tool2688/source4d1e2ec5b0.
See ADAPTATION_BUILD237_OUTCOME_20261107.md for terminal outcome and
ADAPTATION_CAVITY_GEOMETRY240_20261107.md for the current residual diagnosis.
R/m6_237_launch.json records exact owned handles; preserve the frozen attempt.
Judge whole-field q/J net removal/s and geometry counts at matching quality ages,
not query time alone. Source remains frozen throughout build/validation.

If237does not save useful work, preserve/refuse it rather than layering
another fallback. Quality-aware construction, larger cavities, a scheduler or new
geometry representation are not selected. Literature remains design context only.

After shape closure, use existing shape-preserving geometry/size phase, then required
shape reclosure after any validity-only reconstruction, within the SAME budgets.
This phase was not reached235; neither these local controls nor a cheaper query
establish its closure, transfer, adapted CFD or restart.
