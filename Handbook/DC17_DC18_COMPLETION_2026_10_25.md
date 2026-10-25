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
