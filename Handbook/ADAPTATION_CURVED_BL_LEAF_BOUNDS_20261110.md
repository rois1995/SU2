# Connected cubic BL interval-bound correction — working design

2026-11-10. ONE existing curved-BL integration cycle. Production change is not
implemented or qualified yet. Preserve qualified Euler293 checkpoints and all
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

## Connected candidate, pending proof review

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
   Do not replace this proof with an unexplained64eps pad. One reviewed route
   bounds the six squared-norm product/add operations by12 ULPs at a magnitude
   enclosing every intermediate (e.g.4*D^2+128*minimum LD increment), then applies
   the same outward sqrt/conversion. Overflow/uncertainty retains the global bound.
4. Preserve caller semantics: strict IntegrationColumns reconstructs source points
   via SetBLSourceQuery and up to8 inward attempts, rather than ordinary affine
   rounding. A private per-axis source-query envelope must include normalized
   endpoint offsets, radial complement/mix rounding, the double inward expression,
   LD reconstruction and final conversion; use the actual accepted retry scales.
   Keep SetBLSourceQuery itself unchanged. Review whether padding belongs only to
   the metric envelope or must also enlarge ideal-chord exclusion; do not silently
   change certificate scope. This remains a proof decision before implementation.
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
