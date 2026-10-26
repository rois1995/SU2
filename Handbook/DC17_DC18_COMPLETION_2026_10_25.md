# DC-17 / DC-18 completion work — 2026-10-25

User request: implement the missing BL-to-outer metric transition and finish both
DC-17 and DC-18. The complete goal remains accepted M6 Euler then RANS adaptation
with unchanged final q>=.20, J>=.05, edge<=1.8, height within10%, geometry and
solution-transfer gates. No milestone is closed by standalone fixtures.

Starting source: 7c1a56e0bc. No owned build/test was live at start; host load0.5,
available RAM37GiB, root free13GiB, data disk free89GiB. Build-j1, nice19,
threaded libraries1, scratch in build-integrated-v2/compile-tmp. Use Ninja1.13
/home/rausa/Software/su2-develop/ninja. Push approval remains pending; do not retry
an export rejected by automatic review without destination approval.

## Missing capability and evidence

The native sensor is graded, but the BL is composed afterward. Its fixed log fade
across0.9T..T leaves >20x target changes in the M6 TE band. A smooth nodal field
alone does not fix private queries, edge integrals or complexity. Copying the full
h0 wall tensor into the original P1 sensor would smear a thin layer across coarse
source cells and is not an acceptable shortcut.

A field correction must be common to actual-query evaluation, cached samples,
chord-pinned edge integrals, complexity normalization/replay and FIRST_ROW optionA.
If its influence extends beyond T, all support bounds and sampling proposals must
cover it, with cache invalidation when the frozen core/profile changes. Preserve
requested first height, geometry restrictions, per-wall identity and finer sensor
directions. The core transition and generic tensor gradation across changing
frames are distinct; radial smoothing alone is not a combined-field certificate.

The latest c9 continuation fails3 height checks. Rebuild:213 short columns,
1354 unsplit-column prism refusals,230 cross-lid link faces,238 flip refusals;
3 fallback cones fail metric validity. Topology preservation alone does not retain
rebuild feasibility when lids move. The difficult target is c9 event1, not just a
new simple prism. Preserve and inspect those witnesses before changing construction.

DC-18 remains standalone. Required: immutable persisted control net/options/body
and feature identities; production geometric evaluation under the existing exact
parameter-space topology; wall refinement and curved first-row construction;
source envelope/clearance checks; accepted full meshes, transfer and restart.
Configured ADAP_HAUSD stays independent of h0. Flat-face altitude and positive
smooth-reference clearance can conflict at true h0; do not hide that by relaxing
either gate or silently tying the geometric tolerance back to h0.

## Acceptance and cadence

Implement connected target-policy changes before a broad campaign. Focused checks
must cover actual points, cached samples, chord pinning, core/profile changes,
rotated anisotropy and multiple walls. Use the frozen M6 TE field immediately to
measure the band-end transition and cost; then the rejected continuation plus
multi-body/tilted-plane targets. Broader regression/MPI only at integrated changes.
Final milestone requires an accepted full mesh and the relevant transfer/restart
workflow. Save independent stages with source/binary/runtime/input/helper hashes.

## Current work

Implemented (focused checks pass; full capability unvalidated): whole-tensor continuation from0.9T by the normal-ray
metric transport factor, instead of the fixed0.9T..T fade. Independent additive
principal-size growth was rejected because a finer tangent can violate transport
along a coarser normal. This changes the common evaluator, edge/support bounds,
2D quadrature, cache identity, original normalization/replay and FIRST_ROW policy.
The finite support uses a conservative wall+feature spectral bound and frozen core;
profile changes refresh samples; a source-domain mixture covers changing tail support.
Normal-ray transport is not a proof of combined gradation across changing frames.
The support cost and actual M6 field must be measured before calling this usable.

Fixed run_event.sh appending FIRST_ROW/ramp/size controls to .pending.pending instead
of the .pending config that is published. Three receipt/config regression tests pass.
Optional local Claude Opus5.5 review is unavailable (weekly limit); no review exists.
Evidence: `integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/`.

First focused build/test: terminal, 4 cases/420 assertions pass (validation1).
Covers preserved inner tensors, fine tangential demands, point/cache/chord equality,
2D signed sides, analytic sphere, stale samples/workspaces, outer-profile selection.
It does not prove M6 or changing-frame gradation. Follow-up bounds candidate facets
by endpoint nearest distances plus chord length, rather than the much larger field
support. Core queries also reject insufficient support before the broad phase.
The validation2 library build is terminal; source pins and logs are retained. The enlarged tube importance sampler may become unusable:
far edge/vertex proposal masses grow with support radius. It already reports
infinite uncertainty with fewer than32 nonzero corrections; do not bypass that gate.

M6 evidence (validation2/m6_oriented.json): the original donor cells orient every
input facet; 9,393 ray pairs compare the same original P1 sensor. >1% transported
tensor violations fall4,177 to21, worst ratio11,023 to1.406. 53 queries leave the
original domain near the symmetry boundary and are counted as refusals. This is
field evidence, not a full gradation or mesh certificate. The earlier raw-facet
probe is superseded (unoriented input faces caused4,596 refusals).
The extended-tube sampler hits its existing PDF candidate/work ceiling on M6.
No limits were raised. The connected fix now mixes the original thin-wall
proposal with a uniform original-source bounding-box proposal, using the complete
mixture density. This covers all tails without growing every support component.
Core trials refresh samples but reuse that domain-covered proposal. Support only
expands when trials ask for a coarser core, so rejecting a trial cannot invalidate
previous family queries. The new mixture and core-cache checks pass in serial (validation3:6 cases/506 assertions).

Validation4: build terminal, exit0. Focused transition, profile, dynamic complexity
and donor-authority checks pass625 assertions in10 cases. The first full M6
normalization attempt (validation3) refused2,357MiB against2,048MiB before sampling.
Donor construction retained dead peak estimates across phases, counted exchange
buckets twice, and allocated a duplicate-check map over6N records. It now accounts
for live buffers by phase and checks sorted adjacent authority records in place;
duplicate cell/simplex and inconsistent node/sensor rejection remain tested.
The caller also stops counting the original input geometry twice. No cap changed.
Validation4 M6 probe is terminal/exit0: normalization completed in4.97s, retained
540,794,136bytes; full process maximum RSS1,498,668KiB. The8,192-sample estimate
has18.6% uncertainty. Validation5 then used both allowed refinements:8,192 /
16,384 /32,768 samples; final estimate494,938 +/-154,193 (31.2%). This is not
accurate enough for acceptance. No limit was raised. The new proposal design
uses nested wall boxes (doubling their geometric padding, intersected with the
source domain; last box is the full source box), combined with the original
thin-wall proposal using the complete mixture density. This targets the previously
undersampled near-wall tail while preserving all-domain support.

Validation5 additionally records actual normal and foot changes for each ray pair.
The violations do coincide with normal/foot changes, not sensor variation. Their
root contribution (wall interpolation vs anchored feature transport) remains to
be separated before changing the field again.

Validation6 build completed successfully; it includes the nested proposal and a shared
column lid chosen to clear1.5 times each incident requested face height. The old
midpoint assumption is removed; split/cavity validity gates remain unchanged.
The shared-column checks pass in three orientations. The added flat integral
oracle initially omitted the unit triangles' tangential eigenvalue3/2; corrected
the analytic integral (same tolerance), preserving the failure log. Also made
rank-local nested-proposal failures collective before the next MPI operation.
Validation7 build and focused checks pass:642 assertions/11 cases, including the
closed-form anisotropic integral. Validation7 probe completed, exit0: nested estimates749,077 /749,504 /777,625
for8,192 /16,384 /32,768 samples; final uncertainty79,139 (10.18%), retained
549,721,600bytes, normalization13.54s. Existing sample-stage cap reached normally;
no limit changed. This is improved but insufficient normalization evidence.
The no-feature field has26 violations (worst1.660), full D1 has21 (worst1.406);
D1 removal would not fix the underlying changing-frame problem. Preserve this
diagnosis before attempting a different combined-field construction.
Validation7/m6_rebuild completed/rejected (session40302, exit1): c9 event0 into
event1, V disabled, timeout1200s. Shared height constraints reduce short columns
213->1 and increase rebuilt prisms39,436->40,415; fallback1,584->605. Height
failures3->2 (cells1217351 and1217329; cell1203130 is fixed). No inverted or
invalid cells, topology or A3 violations;87,923 bad shapes,295,556 long edges,
0 unmeasurable. Intermediate acceptance is correctly refused; no publication.
The283.66s remesh and4,876,536KiB process maximum RSS are recorded in event1.out.
Its receipt contains non-JSON `inf` for unestimated complexity uncertainty; the
independent construction report remains valid JSON. Preserve that failed receipt.

The continuation path omitted the existing CompleteFirstRow pass, leaving only
V to retry geometric first-height completion. It now calls that same constrained
completion before V, as initial construction does. No new operator or fallback
was added. The test receipt writes null for unavailable uncertainty. Validation8 replay (session25651) completed/rejected, exit1, with the same two
height failures. Its completion report explains the refusal: ApexMove's existing
LP cone is geometrically valid but rejected as a Repair for neither size nor
quality progress. The receipt is now valid JSON with null unavailable uncertainty.

ApexPlace also starts outside the new height window, and witness1217329's old
normal foot has barycentric coordinate0.001 versus the optimizer's0.02 minimum.
Do not add a second feasibility solver: ApexMove already uses ConeColumn/ApexLP
and produces an admissible geometric candidate. The connected fix is the missing
operation intent. ApexMove, ApexPlace and ApexRelocate now use the existing Layer
admission when the old patch fails the CURRENT requested height, retaining Repair
for compliant rows. Fresh heights, cell validity, interface, D2, vertex-removal
size certificate and final q/J/edge gates remain in force. This uses geometry/
constraints, never event labels or fixture identity. A focused test covers height
decrease, already-compliant height and height increase.
Validation9 build51972 and count-fix build53506 terminated successfully. Focused
serial checks pass645 assertions/12 cases; MPI2 passes10 cases/rank
(612/627 assertions). Full-M6 replay session56923 terminated/rejected (exit1), V=0 diagnostic.
Receipt has complete=0, written=0; remesh286.94s, process peak4,875,424KiB. Its completed construction
phase still has2 height failures. Both LP apex candidates now correctly reach
Layer admission and are rejected because moving the vertex adds frozen
oversized-edge excess. The prior Repair refusal did not establish that excess
would be preserved; validation9 resolves that uncertainty negatively. No gate
was waived. Further fixed-star candidate variants are not a demonstrated remedy;
coordinated cavity/size construction remains required.
Broader regression3737 terminated (exit1): metric176 cases/305,646 assertions,
unit2d57/52,273, pipeline3/125 and growth2/3,286 pass. Unit3d passes33/34; its
sensor-only donor integral oracle still used the old fade. Production656.152864
agrees with the new closed-form continuation integral656.152258. Updated only
that formula, keeping its5e-4 tolerance; test-only rebuild78306 terminated successfully.
Validation10 unit3d73683 passes183,577 assertions/34 cases. Reuse audit confirms
only the oracle source and test executable changed: production static libraries,
CFD executable and dynamic libraries are identical. Unaffected v9 stage evidence
is retained under its original pins. Probe build30296 and run3415 terminated successfully. Validation10 reproduces
21/9,393 gradation violations (worst1.405985);53 queries outside original source
coverage remain explicit refusals. Complexity777,624.65 +/-79,139.01
(10.18%) at32,768 samples;13.40s normalization,549,721,600 retained bytes,
68,380,877 PDF candidates below the unchanged80M ceiling. The complete probe
took17.55s and process peak1,516,412KiB. Both refinement stages are exhausted
normally; uncertainty remains insufficient. All final source/input/helper,
binary/runtime and production-library pins verify in validation10/summary.json.
No owned build/test/probe remains live. Both milestones remain open.
This is a connected caller correction, not proof that the remaining cavity
coordination problem is solved. Further local variants are not justified if this
standard completion still stalls. Source/input and
binary/runtime pins are recorded in validation7. Source review during build9
found Refine reducing5 values from4-element control arrays. Fixed to4 before
validation; prior refinement/normalization results are diagnostic only until
rerun without the out-of-bounds access.

Frozen c9 height witnesses are saved in height_witnesses_c9.json. All three failed
first children remain at1.800002 times the requested new height, near the wing tip.
Two prisms have half-old-column minimum face clearances only1.005 and1.143 times
new h0. The previous fixed-midpoint split assumed geometric self-similarity after
column optimization, which is false. The shared lid now respects the currently
requested height of every incident wall face. One failure is fixed; the two
remaining cavities need coordinated height/size construction rather than
acceptance relaxation or more unmotivated fixed-star candidates.


## Continued implementation after checkpoint29e6778c13

User explicitly requests sustained work through all remaining TODO items and newly
found defects. No owned job was live; host load0.77 and39GiB available at restart.
Current connected change removes CavityFirstChild's blanket ban on registered
column transitions. Visibility/widening can cross those cells if every registered
vertex remains on the new cavity boundary; other first children stay outside.
Existing D2, height, topology/positivity, validity and size admission remain.
The target face registration follows its inserted apex, including repeated
continuation. No case identity, limit increase or new fallback is introduced.
A two-wall/two-carrier test at two rotations checks preserved vertices, other
first children, total volume, positive cells, fresh registration and A3.
Full-M6 replay is the acceptance target, not the fixture alone.

Build95298 (validation11) terminated successfully. Focused checks pass443 assertions/3 cases. Full-M6 replay20248 terminated exit1 (V=0 diagnostic): construction accepted, height0, positivity/topology/A3 pass,1,554,208 cells; publication rejected complexity1,757,603 +/-28,089 against500,000 without original-family correction. Complete0/written0; remesh345.68s, whole7m22.60s, process peak8,201,436KiB. No owned job remains live. Source review also finds
why extending the old correction proposal is inappropriate: its radial density
is proportional to1/hn, whereas whole-tensor transport has a1/scale^3 density
outside the full band before sensor eigenvalue floors become active. Those floor
crossings change the power, so a pure cubic-tail proposal is not sufficient. Its current all-axis radius boxes also overbound the
oriented face prisms. Next normalization work must model that transported tail
and evaluate the complete mixture PDF under the same work/storage limits; first
measure contribution/variance by region before choosing proposal parameters.

Connected field-consistency work: normalization currently uses full D1, FIRST_ROW
construction/reassessment uses Option A, and its driver explicitly omits the
normalization callback. Reuse/extract Option A rather than duplicate a law.
Reach solver normalization/profile trials and diagnostics, retained family,
candidate assessment, driver construction/reassessment and focused tests.
Require pointwise agreement plus M6 geometry/budget publication under unchanged
limits. Frozen harness has no original metric family and reinterpolates donor
samples on its event input, so it cannot stand in for normalized solver evidence.

Implementation (validation12 build86572, subsequently terminated): extracted
existing Option A and shared vector operations into CNativeBLTarget3D.hpp and
CNativeBLVector3D.hpp. Solver profile/scale trials and final field diagnostics use
that target; retained families carry it; candidate correction and driver stages
reuse it. Private-stage row continuation uses pending rows without committing them;
RebuildFirstRow already selects only children above the current height window.
The replay now normalizes supplied nodal sensor seeds with ComputeMetric, retaining
the event's family; it is no longer evidence for unchanged original coarse-P1
values throughout the ramp. Added sharp-crease profile/core agreement and solver /
candidate agreement tests. These edits are unvalidated. Cavity registration also
updates all physical wall faces of a multi-wall seed cell.
Host check20:54: load2.45,38GiB available; one owned build-j1, nice19, threads1.

Build86572 terminated exit1: extracted header lacked its direct ReferenceState /
Facet include (solver and driver compiled because they already included it).
Fixed self-contained includes. Review also corrected retained-family selection
across faceted promotion, extra policy memory double counting, and cached base /
full samples + geometric weights for trials (no repeated wall searches for an
unchanged profile). Sample appends invalidate the new base cache.
A numerical counterexample establishes another preexisting Option A defect:
logarithmic interpolation of two sensor-dominating metrics can weaken a sensor
direction (minimum eigenvalue of blend-sensor -0.16648). Shared blending now
intersects with the original sensor using existing long-double intersection;
endpoint policies stay unchanged. Added regression, cached/direct/refinement and
stale-core checks. Validation13 build31862 subsequently terminated; see the build result below.

Build31862 terminated exit1 after production objects/library/CFD executable built:
new test used the passive communicator wrapper with long double, which that
wrapper does not support. Replaced with the existing explicit MPI_LONG_DOUBLE
reduction pattern. No production change for this correction.

Build92348 (validation14) and focused execution are terminal. Build passes;
focused593/594 assertions pass across8 cases, one fixture-only failure: all crease
samples sat in the full-D1 wedge, so the intended base/full contrast assertion
failed. Coordinates corrected to cross wall-side, blend and wedge regions.
Sensor-preservation, direct/cached sample append/stale-core, solver/candidate and
cavity checks passed. v14 source/runtime pins and failed receipt are saved.
Production is unchanged for validation15; only that fixture is rebuilt.

Validation15 build97176 and focused run terminated exit0:594 assertions/8 cases.
Sources and binary/runtime pins verify. Full M6 c9 event0 -> requested event1 is
starting with production normalization and retained-family correction, V=0
geometry/budget diagnostic; whole-job guard1800s allows the newly included
normalization phase. Native operator gates/work limits stay unchanged.

Normalized M6 replay55778 (validation15) terminated exit1 before construction:
reference preparation and original-family field setup complete, but bounded
normalization's DonorImport requests5482MiB > hard-coded2048MiB. Whole99.42s,
process peak3,855,144KiB. Source/runtime pins verify at termination; no job live.
Next connected capability: the ledger's explicit native3D memory-limit option
(MiB per MPI rank), shared by metric quadrature, source preparation, family
snapshot, reference preparation, remeshing and publication. Explicit values stay
fixed; zero preserves existing defaults/automatic FIRST_ROW behavior. This is
accounted native working storage, not an OS/process RSS cap. Use a reviewed12GiB
M6 budget within available RAM; keep all geometry/quality/budget gates unchanged.

Local checkpoint e2626a9c5b preserves the shared target, cavity correction,
passing594-assertion focused checks, and all M6 failures. Remote push remains
blocked by the previously recorded automatic review; no new remote attempt.
Memory option implementation now reaches CConfig registration/overflow/backend
validation and documentation, CDriver symmetry preparation, solver quadrature /
bounded integration / family retention, native3D reference/remesh and publication.
Explicit limits are collective-consistent and cannot auto-grow. Native2D explicitly
rejects the currently unsupported option. Tests cover MiB parsing/defaults and
both METRIC/FIRST_ROW refusal without state publication. Full header rebuild next.

Validation16 full build83949 terminated exit0 (308 tasks,38m48.42s, process peak4,097,284KiB), nice19,
one worker, library threads1, compile scratch on4TB. Host at start:38GiB available,
87GiB disk free, root11GiB. Sources/input/helper/config pins are recorded. Do not
edit pinned C++ before the owned build handle is terminal. Next: memory option /
field/cavity focused checks, invalid option CLI checks, M6 event1 with12GiB,
then the appropriate integrated serial/MPI regression at the capability milestone.

Read-only review during build16 identifies the likely cause of the six baseline
distributed-repair assertions: the injected throwing composition at
CNativeDistributed3D_tests.cpp:1440 omits options.edge_breakpoints. Engine policy
validation correctly rejects it before copying the input or invoking the callback.
Other constant-composition fixtures provide {0,1}. After pinned validation ends,
restore this fixture interface and check its original rollback/counter assertions,
including an explicit expected failure reason. No production guard needs weakening.
The three receipt-helper tests pass in validation16/receipt_helpers.log.

Publication review found a concrete guard hole before the next M6 run: a present
normalizer returning valid=true/changed=false skips both existing over-budget
guards (no-normalizer stage0 and corrected stage1). NativeNormalizationStage3D
returns that default on storage-limited correction and other no-change exits.
Add a final budget check after the stage loop, before ReaderSlices, and cover
unchanged correction/refused publication. Let build83949 finish; run memory/CLI
checks, then fix this guard together with the stale injection fixture and rebuild
the affected objects before M6. No M6 acceptance may rely on this guard hole.

Validation16 runtime checks terminated successfully:607 assertions/10 cases,
three invalid configurations rejected with the expected diagnostics (MiB overflow,
negative value, non-native backend). All957 source/input/helper pins and19 binary/
runtime pins verify. No build/test is live.
Publication guard is now consolidated after every normalization-stage-loop exit;
its regression uses a valid uniform field and a storage-limited no-op correction,
requiring rejection above budget and acceptance below it without publishing state.
The stale distributed failure-injection fixture now supplies its edge callback and
asserts the actual injected error. Config documentation now describes the native
continuation instead of the removed fade. Validation17 build25843 terminated successfully (6 affected tasks). Focused
run36108 terminated exit2:1,656/1,658 assertions pass across12 cases; only the
new injected-error text assertion fails twice. The six old assertions and new
publication guard pass. Source/runtime pins verify. Selection::Add loses the
actual ValidateMetricPatch error when measurement is incomplete; it now retains
that error through its sole caller. Validation18 rebuild/focused/MPI checks next,
then M6. No source is edited during active compilation or execution.

Validation18 build81127 terminated exit0 (82.05s, process peak2,860,436KiB).
The repaired automatic-repair case passes1,032 assertions; the six old failures
and the new injected-error diagnostics are resolved in serial. Source/runtime
pins verify. A startup probe also confirms the shared unsigned parser accepts
ADAP_NATIVE_MEMORY_LIMIT=0.5 as zero (later missing-mesh refusal). This option now
checks its existing raw token for decimal digits before range/backend admission;
unrelated parser behavior is untouched. Validation19 build28180 terminated exit0. Seven invalid-token CLI checks pass;
connected serial2730 passes1,658 assertions/12 cases. First MPI selection82783
failed only because NativeBL17Cavity explicitly requires one rank. Corrected
MPI selection93123 passes11 cases/rank (1,411/2,125 assertions). All source/runtime
pins verify; failed selections are preserved. Next: normalized M6 event1,12GiB,
V=0 diagnostic,1800s whole-job guard. Sources/input/helper and
new invalid configs are pinned.

Local checkpoint fe9bb2d30e contains memory controls, final publication-budget
guard, repaired injection fixture, retained query diagnostics and validation16-19
evidence. Raw logs retain their original whitespace. Remote push remains blocked
by the previously recorded approval review; no remote attempt made.
M6 session26009 is terminal/exit1: c9 event0 -> normalized event1, fixed12288MiB, V=0,
1800s whole-job guard. Host before launch:38.6GiB available, load1.41, no live
compiler/test/solver. validation19/resources.jsonl records the receipt. Sources
and runtime pins verify at termination. No owned build/test remains live.

M6 validation19 passes the memory blocker, then rejects before construction:
coarsest-sensor complexity rises from814,092.78 +/-88,656.92 (profile0) to
2,368,375.60 +/-422,363.34 (profile1), across all nine permitted profiles.
Mandatory estimate18,116 +/-7,455 does not establish infeasibility. Whole122.84s,
process peak7,278,232KiB; no accepted/publication receipt. Whole-tensor transport
decays more slowly when the anchor normal is coarsened, so a relaxed normal can
retain tangential demands over a larger curved/edge/vertex volume. Measure the
inside-band/tail and base/D1 contributions on the uniform coarsest field using
the existing estimator/probe before choosing a connected field correction.
Do not raise the500k budget, sampling/work caps or acceptance tolerances.

Validation20 diagnostic reuses probe_transition.cpp's original M6 loader and
build_probe.py's build/link flags. probe_variance.py generates a separate copy of
the estimator with per-query profile/distance/tensor-density contributions, and a
coarsest uniform-sensor Option A probe. Production C++ is unchanged. Assess all
nine profiles, then0/.5/1 at the existing16k/32k refinements; retain each completed
assessment immediately. Direct/cached equality is audited. This uses the original
coarse volume geometry (not the current fine input), so it diagnoses the field's
cost mechanism rather than replacing full M6 qualification. Sampling/work/storage
limits remain unchanged. Source/helper/input pins include the generator and loader.

Validation20 initial diagnostic build exited1 before execution: including the target before the reused curved test loader exposes a CGNS Foot enum collision. Move that include after the loader; also explicitly pass the shared target to Assess. Production sources unchanged. Preserve failed build.log; retry build in build_retry.log.

Validation20 retry build23654 and run41212 terminate exit0. All15 profile/stage assessments retained, final32,768 queries,68,411,939 support candidates below80M. 36.12s, process peak1,333,320KiB. Source pins verified; runtime pin script initially rejected an external libz path, corrected during the run to preserve absolute runtime paths, then verified after termination. Production unchanged. Attribution self-check reconstructs all15 correction integrals (1e-11 relative) and validates source/runtime hashes. Profile0:821,265 +/-53,688; profile1:2,298,938 +/-198,670. Inside0.9T falls541,371 ->269,268; beyond10T rises156,396 ->1,876,743. Base/D1 results also differ in the outer tail because stronger D1 at the anchor accelerates the scalar continuation. These are diagnostics, not accepted meshes or certified infeasibility.

Next diagnostic tests an upper bound on normal spacing by the shortest tangential size (never below h0) throughout the continued native field. It preserves tangents, first height and all original protected resolution by refinement, and prevents normal relaxation from creating reverse anisotropy at the band end. This is a candidate field design, not yet production: require lower M6 intrinsic cost, unchanged first-height/protected/finer constraints, shared point/cache/chord behavior and full changing-frame gradation assessment before adoption. Production guards and budget remain unchanged.

Validation21 diagnostic build2703/run17239 terminate exit0. Normal cap is rejected: profile1 remains1,182,909 +/-67,994; inside0.9T becomes1,077,101. No production field change. Fifteen attribution reconstructions and pins pass. All prior jobs terminal before DC18 source edits. Asked asynchronously whether tangential relaxation outside the protected band is permitted; existing normal-only contract remains in force pending reply.

DC18 connected implementation scope: extend existing MappedFacetReference with immutable cubic control nets, validated corners/shared edges/projected Jacobian, outward physical realization/deviation bounds, versioned sidecar round-trip, source-envelope displacement and child-faceting queries. Preserve exact affine parameter topology. Do not activate automatic curved construction before first-height/clearance and full geometry/transfer gates are qualified. Curved facet-local mode initially preserves original seams; it cannot claim the existing planar component release supports multiple nonlinear charts. Tests must exercise closed curved meshes, wrong/corrupt controls, budget refusal, restart identity and child error under refinement.

Validation22 syntax82992 passed all five affected translation units. Integrated31-task build started with one worker/nice19/threaded libraries1 and compile-tmp on4TB. Host load2.20,38.6GiB available,4TB87GiB/root11GiB free. All957+ relevant sources/inputs pinned before build. Source frozen until owned build/test handles terminate. The new curved path is facet-local and requires complete matching controls on adjacent patches; it certifies positive projected Jacobian but still needs whole-surface embedding/clearance and construction qualification. No automatic production switch enabled.

Build22 remains active/session30140; production source frozen. Review follow-up before qualification: preserve PointModelError's exact-zero semantics on disconnected flat components when another component has curves (current new ErrorBound takes an outward positive floor there). Add a mixed curved/strict-flat regression after build terminal. M6 probe_curved_authority.cpp is a new, not-yet-compiled diagnostic; it reuses probe_transition.cpp's donor-oriented loader, not raw SU2 face orientation. Full M6 admission/persistence is required alongside focused cases before this authority stage is called validated.

Pre-test review also caught a lifetime error in the new unit fixture's range-for over MappedTetrahedron()->Facets(); retain the shared pointer before iterating when build22 is terminal. This is unexecuted test code, not production. Rebuild the affected object(s) after these two fixes before running tests. The new M6 diagnostic reuses the original loader and names its PN object curvedSurface to avoid the loader's existing surface-map name.

Build22/session30140 terminated exit0:31 tasks,16m28.26s, process peak4,087,828KiB. Fixed the reviewed mixed-flat exact-zero handling and temporary-owner lifetime before testing. Added fixedMarkers to the PN builder so non-wall/non-carrier boundaries retain their faceted geometry and shared edges; a sphere/fixed-patch contract checks flatness, zero smooth curvature, watertightness and retained curvature elsewhere. The M6 probe selects these roles from config kinds/BL markers, not case names. Source pins updated for the next build (old pins preserved); production remains disabled pending qualification.

Build-fix25302 is terminal/exit0 (53.78s, process peak2,868,724KiB). Approval review initially treated it as active and rejected the following edit/test command; no edits executed. Read-only host ps subsequently confirmed no ninja/compiler/assembler/linker exists, and time log independently records exit0. Fixed-marker projection groups are separated from walls/carriers; source/runtime pins refreshed after that verified terminal state.

Validation22 serial12486 terminates exit0:959 assertions/11 cases,2.12s, source/runtime pins verify. Covers new immutable curved mapping, malformed controls/folds/shared edges, unchanged strict-flat component handling, error bounds, v7 round-trip and existing v5/v6 mapped publication/components. Next full-M6 authority diagnostic uses the original donor-oriented loader and fixed non-wall/non-carrier markers; no mesh acceptance claim.

Full M6 authority81869 terminates exit0:48,954 facets, zero fallback patches, all projected Jacobians certified, fitted carriers/fixed farfield admitted. Largest mapping bound6.04e-14; conservative full child deviation1.995e-3 and source envelope1.995e-3 (these are correspondence bounds over the complete boundary, not the earlier LE-only normal sagitta). 26,043,760 resident authority bytes;46,610,856-byte sidecar round-trips bit-identically. 80.04s, process peak569,024KiB. Source/runtime/archive pins verify. No mesh constructed, no first-height/clearance/transfer acceptance claimed. Existing ten-group standalone campaign now runs against updated PN options; then connected MPI2 qualification.

Validation22 standalone67191 terminates exit0 (all10 groups, reusable per-case provenance); MPI34115 terminates exit0 (11 cases/959 assertions on each of2 ranks). All source/runtime/archive pins verify. Geometry-authority stage is validated in this scope only. No automatic curved promotion is enabled. Production field/clearance still knows affine/analytic authorities; v7 controls must not be treated as a qualified adaptation input until those consumers and first-row/wall-refinement paths are connected. Next stage must reach these consumers together, then the difficult M6 and multi-body geometry/mesh targets under unchanged gates.

## Curved query integration in progress

After783bdb7bda, the immutable mapped authority is gaining a bounded closest-distance
query over edge-connected same-marker components, including disconnected bodies
sharing a marker. Reuse the house ADT on complete control-net boxes; a certified
control-hull subdivision bounds the minimum distance. The returned normal belongs
to the candidate point; neither uniqueness nor signed fluid-side clearance is
claimed. Required checks: finite/invalid contracts, budget/storage exhaustion,
known-distance curved points, restart equality, marker/body confinement, and full
M6 query cost/accuracy before connecting clearance and metric consumers.
Production import, BL builder and first-height admission explicitly refuse v7
curved adaptation while those consumers remain incomplete. This closes a silent
flat-substitute hole; decoding/inspection/persistence remain supported.

Caller review also confirms two integration dependencies: FIRST_ROW W currently
selects only metric-long/strip edges and has no SurfaceError refinement demand;
the v7 facet-local topology pins the imported planar-carrier seams, whereas DC17
needs released fitted-plane components for room slides. Curved wall patches and
released planar carriers need an explicit mixed policy before automatic promotion.
Do not remove the new production guard after validating closest distance alone.
Build6487 terminated exit0 (16m25.40s, process peak4,093,440KiB); syntax76043 passed. Incremental build47156 terminated exit0 after the queued review fixes. Serial35231 passed1,025 assertions/12 cases in2.33s; source and runtime pins are preserved.

Read-only audit during build6487 found another required fix: WriteSnapshot in
CNativeBoundaryParameters3D.cpp serializes facets/errors/affine declarations but
omits cubic controls. ConfirmParameterAgreement and CheckReferenceAgreement use
this snapshot, so a same-version control mismatch could escape rank agreement.
After the owned build terminates, include net counts/controls in the snapshot and
add a changed-interior-control regression (unchanged original bound corners).

Curved first-row design constraint from the caller audit: centroid-normal apices
can lie inside a convex curved wall when sagitta exceeds h0. Vertex-normal lifts
can clear the curve but put their normal foot outside the adjacent flat triangle.
A per-face apex near a corner may meet both constraints; shared vertex columns
alone do not prove feasibility. The existing footprint, flat-altitude and positive
clearance gates must all remain. Qualify a coordinated face-apex/transition design
on the M6 leading edge rather than forcing HAUSD to scale with h0.

The post-build fixes include cubic-control counts/values in the shared snapshot,
a same-version altered-interior-control test and collective mismatch regression,
preallocation matching the index input-storage model, and skipping the optional
nearest-line seed for distances whose binary64 square could overflow. The exact
control-box broad phase and outward certificate remain the authority.

M6 query session41622 terminated exit0 (10.09s, process peak314,412KiB), reading the
validated validation22 sidecar. The probe saves each independently checked query
and accepts a first-query index for resumption into a separate receipt file after
verifying source/input/runtime provenance. Planned195 queries cover all sampled
markers at0,1e-6 and1e-3 offsets, distance accuracy1e-8, work cap16,384 and query
scratch cap16MiB. No volume construction or signed-clearance claim.

Validation23 completed:195/195 M6 queries, maximum work2,336/16,384, maximum
bracket width9.93424696e-9, total query time1.5875s, worst query0.22282s, all
three markers covered. Authority resident storage46,639,568bytes. MPI84011
terminated exit0:12 cases/1,026 assertions per rank, including deliberate same-
version control mismatch and atomic import refusal. Serial12 cases/1,025
assertions. Source/input/binary/runtime pins verify after all stages.
All owned jobs are terminal. Next connected work: certified curved clearance in
CheckFirstHeight and geometric normals/curvature for the metric, retaining the
production import guard until support, integration, wall refinement, fitted-plane
sliding and full construction are qualified together.

## Curved clearance and differential integration (active, 2026-10-26)

Local checkpoint73167a10c4 contains validated queries/guards/snapshot agreement.
The next change bounds normal clearance on every subdivision region that can
contain a globally closest point: derivative control hulls enclose the oriented
normal, and interval dot products certify a positive fluid-side projection. An
unresolved sign retains a conservative negative distance bound and cannot pass
first-height admission. This is local oriented-wall clearance, not a new global
embedding proof. The shared CheckFirstHeight keeps its flat altitude/footprint
gates and queries at h0-scaled numerical accuracy; HAUSD/faceting is unchanged.
The curve's first/second polynomial derivatives also supply principal curvature
and directions at the returned binding (local values, not global bounds).
The maximum one-query scratch is included in the mapped geometry workspace
model. Production import and BL field guards stay until remaining consumers are
connected. Acceptance: known cubic-graph curvature oracle, clear versus blocked
apices with identical correct flat height, persisted full-M6 fluid/solid queries,
and the existing geometry/height regressions under unchanged limits.

Review queued while build76284 is active: the old first-height error says an apex
"crosses" the surface, but a conservative nonpositive bound may mean unresolved
sign. After the build handle is terminal, change that diagnostic to uncertified
positive clearance (no acceptance change), and have the M6 probe independently
check -upper <= clearance_lower <= upper. Keep the first-height and HAUSD laws.

The frozen M6 config used for prior qualification requests h0=2e-6 and
HAUSD=1e-4 (thickness.01, growth1.2). The validation24 probe uses offsets
+/-1e-6 with distance accuracy1e-10, plus0 and+/-1e-3 at1e-8, for325 queries
across the same65 sampled facets. Numerical query precision does not change
the geometric faceting limit or the first-height/footprint acceptance law.

Validation24 build76284 passed (16m25.14s, process peak4,095,104KiB);
review build68760 passed after correcting the uncertified-clearance diagnostic
and adding the independent clearance interval assertion. Serial80535 passed
25 cases/2,332 assertions, including the cubic graph oracle and all selected
existing first-height/analytic geometry cases. M6 and MPI checks remain.

Validation24 M6 session3905 terminated exit1 after queries0/1. At facet0,
offset-1e-6, the distance bracket is [1.00000001016e-6,1.00000001438e-6],
but clearance remains unresolved. Independent position/normal coordinate boxes
lose tilted-plane cancellation. Validation25 bounds the degree-four normal
with its Bernstein controls and dot products with intact position controls;
this preserves spatial correlation. The shared normal polynomial also serves
the existing Jacobian proof. A tilted planar tetrahedron regression is added.
M6/MPI acceptance remains pending; all validation24 handles are terminal.

Validation25 completed. Build9945 and probe build70513 passed. Serial20274:
25 cases/2,335 assertions; MPI19269:25 cases/2,336 assertions per rank. M6
session58383:325/325 queries, maximum work2,336/16,384, query time4.5688s,
whole13.06s, process peak314,176KiB. Every sampled inward1e-6 offset has positive
clearance (minimum9.9794276e-7); all three markers covered. Source/input/runtime
pins verify. No curved mesh construction or automatic production enablement.

Next, validation26 calls the actual internal Coverage routine on the persisted
M6 authority and original donor records, independently checking source orientation,
coordinates and one-to-one boundary incidence. This diagnostic includes the
existing implementation without adding a production API or bypassing the import
guard. It uses12GiB requested storage and180s source-work budget. Current coverage
uses global radius4*max(EnvelopeError), and SourceColumn proposes a further4-radius
extrusion; full M6 physical displacement may make this inappropriate. That is a
hypothesis until this admission run supplies a concrete witness. Production metric,
wall refinement, plane seam release and complete construction remain unqualified.

Validation26 terminated: probe build13810 exit0, M6 source58370 exit2. The
full original303,359 donors remain unchanged. Global radius0.00798016999m;
24,479 columns,1,022,435 cone trials, then folded prism at sorted face284.
Source-work22.32s, whole53.02s, process peak469,364KiB. This confirms the
uniform physical-displacement shell is not an admissible curved-source strategy.

Validation27 uses local original-patch displacement radii for cubic authorities,
sharing the maximum incident radius at each proposed vertex. Every original
face's required band is certified against all nearby outer faces (including other
bodies) by reversing the distance broad phase: query original face plus its local
radius against shell-cell boxes, map hits to unique outer faces, then apply the
same outward separation predicate. Positive product cells and the exact global
contact audit remain mandatory. Uniform affine/faceted radii retain their prior
proposal; only the coverage planning direction changes. Acceptance is the actual
full-M6 shell plus existing affine/narrow-gap regressions, unchanged resource caps.

Validation27 also fails at sorted face284/reference7440, a symmetry triangle
with nodes3357,65,64 adjacent to the wing. Source-work44.28s, original303,359
donors unchanged, no shell cells accepted. Global-radius magnitude is therefore
not the sole cause. The local proposal/audit change is preserved as
validation27/local_source_trial.patch and reverted from production. No additional
local factor/limit variants are planned; diagnose the junction and the physical
coverage actually required before selecting a different source construction.

Validation28 junction probe58344/99843 passed and pins verify. Reference7440's
product orientation signs are[-,+,+], whereas the third must be negative.
Node64 local radius7.98017mm proposes(x,y,z)=(.0200974,-.0319207,~0);
node65 moves from z=-.00763483 to z=+.00736976. Incident wing control-plane
projections are approximately[-.00168190,0] (fluid side), despite the positive
absolute displacement radius. Symmetry-only node3357 also inherits3.17461mm
radius from tangential curved-edge displacement. These measured signs diagnose
an unnecessary solid-side extrusion; they are not a whole-domain containment
certificate. Next measure actual curved-surface coverage in original donors and
retain every uncovered witness before designing directional coverage.

Validation29 source probe93601/74640 completed:342,678 actual curved-surface
samples on all48,954 facets.78,209 samples lack original source coverage
(78,179 wing,30 symmetry,0 farfield);15,291 facets have an uncovered sample.
Whole25.46s, process peak407,280KiB. Each facet has an independent receipt;
all planned indices and source/runtime hashes verify. This rules out simply
omitting exterior support. Sampling is not a whole-domain containment proof.
The older source-policy handbook also records a rational counterexample to
nearest-face continuation and rejected wide collars; these remain rejected.

The current source construction and validation30–33 are maintained in
[DC18_CURVED_SOURCE.md](DC18_CURVED_SOURCE.md). Validation32 certifies the full
M6 source shell after replacing unnecessary isotropic displacement with directional
proposals and complete outward control-hull separation. This is source qualification;
complete field/mesh/transfer/restart gates remain mandatory and unclosed.
