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
TERMINAL124/1802.249s, actual diagnostic timeout1800. All5 actual identities absent;
source/input/runtime pins verified. CPU1797.84+.60s; maximum sampled treeRSS
324,579,328B, later stable224,550,912B. No completed integral or inner guard:
target construction and BL mesh acceptance remain unproven. This is not a proven
native5400 failure or a measured attribution to chord preparation.

Focused316 PASSED277assertions/51.421s, all5 actual handles absent, pins verified.
Cached/fresh tensors and unknown-envelope/global fallback are covered. No affected
broad or MPI qualification completed for316 yet. Preserve the fixed global fallback, every
feature/image, height policy and original ideal-chord certificate scope.

## Next bounded observation within this cycle

Read-only audit confirms strict IntegrationColumns3D recomputes EdgeBreakpoints for
each angular ray and retains only local radial/comparison/angular points. The
separate estimate/workspace path has preparedChords/reusedChords/preparationSeconds
and a barycentric-key cache; those are NOT counters for this strict replay.
The existing density callback evaluation count and AtlasBudget work/deadline callback
can emit rate-limited test-only windows without changing numerical decisions.
They distinguish density progress from geometry-only periods, but time between
density callbacks also includes SamplePoints and source reconstruction: it cannot
by itself identify chord preparation. Exact repeated-chord counts/preparation time
need explicit phase counters; do not infer them from the1800s timeout.


## Observational317/318 follow-up (not a second optimization cycle)

Test-only317, production byte-identical316, uses existing AtlasBudget deadline checks
to flush60s work/evaluation windows.300s observation timed out as planned, all5
actual identities absent, all pins/runtime verified. At240s work2537406/evals1407;
density arithmetic.003751s.120→180s adds649969work but only5density evaluations.
These confirm geometry-heavy intervals, not chord-vs-point attribution.

318 adds optional observational diagnostics to IntegrationRule3D using the EXISTING
ComplexityEstimate3D counters plus preparingChord. It stays separate from the scalar
estimate selector through all three strict-column routes. Completed preparation
count/time exclude a currently active or throwing chord; preparingChord intentionally
remains true on a chord exception. Other support/point/cylinder costs are not
attributed by this counter. No caching, changed query/cut/acceptance decision or
new production configuration. Timing/output can affect elapsed-limit termination.
Build318 passed246.227s/all52 sampled actual build handles absent. Equality fixture
passed396assertions/4.353s, exactly matching weights, barycentrics, samples and
full tensors with/without diagnostics. Same actual cell completed its300s phase observation (expectedtimeout124), all5
actual identities absent and all source/input/runtime pins verified. At120s four
completed chords cost57.996s, another active; at180s five cost138.749s, activefalse;
at240s seven cost138.871s, another active. Geometrywork2604384, density arithmetic
.003898s. This identifies costly strict chord preparation, not exact point-query
repetition. It is not a new baseline campaign or full target qualification.


## Selected connected hypothesis319: reusable exact cubic region geometry

Missing capability is usable strict M6 BL target integration within existing
budgets.316 clears both captured chord refusals but its complete first-cell replay
is incomplete at1800s.318 attributes long intervals to chord preparation. Source
inspection finds ClosestCurved::prepare recomputes ten outward Blossom control
boxes and their hull for every facet/parameter triangle on every nearby query.
This geometry-only data repeats independently of exact point-query identity;
its fraction of CPU and the speedup remain hypotheses to validate.

Reuse the EXISTING prepared scope pool, clock eviction/shared_ptr pinning, quota
and admission for immutable exact descendant nets/hulls. A disjoint key namespace
contains facet plus all exact binary64 parameter-triangle coordinates within the
same immutable authority/version. Preserve existing facet/scope allowance halves;
closest geometry shares the scope half, rather than adding a new allowance/pool.
No approximate UnionTriangle may substitute for cubic nets. Query candidates,
distance/clearance lower bounds, normals and allowed-support traversal stay fresh.
No reuse of completed closest results or assumed projection idempotence.

Cache construction/keys must fit transient scratch admission; retained payload and
both map/clock keys count against the existing reserved pool. Active shared_ptrs
pin data through use. Metadata maintenance consumes the existing query work budget;
zero quota/refused retention recomputes normally. Do not change gates, configuration,
work/time/memory ceilings, sensor/target/construction policies or CAD optionality.

Qualification: exact cached-vs-zero-cache closest intervals/candidates on distinct
queries and restricted/body supports; exhausted work/storage/deadline failure,
seams/features/heights and shared ownership. Then the SAME two captured M6 chords
and actual complete first cell under unchanged diagnostic limits. Measure build
reuse separately from fresh query work, CPU and useful integral progress. Successful
reuse counters alone do not qualify the correction. Complete affected serial and
ONE MPI milestone only after demonstrated actual-case benefit, then promptly full
SAMEseed RANS/BL. No mesh/workflow completion claim from fixtures.


###319 first qualification (incomplete)

Build319 terminal0/368.360s, all152 sampled actual handles absent/pins verified.
Focused exact cached/zero/tiny-cache comparison PASSED231assertions/6.347s,
all5 actual handles absent. Complete support, separate bodies and restricted facet
checks, exact intervals/candidates/normals/curvatures and resource refusals pass.
Concurrent duplicate publication/pinned eviction and surface-pool competition still
need their affected milestone coverage; no all-suite claim.

Actual captured chord314 PASS60assertions/32cuts: certification26.624s vs31637.560s
(-29.1%); work432259 vs409726, exactly22533charged cache insertions.384255region
hits/22533misses,36,593,592retained bytes/no refusal, within existing3,204,054,800quota.
Chord311 PASS37assertions/9cuts:7.449s vs10.443s(-28.7%);124069 vs113482work,
102209region hits/10587misses,17,193,288bytes/no refusal. Both actual process trees
terminal/all5 identities absent and source/input/runtime pins verified. Query time
and work are distinct: geometry reuse reduces CPU while maintenance adds charged
work. Consistent isolated-query timing benefit is NOT full-target completion.
The complete first-cell319 replay is now active at unchanged1800s diagnostic cap.


###319 complete-cell observation and return to actual M6

Cell319 TERMINAL124/1803.413s, original diagnostic1800 cap, all5 actual identities
absent and all source/input/runtime pins verified. Last1740s window has73completed
chords/13734density evaluations/29.300Mchargedwork; CPU1798.88+.77s, sampled treeRSS
maximum1,681,326,080B plateauing near1.68GB. No completed integral or intrinsic guard
refusal. Keep actual_cell passed:false; actual target/BL generation remains unproven.

The1800s probe is a shorter observation than the ALREADY authorized5400s production
native limit. It cannot establish a native5400 failure. Complete affected serial and
ONE MPI behavior qualification, then return to the full SAMEseed actual M6 attempt
with native5400/total14400 UNCHANGED. This avoids making a diagnostic the capability
objective. The full-run helper explicitly records/accepts this incomplete diagnostic
as evidence, not a passed integral; all five other behavioral stages must pass.
No production C++ change or gate/constraint/budget relaxation accompanies this scope
correction. Any intrinsic guard or new validation failure still requires a fix.


###319 behavior qualification and actual workflow launch

Affected serial PASSED52cases/286104assertions/326.610s, all5 actual identities
absent. ONE MPI6 PASSED55cases on EVERYrank/164.704s, all10 identities absent;
assertions2960/2949/2912/2951/2898/3442 (log order), all pins verified.
qualification319.json seals this BEHAVIOR scope, keeping actual_cell passed:false.
Prior303seven baseline-controlled failures outside this selection remain failures.
No all-suite, normalized target, BL generation or workflow acceptance claim.

Full SAMEseed RANS/BL319 launched with original303 mesh/reference/seed/config,
native5400/total14400/12GiBperrank unchanged. Frozen full-run source/helper/input pins,
initial finiteCFDCL.155731/CD.050268 and matching Hessian/D1 prefix. NativeFIRST_ROW1
with MPI6 parents is not distributed generation qualification. Follow HANDOFF for
live identities and do not edit sources until terminal or preserved stop evidence.
Euler293 BOTH checkpoints and isolated restart artifacts freshly hash-verified in
euler293_preservation_during_bl319.json (22files). All completion duties remain.
