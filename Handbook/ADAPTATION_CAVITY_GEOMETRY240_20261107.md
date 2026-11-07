# Actual one-face residual: certified geometry violation and reconstruction question

Goal unchanged: TWO accepted same-seed corrected-L1 Euler50k meshes, conservative
admissible transfer, finite adapted CFD and ACTUAL isolated adapted disk restart;
then BL/MPI qualification and RANS500k. ZERO accepted meshes. Geometry100µm,q>=.2,
J>=.05,maxedge1.8,heights strict. Validity-first construction remains unchanged.

R=`integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57`.
A237=`R/m6_runs/euler_build237_signed_bounds_real_cfd50k`.

## Evidence that selected this diagnostic

Full237naturally refused at native5347.003818s with COMPLETE final census:
1020222cells/296q/327J/1366geometry/0size. First quality pass exhausted44q/17J,
46shape-union all blocked. Surface reconstruction reduced geometry1656→1379 but
fresh shape entry became410q/493J. No geometry-only phase was reached. See
[237outcome](ADAPTATION_BUILD237_OUTCOME_20261107.md) for the complete workflow evidence.

Five ACTUAL final contexts with complete four-star support were tested. Three have
useful original-star controls. One-face core230222079/52cells has no net q/J gain
from any of its four original stars. Bounded238diagnostic coverage then tested
existing directed collapse plus retained-star optimization: only291052210→7876
constructs out of12directions. Its whole52-cell context becomes50cells,4q/3J→4q/2J,
geometry-bad6unchanged, but geometry excess1.7369589e-12→2.8218985e-12(+62.46%).
The optimizer makes NO further change. It is correctly refused by current merit.
No production integration or support expansion selected. All33prior controls remain
unchanged. Detailed results A237/current_residual238_assessment.json.

## Exact face replay, not a new M6 or synthetic trial

CollapseVertex retains each physical face's facet, replaces the removed ID with
the retained ID and drops faces containing both. Because the optimizer made no
change, existing prepare_shell227_queries.py's wire format can export the exact
four old and two collapsed physical faces using captured coordinates/bindings.
R/prepare_collapse238_queries.py asserts these preconditions and before/after hashes.
All six anchors are16734; no nearby positions were invented. Source/input receipts
are in A237/collapse238_geometry_queries_v1/geometry_query_preparation.json.

The existing geometry_current_queries fixture validates fresh entity associations,
discovers supports in C++, and checks cached/repeated/recomputed bounds agree.
Production's direct correspondence bound can be much tighter than raw one-facet
union output; score reconstruction MUST use production_guidance_upper.

| Query | Kind | Physical nodes | Production upper µm | Whole-component outside lower µm |
| --- | --- | --- | ---: | ---: |
| 0 | old | 7876,291052210,7868 | 414.1053 | 248.3343 |
| 1 | old | 291052210,121219186,7868 | 190.1954 | 121.9588 |
| 2 | collapsed | 7868,7876,121219186 | 414.1053 | 248.3343 |
| 3 | old | 7876,14236978,291052210 | 51.9356 | Not probed: upper passes |
| 4 | old | 291052210,14236978,121219186 | 59.9050 | Not probed: upper passes |
| 5 | collapsed | 7876,14236978,121219186 | 132.6493 | 112.3300 |

Area-weighted squared excess over100µm for these faces recovers the native candidate's
whole-context geometry delta:1.084939619280091e-12 versus1.0849396192800913e-12,
relative difference3.73e-16. Unchanged faces cancel. This establishes the attribution
for this CURRENT candidate, not historical execution/publication replay.

## Rigorous outside witnesses; no sampling-based acceptance

Only the opt-in geometry diagnostic changed. For a refused query it probes15dyadic
barycentric locations using exact ParametricPointBounds, then existing ClosestCurved
on the whole hinted edge-connected same-marker cubic component. The interval lower
is reduced by a rigorous bound on displacement between the exact triangle point and
its rounded probe. It is also reduced by max(authority.Errors())=1e-11as an OPTIONAL
extra conservative margin. Every subtraction rounds downward; clamp at zero.

ClosestCurved and surface-union geometry use the SAME stored cubic net authority.
Surface-union triangulation error encloses its numerical approximation, not an
independent CAD uncertainty. The extra1e-11margin is deliberately conservative,
not a missing term in the closest-distance kernel. No separate CAD truth is claimed.
The queried whole component contains the permitted local supports; a lower distance
to this superset above tolerance therefore proves the face violates tolerance.

Closest-query work is capped16384per probe,8GiB workspace ceiling,absolute error
100µm/64. Resource exceptions are recorded UNKNOWN, never acceptance. At most12
queries/15samples each; no production decision or work/budget counter changed.
All60probes completed;16prove outside across the four refused faces. Unsampled
locations remain unclassified; a lack of sampled violations cannot certify a face.

R/assess_collapse240.py independently checks exact rational barycentric membership,
exact rational squared source-point displacement<=reported roundoff², and corrected
lower<=the exact rational subtraction including extra margin, plus score/churn/source
joins. A237/collapse240_geometry_assessment.json records the result. This is stronger
than treating floating samples alone as geometry proof.

Build239terminal0,42.107258s,pins unchanged,all sampled compiler handles absent; no
validation was launched239. Then added the optional conservative1e-11margin after
terminal verification. Build240terminal0,45.143734s,all1302pins unchanged,both targets
current,all compiler handles absent. Query tool49570/case...080717_3175207terminal0,
53.562074s,443assertions/1case,allpins and supplemental preparation pins unchanged,
all5sampled runner/job handles absent. It ended before detailed tree/start sampling;
runner start ticks captured at launch, no stop sent. No MPI campaign repeated:
production code remains unchanged; this is bounded actual-input geometry diagnosis.

## Next target from these data

Both collapsed faces contain genuine geometry violations. Tighter/cheaper certification
alone cannot make them satisfy the final100µm gate. This does NOT prove that the
intermediate area-weighted upper-bound merit is tight or that every refusal is physical.
In particular, query0and query2share the SAME original chord7876–7868; its exact midpoint
has a certified whole-component lower248.334µm. Original reference facet16734contains
both endpoints. This is an actual under-resolved geometry chord already present before
collapse, rather than evidence for another operator or a tolerance relaxation.

The preserved diagnostic rejection ring contains feature[7868,7876] at round37419,
old cells[227654017,227654022],shape unmeasured,failure0,reason EMPTY. The source records
this only for a rejected proposal. It proves the chord was considered; it does NOT
identify which primitive failed or faithfully reconstruct that earlier cavity. The
current final52-cell context is different and must be labelled as such.

Assess existing projected SplitEdge/InsertEdgeCavity/ReconstructSplitPatch on this
actual chord and its complete current neighboring volume before changing production.
Distinguish geometry projection, exact positivity/interface obstruction, dependency
closure and later quality undoing useful construction. Geometry-directed complete-star
apex motion is currently gated differently from metric/height strip paths; inspect
actual failure before inferring that gate is the cause. RefitWallSplit explicitly
requires requested heights and cannot simply be reused with empty Euler heights.
No new operator, larger cavity, scheduler, quality-aware construction or new geometry
representation is selected. Preserve all final geometry/association/feature/body/height,
transfer and restart obligations. Once a justified connected correction is qualified,
return immediately to full SAME-seed two-event M6 under unchanged budgets.
