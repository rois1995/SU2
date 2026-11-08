# Final two-sided certification after M6 shape closure

Goal unchanged: two accepted Euler50k meshes, conservative admissible transfer,
finite adapted CFD and isolated adapted disk restart; then dedicated BL/MPI and
RANS500k. Zero accepted meshes. All strict gates/budgets and CAD optionality remain.

Build274 stopped normally inside native adaptation with time-budget refusal,
complete734806cells/zero q/J/localgeometry/size failures. Final two-sided geometry
proof was incomplete. Its full148781point rejected mesh with captured nodal metric
is saved and independently verified, including every raw cell-ID measurement join.
All pinned files were verified before exactly one owned MPI stop; all handles are
terminal. See handoff and staged57/m6_runs/euler_build274_metric_cleanup_mach_only_real_cfd50k.

From entry25035 to exit25036 each rank added ONE full geometry query taking~1903.6s,
~651–659M traversal work; preparation <.4s. Rank3 added ONE repair commit/+22work/
+6cells; other ranks no repair. Cleanup never executed. Final traversal is the
active demonstrated blocker, distinct from the leading-edge cleanup hypothesis.

Complete GeometryCertificates traverses its ordered body/sheet/marker map. Actual
first key(0,0,0) has2060reference facets; preserved current mesh has2827physical
faces. prepare_final275_query.py extracts exactly that support/physical union from
verified reference/SU2, preserving all coordinates. Diagnostic276 keeps production
CFD byte-identical274 and runs ONE source-to-mesh query with1Mwork. It exhausts
that budget in2.7467s;992982work/2.650s is traversal. This establishes expensive
source traversal on actual inputs, not the exact failing source subdomain.

## Connected correction under qualification

Reuse the existing rigorous flat triangle/deviation construction from target
preparation. For cubic controls Cq and the degree-elevated chord controls Lq,
Bernstein weights are nonnegative and sum to1. The complete curved source differs
pointwise from its chord by at most maxq(norm(Cq-Lq)), computed outward with the
existing interval arithmetic. Distance to a closed target union is1-Lipschitz.
Therefore an upper bound for the entire chord plus this deviation is conservative
for the entire cubic patch. Existing N=3 target bounds can certify the actual
triangle footprint, where N=10 uses an enclosing rectangle and weaker lift bounds.

The shared UnionChord extraction keeps target preparation operations unchanged.
Model-source traversal charges10additionalwork for its coefficient scan. Only when
the deviation can fit the requested tolerance is an N=3 upper witness attempted;
addition rounds outward. If it does not pass, the original N=10 witness remains,
using the smaller conservative bound. No direction/support/tolerance/association
or final proof is removed. All uncertainty/resource exhaustion remains fail-closed.
No new cache identity, geometry representation, config or scheduler.

Affected callers: final Both-direction certification; ModelToMesh source-envelope
queries used by BuildJointSourceEnvelope/SourcePatch::Tube; direct full union API.
Physical-to-model-only construction queries retain their existing source path.
Source tubes may tighten but must still enclose the complete declared authority;
independent swept-patch separation remains mandatory. Existing strict final Both
query bypasses partial revision cache exactly as before.

Qualification: actual saved query at the same1Mwork cap must demonstrate traversal
savings; focused flat/slightlycurved/largebulge and missing-footprint controls;
serial cache/recompute, curved seams, holes/bodies, accepted faces, import/height/
resources; ONE connected MPI6 milestone with empty ranks/publication/census cases.
Then immediately same-seed full two-event M6. Any local proof is not mesh acceptance.
No speedup or full-final-cert completion is claimed before measured results.

277bounded actual source query now COMPLETE:194957work/.659146s,187939traversalwork/
.565312s, certified bound9.5858962087e-5. Old276sameinput exhausted1Mwork/2.746702s,
992982traversalwork without a certificate. This is a bounded witness improvement,
not a measured full-run speedup. Focused flat/tinybulge/largebulge/missingface and
actualquery passed10576assertions/2cases; allpins/ownedhandles terminal. 277build
49.157s; CFD SHA c8ac77f8acc1c1d389af440989bd3ba5e5616b62613ccce7a3e09a878d24492a.
Read-only source review found no concrete safety defect; it is not validation.
Test-only278 adds the mandatory Both query on the SAME actual saved entity after
the source bound completes, sharing the unchanged1M totalwork cap. No production
change from277. Build live; qualify serial and ONE MPI before fullsame-seedM6.

278test-only build PASSED43.054s/productionCFDbyte-identical277. Actual saved first
entity Both proof now PASSES, plus source query, within TOTAL442406work/1.543184s;
source194957work, modelbound9.5858962087e-5, reversebound2.2833012834e-8. All pins
unchanged; focused10578assertions/2cases12.649s. Serial live; ONE MPI milestone due.
This proves the bounded actual first-entity replacement, not all remaining model
entities, a certified complete mesh, full-workflow speedup or accepted transfer.

Connected278serial PASSED84cases/62994assertions154.265s, allpins unchanged/all
ownedhandles terminal. ONE MPI6 milestone LIVE; no otherheavyjob. Immediate full
M6 follows verified qualification; keep sources frozen throughout. Goal unchanged.

ONE278MPI6 milestone PASSED118cases on EACHrank169.654s, allpins unchanged and
allownedhandles ABSENT. Qualification sealed inR/certification278_qualification.json
with exact query/serial/MPI/source/binary/runtime/input hashes. No further fixture
campaign before immediate fullsame-seedTWOeventM6. Still ZEROaccepted meshes.
