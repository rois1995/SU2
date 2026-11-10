# Connected cubic BL interval-bound correction — working design

2026-11-10. ONE existing curved-BL integration cycle. Production316 built successfully; qualification is pending. Preserve qualified Euler293 checkpoints and all
construction/certification improvements. No gate, budget, sensor, Lp objective,
requested height, geometry authority, scheduler or production configuration change.

## Missing capability and actual evidence

Full SAMEseed RANS/BL314 selected a later chord work refusal in first target cell0
(trial1, same303 core/sensors/coordinates). All10 actual handles terminal and all
pins unchanged. The prior captured chord passes314; whole target still does not.
The new chord has7246 splittable leaves and unresolved3.1354e-8 >1e-8 at16384 work.
Its local metric-eigenvalue upper is7.00000248e12, down from global1.215e14.
Full5462s includes other ranks and collective failure join, not selected-chord time.

Bounded point315 passed164assertions/11.439s helper, all5 actual handles absent,
source/runtime/input pins verified; production CFD byte-identical314. Eight
captured leaf/neighbor points used6680 direct closest-query work plus1920 separate
policy-evaluation work. Candidate distances/normals are diagnostics, not proofs.
Certified point-distance intervals span~2.26e-17; classifier radius~2.48e-16.
Normal-dot-chord slope~8.8649e-4 agrees with finite differences; normalized.3779
is not near tangent in these samples. Measured full-tensor maximum~8.54e6 is far
below the7e12 CHORD upper. This supports interval-local bounds, but does not prove
that the same small bound holds throughout a chord or that a new method converges.
Data: staged57/m6_bl_inputs/crossing314_chord_v1/point_probe_v1/analysis315.json.

## Connected316 implementation, pending qualification

1. Retain the global upper as fallback. Split the immutable wall bound into its
   tangential contribution and normal-height contribution. For each unresolved
   leaf reuse its existing certified distance interval/radius; use a conservative
   lower bound on the actual evaluated distance to bound the monotone existing hn
   profile, including min(distance,full), h0 floor and outer-normal policy. Keep
   the existing numerical safety factor; summing tangent and normal uppers avoids
   assuming exact floating-point orthogonality. Outer tensor transport divides by
   a scale>=1 and may conservatively be ignored.
2. Bound transported feature anchors per leaf. The old root factor3 distance
   inflation expands the anchor box by centimetres and admits many h0 maxima.
   Retain EVERY feature/image and the sequential-intersection sum; never replace
   their sum by an unproved maximum or sample-based bound. Keep implemented
   feature segments a+s*t and exact wall/component membership.
3. Give computed candidate distance an explicit arithmetic contract. A small CBC
   displacement-norm enclosure can use outward LD products/sums/sqrt and outward
   binary64 conversion, then clamp the existing hypot result to that enclosure.
   Ensure normal normalization remains stable/unit to the existing numerical
   contract; ordinary results should be preserved where possible. A portable
   envelope must cover candidate query precision, realization contracts, rounded
   subtraction, norm evaluation and zero/subnormal/overflow cases separately.
   The implemented scaled directed norm bounds normalized intermediates by8. Its
   error envelope uses34 LD ULPs at8 times the distance, plus explicit scale/cast
   ULP terms and subtraction error. Overflow/uncertainty retains the global bound.
4. Preserve caller semantics: strict IntegrationColumns reconstructs source points
   via SetBLSourceQuery and up to8 inward attempts, rather than ordinary affine
   rounding. A private per-axis source-query envelope must include normalized
   endpoint offsets, radial complement/mix rounding, the double inward expression,
   LD reconstruction and final conversion; use the actual accepted retry scales.
   SetBLSourceQuery itself remains unchanged. Padding bounds metric evaluations
   at reconstructed/inward samples. Original ideal-chord exclusion remains unchanged:
   this does not newly certify that rounded Gauss samples cannot cross an excluded
   level. Preserve that existing scope and the independent quadrature-error checks.
5. Store each leaf's effective bound; children may tighten by min(parent,new) only
   for the same frozen profile and complete child interval. Adjacent leaves inherit
   the parent bound without querying. Update pending-cover diagnostics consistently.
   Existing GeometryWorkspaceBytes reserves4*sizeof(LD) per frontier interval;
   verify added fields fit with a static assertion and actual sizeof admission.
   Existing cache key is8 original-cell ENTRY/EXIT barycentrics, not endpoint xyz.
   With frozen workspace source/owner/profile and deterministic inwardScale/padding,
   it already identifies the envelope: no new cache-key structure is needed.

## Scope, risks and acceptance

Affected production sites: CBC curved ProjectWall numeric distance/normal,
WallData derived bounds, EdgeBreakpointsImpl frontier/metric envelope and both
IntegrationColumns callers/private signature. Existing public point/edge API,
geometry backend/authority, operators, field transport and construction stay intact.
Distance clamping/normalization can change ULPs, cached samples and mesh trajectories;
record this explicitly and compare cached/recomputed results and old point315 data.

Before declaring a correction qualified, review all bound assumptions, exercise
curved seams/features, separate bodies, requested heights, transported anchors,
source-consistent point placement, cached/fresh results and resource exhaustion.
Use focused serial checks and ONE appropriate MPI milestone at the established
cadence, reusing unaffected passing evidence. Test the ACTUAL captured chords and
coupled first-cell behavior before attributing a speedup; do not launch another
baseline/perf/GDB campaign. All existing1e-8 crossing/work/storage/time/cut gates
remain. Then promptly return to the full SAMEseed M6 RANS/BL attempt.

Completion is not these diagnostics: require a complete accepted BL mesh and its
current-mesh suitability census, BL/constraint preservation under MPI, TWO accepted
RANS500k events, conservative admissible flow AND SA transfer, finite adapted CFD
and an actual isolated adapted disk restart. FIRST_ROW native1 with MPI6 parents
alone is not distributed BL generation. Missing progression/coverage and curved
alignment evidence remains documented in ADAPTATION_BL_SUITABILITY_EVIDENCE_ROUTE.

## Build316 first actual evidence (incomplete qualification)

Build316 terminal0/252.261s, source pins unchanged and55 sampled actual build
processes absent. Captured314 chord PASSED60assertions/47.845s helper; certification
37.560s/409726geometry work/32cuts. This resolves that preserved work refusal,
not a measured comparison to its incomplete full-run chord work.
Captured311 chord PASSED37assertions; certification10.443s/113482work/9cuts,
versus314 17.014s/181652work/16cuts on that SAME captured query. Work reduced37.5%;
wall times remain subject to contention. Both actual process trees terminal,
all source/input/runtime pins verified. Faithful complete first-cell replay is
currently active; target construction and BL mesh acceptance remain unproven.

No broad or MPI qualification completed for316 yet. Cached/fresh and unknown-envelope
regressions are compiled but not run. Preserve the fixed global fallback, every
feature/image, height policy and original ideal-chord certificate scope.
