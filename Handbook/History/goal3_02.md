# Goal3 joint first-height/transition kernel qualified locally — 2026-10-13

Working folder /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated.
Branch codex/native-3d-bl, preceding published c34b2cd3c1b21bacd19e78fb0c478e4ce1c02163.
Whole Goal3 ACTIVE; no owned heavy job remains live (last build73546/check37097
are terminal). No CFD, MPI or cluster campaign was run in this step.

New ConstructFirstLayer kernel reconnects a complete admitted cavity around a
reserved background and carves independent centroid-normal wall apices at h0,
including their transition. It reuses Cone/Finish/fixed-interface and immutable
geometry validators. CheckFirstHeight reports unique wall coverage, relative
altitude error, tilt, worst facet/cell/apex. Empty height policy releases constraints.
Local reasons distinguish geometry-height incompatibility within admitted volume
from background obstruction; no global infeasibility or sensor override claim.
All output remains unchanged on rejection; identities/ranges are caller-reserved.

first_layer_kernel_v1/validation.json PASS:4cases/944assertions, repeated height,
exact pairwise conformity/no overlap, unchanged interface, wall junctions, atomic
rejection and independent tilt/clearance diagnosis. Independent_geometry.json PASS
checks positive volume, unchanged original boundary/coordinates, altitude/coverage/
alignment and exported SU2 hashes. Grids are6/18/18tetrahedra at noBL/h0.05/h0.025.
They are private kernel geometry, not accepted CFD adapted grids or qualified metrics.

Evidence/testcase folder: integration_evidence/native_3d_bl_v1/first_layer_kernel_v1.
Inspect first_height_constructed_kernel.su2 and first_height_updated_kernel.su2.
Per-case source/build/object/binary/helper/log pins and original phases are retained.
The exporter reuses SaveCurved; standard-library independent checker adds SU2 output.

Next: integrate height constraints and joint proposals into existing native MPI
selection/dependency/reservation/publication/repair/completion paths, preserving
valid wall children during all edits and releasing them for BL-to-Euler. Wire the
shared actual-query geometric field into the remesher/return/complexity path.
Keep production first-height/analytic guards until actual engine/driver qualification.
Hidden-wall density integration remains FAIL0.47% and requires error-controlled
angular integration, not more fixed orders or a relaxed gate/cap. Full analytic/
feature/interacting-wall/composed gradation/actual RANS/Euler-BL/MPI/current+BDF/
admissibility/SU2+CGNS/reference/restart and lifecycle performance remain mandatory.
Preserve Goal2; leave AdapNoExt, foreign results, dependencies and other workers
untouched. AGENTS connected-work/provenance/resumable validation policy applies.

Earlier notices below are historical.

# Goal3 feature/ray integration diagnosis; angular accuracy unresolved — 2026-10-13

Working folder /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated;
branch codex/native-3d-bl, preceding published29c29f013e1f1621fc2186c3e74b9de3d6b811b4.
Goal ACTIVE, full original scope intact. All owned jobs are terminal (last build
37998/check52174); no CFD/MPI/cluster campaign was run.

New feature-aware triangle partitions plus distance-resolved conical integration
fit the unchanged65536point budget (latest65397), but the stronger hidden-wall
density oracle FAILS:2.4658690877 vs2.4775664410,0.47% low (gate5e-4). Volume/
moments/source containment and the five other original stages pass. DO NOT
promote feature_partition_v1/validation.json; full thin-volume milestone remains
INCOMPLETE. Frozen targets/shape/length/caps have not been weakened.

Fixed angular orders are not a reliable fix:3point/5radial gives2% high,
7point gives0.39% high,12point/3radial gives0.47% low; restoring radial order can
exceed budget. Certified outside pieces use the original sensor rule. Local
coordinate arithmetic/bounded generated-sample interior placement resolves source
rounding while still using exact actual-query original P1. Edge-band filtering
retains actual nearest-wall roots with coordinate-roundoff allowance; normal/fade
composition and finer sensor requests stay unchanged. Public fade-band count is
an internal quadrature parameter, not a new config/target policy.

Evidence: integration_evidence/native_3d_bl_v1/feature_partition_v1.
Stronger oracle uses independent convex-triangle parallel-body density integral;
raw failures/receipts/aggregates are preserved. Planes-only source patch was
reconstructed and hash-verified; final sources/patch are published. Other transient
variant receipts are historical unless their exact source is explicitly available.

Next big steps: ERROR-CONTROLLED ANGULAR integration, not more fixed-order/cap
increments, and coordinated first-height wall-plus-transition construction using
existing coning/cavity/atomic MPI infrastructure (LAYER_CONSTRUCTION_NEXT.md).
The full production first-height/analytic BL guards remain. Preserve goal's full
analytic/feature/interacting-wall/sensor-plus-BL gradation/actual RANS/Euler-BL/
MPI/current+BDF/admissibility/reference/CGNS/restart and lifecycle performance
gates. Preserve Goal2; leave AdapNoExt/foreign evidence/dependencies/other workers
untouched. AGENTS connected-work/provenance/resumable validation policy applies.

Earlier notices below are historical.

# Goal3 thin-volume milestone INCOMPLETE; hidden-support point budget — 2026-10-13

Working folder: /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated.
Branch codex/native-3d-bl; preceding published checkpoint cb2c645590ba78947bccfed1073d1adebdccf0a8.
Entire Goal3 remains ACTIVE. No owned heavy job is live: builds77854/23926/
18649/15918 and checks46838/44014/7716 are terminal.

Production3D thin-BL complexity and retained faceted wall references are implemented.
Five stages PASS on the latest actual binary: microscopic tetrahedron integral/
volume/moments/reversed orientation (19263assertions), production coarse-cube
complexity with sensor-only tensors (82), immutable-reference extraction (5),
changing nearest face (11093) and existing19 BL cases (3486). This proves serial
kernel/frozen metric construction, not an actual3DBL remesh or MPI/RANS workflow.

Full volume_integration_v1/validation.json is FAIL: hidden finite-wall support
still exhausts the unchanged65536-point budget. Initial coarse SAT alone failed
at depth32; closest vertex/feature separating directions resolve that uncertainty,
but spatial subdivision/clipped-piece quadrature remains too expensive. Direct
four-point quadrature for already-tetrahedral pieces removes redundant coning,
retains moment/accuracy gates, but does not complete the hidden-support case.
Do NOT promote the aggregate, increase caps, change the target or quietly exclude
this case. Next algorithmic step: feature-aware geometric partition/integration
so finite-wall support is localized before expensive spatial subdivision. Continue
coordinated construction work as appropriate without redefining the full goal.

Initial complexity/reference fixtures were invalid because hmin>.firstHeight;
CConfig correctly rejected them. Corrected test-only HMIN values pass with the
same expected sensor identity/lower-complexity bound. No configuration check was
weakened. Every failed/complete stage log and hash receipt is preserved. The
support_fix_experiment.patch reconstructs the intermediate source from cb2c645590;
its source hashes were checked against saved receipts. The final failed aggregate
is archived; future runs save whole aggregates as well as per-case receipts.

Evidence folder: integration_evidence/native_3d_bl_v1/volume_integration_v1.
The runner refuses stale builds, pins source/build/object/binary/helper/log hashes,
and runs independently bounded cases on one sampled CPU/nice19/threads1. No CFD
timestep, published adapted BL mesh, cluster or performance qualification. The
production first-height guard and analytic BL-distance guard remain intact.
LAYER_CONSTRUCTION_NEXT.md records the next joint wall/transition approach and
coupling-aware local height/metric feasibility diagnosis. Full first-height,
analytic/feature/interacting walls, composed gradation, actual RANS/Euler-BL/MPI/
current+BDF/output/restart and lifecycle cost gates remain required. Preserve
Goal2; leave AdapNoExt/foreign results/dependencies/other workers untouched.

Earlier notices below are historical.

# Goal 3 volume/complexity implementation; grouped build ACTIVE — 2026-10-13

Repository: /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated.
Branch: codex/native-3d-bl; previous pushed kernel checkpoint
2cd69c17bead823ade7b5bb544846f4182e4f590. Entire Goal3 remains ACTIVE.

Next connected implementation is present but UNVALIDATED: positive degree-two
3D quadrature after BL distance-band clipping, reusing ConvexClipping.hpp in
original barycentric coordinates. Changing nearest faces and hidden wall support
trigger bounded longest-edge subdivision. Exact original-P1 weights are recovered
at actual rounded quadrature coordinates. Work/depth/piece/point caps reject;
metric complexity has an aggregate2GiB requested quadrature storage bound.
Geometry samples are reused across complexity/gradation trials; stored tensors
stay sensor-only. Actual global cell IDs accompany collective integration failures.

Native ComputeMetric now consumes the3D rule and optional retained3D reference;
steady/window/PREDICT calls carry that reference. PrepareReference loads/validates
sidecars or imports initial immutable geometry before BL metric construction.
Configured analytic BL distance/normal authority remains explicitly unsupported;
production3DBL remeshing stays guarded until first-height/transition construction
is qualified. No goal requirement has been removed or weakened.

Build session77854 is the sole owned heavy job, confirmed live on the latest poll.
One compiler job, sampled CPUs5/3/7, nice19/threads1; no CFD/MPI/cluster campaign.
The public ComputeMetric signature change requires127 dependent objects in this
single grouped build. Do not restart from elapsed time/log quietness. Verify the
handle. No source edits after this checkpoint unless needed; check Ninja dry-run
once terminal and finish any remaining affected objects before validation.

Test folder: integration_evidence/native_3d_bl_v1/volume_integration_v1.
run.py saves five independent new cases and the19-case existingBL kernel stage;
source/compile-command/object/binary/helper/log pins govern reuse. It refuses an
incomplete/stale build. Each case has180s bound. Planned proof: microscopic layer
vs independent integral, positive weights/volume/moments/reversed orientation,
production coarse-cube complexity and sensor-only tensors, original-wall
extraction, nearest-face switch and hidden finite-wall support. No new tests have
run yet. Prior44 executions qualify the previous kernel checkpoint only.

Next: finish this live build; run the focused resumable checks, preserve any real
failure and improve its shared cause. Then add coordinated wall/transition
construction using existing coning/cavity/MPI transactions. Analytic authority,
height/alignment/coverage/feasibility, separate composed gradation, actual RANS and
Euler/BL transitions, MPI/transfer/restart/output and lifecycle cost qualification
remain mandatory. Preserve Goal2 evidence; leave AdapNoExt/foreignClusterResults/
dependencies/other workers untouched. Permanent AGENTS batching/reuse rules apply.

Earlier notices below are historical.

# Goal 3 ACTIVE — 2026-10-13

Repository: /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated.
Branch: codex/native-3d-bl, based on published Goal2 checkpoint
c43a8a3907ea2655e4ebbb6286ab16c3fa625d8b. Goal2 remains COMPLETE.

User explicitly activated Goal3: robust coarse Euler-to-BL and BL-to-Euler
native tetrahedral adaptation, with performance central and minimal wasted effort
and SU2 runs. Full scope remains NATIVE_3D_DEVELOPMENT_GOALS.md Goal3 and the
active goal tool; no metric-only prototype may establish completion.

First connected implementation: shared 2D/3D faceted-wall distance-band chord
breakpoints; native3D composed edge quadrature through exact sensor-source
intervals, retaining original P1/finer demands/fade and actual-query composition.
Engine/private/proxy fields receive the frozen geometry-band callback.
Sensor-only edge integration retains the existing analytic path. Missing/invalid
bands, quadrature failure or work exhaustion reject explicitly. The production
3DBL guard remains until first-height construction and complete qualification.

Focused checks added: microscopic flat-wall layer/reference integral, reverse
chord, finer sensor preservation, no coarse-cell tensor spreading, hidden finite
wall missed by endpoints/midpoint, malformed callback and sensor-only equivalence.
Focused builds43338/57536/61486/36229 and validation45064 are terminal.
No owned heavy job remains live. Initial3D reference failure was isolated to
existing wall-metric spectral cancellation (full eigenvalue1.0016903085788469e12
vs1e12 at h0=1e-6). Fixed primal normal/tangent inversion/log-fade passes final
source/build/object/binary-pinned checks:19 BL cases/3486assertions plus25 field
and metric-acceptance cases (see validation.json for exact totals). Original
failed logs and direct-point diagnosis remain preserved. The production3DBL
first-height guard remains. Active-scalar wall inversion retains its old path;
AD runtime is not qualified. Separate nested edge geometry/composition times and
bounded breakpoint scratch are included. No CFD/cluster/performance run.

Evidence and resumable helper:
integration_evidence/native_3d_bl_v1/edge_integration_v1.
Validation pins relevant sources, compile commands, objects, binary and helper;
reuse requires all pins and stage log hashes. This is serial kernel qualification,
not 3DBL remesh/MPI/viscous/complexity qualification. Entire Goal3 remains ACTIVE.

Next: inspect focused build/test outcome, implement thin-tetrahedral complexity
quadrature and immutable3D/analytic wall authority, then coordinated prescribed
height/transition construction with feasibility/coverage diagnostics. Reuse all
qualified Goal2 topology/MPI/transfer/restart infrastructure. Separate sensor vs
composed gradation audit; preserve ADAP_HESSIAN_NOISE=0. Actual RANS, repeated
Euler/BL transitions, MPI1/2/4 and full lifecycle performance are later mandatory
Goal3 gates. Follow AGENTS batching/provenance/resumption policy. Push reviewable
checkpoints, label incomplete work; leave AdapNoExt/foreign evidence/dependencies
and other workers untouched.

Earlier notices below are historical.

