# Goal3 production geometric policy and real RANS restart: local MPI PASS — 2026-10-14

Working folder: /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated.
Branch: codex/native-3d-bl; preceding published checkpoint555d2ee6f0.
Cases: integration_evidence/native_3d_bl_v1/production_policy_v1.
Goal3 ACTIVE and incomplete. All owned jobs terminal at this checkpoint.
No cluster submission or actual CFD iterations/convergence in this checkpoint.

Production now uses one actual-query geometric BL policy in partition weights,
worker refinement and coarsening, with a shared frozen sensor-core eigenvalue,
retained original geometry and geometry-resolved edge bands. The ordinary native
remesher/driver/replacement paths perform the operations; Euler-to-BL and BL-to-
Euler remain validation labels, not solver/mesh transition modes. Returned nodal
metrics contain only the original P1 sensor, preventing wall-tensor spreading on
subsequent events. The policy factory is local after common core reduction; missing
policy/bands and differing policy counts reject collectively. Wall-height/metric/
geometry/quality/length and atomic publication gates are unchanged. The blanket
static BL rejection is removed; analytic-wall BL remains explicitly unsupported
by FromNativeReference. New policy phase and retained wall/tree/workspace bytes are
reported. Constructor scratch uses a documented conservative per-facet model.

Actual initialized RANS/SA driver with computed Hessian/metric, conservative current
and BDF histories, requested wall constraints then their absence through a real
CGNS restart, positive/admissible states and SU2/CGNS/reference reload all pass.
N/M=(1/1,2/1,2/2,4/1,4/2,4/4), twelve primary frames independently audited for exact
coverage/volume/no-overlap/manifold geometry, full analytic composed metric lengths
and shape, exact prescribed height, original-P1 sensor transport, flow/BDF integrals
and restart preservation. Strong native checks also cover turbulence history against
the solver's declared lower bound (SA wall-zero transfers are clipped to1e-16).
The first invalid uniform no-slip fixture and unpaired configuration swap were test
errors; negative evidence and source snapshots remain. The final fixture uses an
admissible affine shear field and rebuilds the actual driver from a real restart.

Important findings, not solved by passing height/shape gates: maximum first-row
physical tilt4.7140452079 after coarsening; separate composed-field probes reach
transported-metric ratio24.5183963 although sensor gradation passes. No nodal or
coarse-cell BL correction was spread and no target was changed during reconstruction.
The requested complexity1 is below the retained BL minimum (~3.69416); the code
reports that incompatibility and keeps the wall/sensor demands. These are not proof
of arbitrary field feasibility or aligned multilayer construction. Next major step:
control first-row/transition alignment under joint reconstruction/coarsening, audit
and diagnose composed gradation before freezing, then widen thin/curved/interacting
wall and actual viscous CFD/accuracy qualification. Full performance/affordability
and M6 remain open. Do not claim efficiency from these contended correctness times.

Inspect mpi1/mpi2/mpi4: policy_m*_s0.su2 are independently exported accepted BL
meshes; *_s1.su2 are the subsequent ordinary sensor-target meshes; *.json contain
source/geometry/states/projection certificates, *_written_SU2.su2 and *_written_CGNS.cgns
are actual output/reloads; native.log records exclusive lifecycle timings. The
summary.json, mpi*.json and independent_mpi*.json pin scope and provenance. Existing
seven default/budget/support consumer cases pass MPI1/2/4; only a collector expected
five initially, corrected without repeating unchanged serial work. No broad suite
was repeated after each small fixture edit. Preserve failed native/collector stages.
Goal4 is authorized only after full Goal3 completion. AdapNoExt, foreign ClusterResults
and dependencies remain untouched. One heavy job at a time, nice19/threads1,
build-j1/affinity7,6,3,2; sequential locally available MPI1/2/4.

Earlier entries below are historical.

# Goal3 production geometric policy integration in progress — 2026-10-14

Working folder: /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated.
Branch: codex/native-3d-bl; published checkpoint555d2ee6f0.
Current cases: integration_evidence/native_3d_bl_v1/production_policy_v1.
Goal controller ACTIVE (user resumed2026-10-14). Full Goal3 incomplete.
Previous goal turn made progress: cold/replay nine-case MPI kernel workflow
independently validated and pushed, then architectural no-transition-mode rule
recorded and pushed. New production integration is not validated yet.

Owned build18323 is live: actual configured UnitTests/test_driver and SU2_CFD,
16steps, -j1, nice19, affinity7,6,3,2, threads1. Revalidate that handle before
restarting or altering compiled source. Source changes connect a geometric policy
factory to refinement/coarsening and partition weights, freeze a globally shared
sensor core before worker migration, preserve sensor-only nodal return, and add
policy memory accounting and an exclusive named timing phase. Static support no
longer blanket-rejects configured BL; runtime rejects missing policy/bands, and
existing analytic-wall policy still rejects unsupported analytic BL geometry.
Do not call this production-qualified before its new RANS driver test passes.

[NativeBLPolicyRuntime3D] in CNativeImport3D_tests.cpp exercises initialized RANS
plus computed metric, actual conservative replacement, current+BDF/turbulence,
requested constraints on/off through ordinary remeshing, worker counts1/2/4,
SU2+CGNS reload and reference persistence. No CFD iterations/convergence yet.
After build: run the existing actual-driver test binary with this tag in the case
folder (verify registration; configured meson test_driver includes these objects),
serial first, then MPI2/4; independently audit retained grids/states and extend
scope from evidence. No broad test suite after each small edit.

The production callback is local after the shared core reduction; exceptions vote
collectively. Policy construction uses a conservative per-facet scratch admission
(shortcut explicitly documented); held vectors/tree and future first-thread ADT
workspace growth are charged through existing byte counters. Do not claim calibrated
memory/affordability. Keep goal requirements and wider curved/interacting/sensor+
composed-gradation/accuracy/full lifecycle performance gates intact. No cluster
submission or unrelated process termination. Foreign ClusterResults/dependencies
and AdapNoExt untouched.

Earlier entries below are historical.

# Goal3 cold Euler-to-BL and BL-to-Euler kernel workflow: MPI1/2/4 PASS — 2026-10-14

Working folder: /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated.
Branch: codex/native-3d-bl; preceding published checkpoint ecce0f52fb.
Cases: integration_evidence/native_3d_bl_v1/coupled_engine_v1.
Goal3 remains incomplete; production configured-BL guard is retained.
Architecture clarification2026-10-14: Euler-to-BL/BL-to-Euler are test labels,
not production transition modes. The engine sees target metrics and optional wall
constraints. No reconstruction dispatch by solver label, inferred input-mesh class
or case identity; preserve this rule during production integration (AGENTS.md).
Current joint repair is composition-gated and still needs broader qualification.
All owned jobs are terminal. No CFD or cluster submission this turn.

Cold six-tetrahedron Euler construction now completes the frozen sensor-plus-BL
contracts, followed by BL removal and Euler coarsening, on MPI1/2/4. Nine final
native cases and nine independent geometry/analytic-metric audits PASS: cold
construction plus saved89-cell and100-cell repair/release fixtures on each rank
count. Actual source remains the original six-cell identity-P1 donor field;
wall h0=.1, growth1.2, thickness.3, unchanged fade/core1/no hard-normal reset.
Cold accepted BL cell counts79/79/89; Euler counts6/10/6. Independent exact volume,
coverage/manifold/no-overlap, whole-mesh q/J, full composed lengths and first-height
checks pass. Different rank counts may produce different valid topology.

Implementation reuses existing operators: metric-spring surface movement plus
corner splitting can be coupled with one off-edge insertion through a complete
interior edge-star cavity. The point is halfway toward the existing metric-spring
mean, without case coordinates in production. Blocked composed splits now reuse
MPI vertex-star change notifications: changing neighboring connectivity can make
an unchanged edge reconstructible. Serialized planar composed edits elect global
metric priority and canonical edge ties before owner rank. Final private proposals
still pass the unchanged h0/geometry/q>=.20/J>=.05/edge<=1.8/frozen-target
and atomic publication gates; work4096 and memory/cavity/identity bounds remain.

Inspect validated_cold_construction_v1/: grouped_initial_euler_n*.su2,
grouped_constructed_BL_n*.su2, grouped_released_euler_n*.su2, native receipts,
independent frames/audits, source snapshots and hashed manifest. Original failed
probes and60-second deadline rejections are retained in separate archived folders.
Positive correctness cases now allow300seconds (outer collector420) because the
host became heavily contended; this changes no production setting or mesh/work
contract. Serial CPU3, MPI affinity7,6,3,2, nice19/threads1; one heavy job.
No efficiency or scaling claim follows from these times. Composed commits remain
serialized until concurrent complete-star reservations are qualified.

This is a planar single-wall cube kernel workflow, not full Goal3 completion:
production actual-query composition/complexity coupling, full M<N workflow and
current+BDF/turbulence transfer, actual RANS/M6, curved/interacting walls/features,
separate composed-gradation audit, restart/CGNS, accuracy and full lifecycle
performance remain open. Next substantial step: wire the production BL policy
through the existing remesher/worker/return path, then validate actual driver state.
Goal4 starts only after all full Goal3 requirements pass. The goal controller last
reported BLOCKED even though work resumed on the user's “go on”; no tool here can
set it ACTIVE. Resume it in the controller for automatic continuation.
AdapNoExt, foreign ClusterResults and dependencies remain untouched.

Earlier entries below are historical.

# Goal3 actual joint-engine repair and Euler release: MPI1/2/4 PASS — 2026-10-13

Working folder: /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated.
Branch: codex/native-3d-bl; preceding published checkpoint 4bda1c8651.
Goal3 ACTIVE and incomplete; production configured-BL guard retained.
All owned jobs terminal at this checkpoint. No CFD or cluster submission.

The bounded general metric-directed search finds the previous private witness
without saved production coordinates. The full engine now discovers and publishes
that repair from the saved 100-cell ordinary-marker obstruction using immutable
six-cell Euler donors. MPI1/2/4 all construct101 accepted cells in one transaction
and then release the BL and coarsen to valid Euler meshes (12/6/6 cells).
Independent exact whole-mesh geometry/volume/coverage plus analytic full-target
metric and first-height audits PASS on all three ranks. BL maximum length
1.7770384925, minimum quality0.2127185473, minimum Jacobian0.0522737866;
volume1, first-height error0, tilt0. Euler maxima1.41421/1.73205/1.73205 and
qualities0.39613/0.75595/0.68723; topology need not match across ranks.

Inspect integration_evidence/native_3d_bl_v1/coupled_engine_v1/
validated_engine_replay_v1/: saved_BL_input_n*.su2, replay_grouped_constructed_BL_n*.su2,
replay_grouped_released_euler_n*.su2, receipts and independent audits. The input
has already acquired BL cells; this is an engine repair/release proof, not cold
Euler-to-BL completion. The historical replay_grouped_initial_euler filename is
misleading; saved_BL_input copies its exact bytes with the correct description.
Meshes have no attached CFD solution. The prior explicit witness snapshot remains.

Cold six-cell Euler construction still rejects an89-cell mesh with the surface
corner edge3.15014 and an interior transition edge1.85281. Its operators now admit
complete versioned edge/vertex unions, up to three coordinated moves, interior
splits, and private reconnect-plus-two-insertion alternatives. Fixed vertices
keep coordinates and identities. Target tensors, finer demands, fade and height/
shape/length gates stay unchanged. No experimental hard-normal reset is used.
Extra reconstructions remain private until complete final validation and owner
publication. Work/time/cavity/ID bounds remain explicit; no512 increase.

Important correction: the existing engine already retries primary splits when
recovery repopulates its queue. A suspected phase-transition change was tested,
did not resolve the stall, and has been removed; Recover retains its original
semantics. Reconstruction/operation order remains the cold-start problem.

Composed3D commits are temporarily serialized within their MPI communicator so
expanded moved-star reservations cannot conflict with concurrently accepted edits.
This is a correctness shortcut, not a scaling result; broader concurrent admission
must be qualified before affordability claims. New phases use existing dependency,
reservation, donor, reconstruction and metric timing counters. Contended correctness
runs establish no speedup. Coarse/curved/interacting-wall construction, composed
gradation, production coupling, conservation/current+BDF/turbulence, restart/CGNS,
actual RANS/M6 accuracy and full lifecycle performance remain incomplete.

Current-binary compatibility PASS: joint/witness and interior/curved strip tests,
metric movement, microscopic BL, first-height MPI1/2/4, and composed startup
MPI1/2/4 (including reduced worker subsets). Full engine replay MPI1/2/4 passes;
cold construction remains FAIL. Do not repeatedly run a broad SU2 suite or CFD
until a connected change resolves a concrete gate. The next step is exact replay
of the89-cell cold-start residual to qualify a coordinated transition replacement,
then connect it to cold construction. Goal4 activation is authorized only after
full Goal3 completion, with no need to ask the absent user for cluster jobs.

Earlier entries below are historical.

# Goal3 independently accepted joint witness — 2026-10-13

Working folder: /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated.
Branch: codex/native-3d-bl; preceding published checkpoint 15e5b2575e.
Goal3 ACTIVE and incomplete. Production configured-BL guard remains unchanged.
No active owned job at this checkpoint. No CFD or cluster job launched.

The ordinary wall/far-marker cube still stalls in the engine, with the same
corner edge as the distinct-marker seam case. A saved private reconstruction now
establishes that the ordinary-marker target is attainable locally: split the
corner edge at a biased full-target coordinate and move the two surface
neighbors together, using their complete 14-cell union of incident stars.
The resulting whole mesh has 101 cells and32 points; native frozen-field gates
and an independent exact embedding/coverage/volume and analytic BL audit PASS.
Volume1, minimum quality0.2127185473, minimum scaled Jacobian0.0509273888,
maximum integrated edge1.7936404888<1.8, two wall faces, height error0 and tilt0.
The witness has narrow sliver margin; this is one validated repair, not a
robustness envelope. No sensor/geometry/height/metric gate was relaxed.

Inspect integration_evidence/native_3d_bl_v1/coupled_engine_v1/
grouped_joint_witness.su2 and independent_witness_audit.json. The immutable
validated_witness_v1 snapshot preserves its passing native receipt/log, original
accepted private coordinates, audit and mesh across subsequent campaigns.
These are kernel meshes without flow solutions, not a qualified RANS workflow.

The existing surface-strip primitive now supports exact planar boundary splits
with coordinated complete-star movement while retaining curved off-edge rules,
finite markers, fixed interfaces, positivity and output rollback. Movement
constraints retain original coplanar marker seams and free same-marker canonical
patches. Full-target split placement accepts a bounded fraction in(0,1), default
half unchanged. Native focused split checks and four strip/witness checks PASS. Compatibility
checks for movement/microscopic BL splitting, first-height and composed startup
on MPI1/2/4 also PASS on this binary.
Both accepted and rejected full-engine frames are now exported for auditing.
The resumable runner can select named stages without overwriting full-campaign
failure evidence; fixture and binary provenance remain pinned.

Important: the witness was found by an offline joint placement optimizer. The
simple common damping of metric-spring displacement did not find a complete
repair. The successful points move appreciably tangentially but only slightly
normally; next try bounded metric-direction damping and split placement together,
then integrate complete versioned star admission/atomic publication. Do not
hardcode the witness into production or claim a full engine adaptation from it.
Distinct-marker seam case, Euler release, complete remeshing MPI and all wider
Goal3 requirements remain unresolved. The user authorizes Goal4 immediately only
once the full Goal3 completion audit passes.

Bounded reconstruction experiments are preserved under reconstruction_experiments.
Fixed skins/point sets, selected insertions, swaps and seam edits rejected several
limited models; the unsplit and relaxed controls passed. These floating/MILP
experiments do not prove global infeasibility. Native/exact acceptance is separate.

Earlier entries below are historical.

# Goal3 coupled BL engine checkpoint — 2026-10-13

Working folder: /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated.
Branch: codex/native-3d-bl; preceding published checkpoint 8c4081278d.
Goal3 ACTIVE and incomplete; production configured-BL guard remains.

Actual geometric composition and complete original-source donor coverage are now
exercised by the first-height engine. Target-aware initialization, full composed
length-balanced splitting, metric-weighted movement and prescribed-height tangent
constraints reduce the serial cube pilot from 27 oversized cells to one. This is
progress on a rejected mesh, not a completed adaptation or performance claim.
The final corner edge 0–4 has integrated length 3.150140877566013 (limit 1.8).
The independent analytic/exact-topology audit confirms volume 1, no overlap,
finite facet coverage, minimum quality 0.2127185473, minimum scaled Jacobian
0.0559466249, zero first-height error and zero wall tilt. A coordinated edge split
plus movement of surrounding surface/transition vertices is the next step.

Evidence: integration_evidence/native_3d_bl_v1/coupled_engine_v1.
validation.json records PASS for split-placement, metric movement, microscopic
BL splitting, geometric first-height MPI1/2/4 and actual-composition bounded
startup MPI1/2/4, including reduced worker counts. Full construction serial FAIL;
full construction MPI2/4 deliberately NOT_RUN until serial passes. BL-to-Euler
release is not reached. independent_audit.json reports REJECTED for length.
Inspect initial_euler_n1.su2 and constructed_BL_rejected_n1.su2; these are kernel
meshes, without CFD solutions. Failed variants and resumable provenance remain.
All owned jobs are terminal at this checkpoint; no CFD or cluster submission.

No height/shape/length/work tolerance was relaxed. Sensor-only original P1,
finer sensor demand, fade and immutable actual-query composition remain.
Universal streaming recovery was rejected because it worsened donor work without
improving the mesh; targeted physical-boundary recovery is retained. Curved BL,
composed gradation, production coupling, conservative current+BDF/turbulence,
restart/output, actual RANS and full lifecycle cost remain unqualified.
If full Goal3 is achieved, the user authorizes immediate activation of Goal4.

Earlier entries below are historical.

# Goal3 finite-wall integration milestone — 2026-10-13

Working folder: /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated.
Branch: codex/native-3d-bl; preceding published checkpoint718f2e1c12.
Full Goal3 remains incomplete; production configured-BL remeshing guard remains.
All owned builds and checks are terminal. No CFD or cluster run was launched.

The prior tiny-wall omission was integration aliasing: native and independent
analytic densities agree at retained points to about1e-13, yet two angular rules
missed part of the support. The new hidden-support path uses parallel chords,
projected finite-wall/edge/corner/fade partitions, actual geometric radial bands,
and density-directed angular subdivision. A geometric support guard supplements
scalar error estimates. Projection conditioning avoids nearly vertical source
faces; an exact shared-edge side test excludes zero-volume face pairs. Positive
fan anchors avoid collinear boundary fans without extra centroid panels. Sub-ulp
rounded chords retain their positive original-source barycentric measure.

Original-P1 sensors, actual query composition, finer demands, frozen core, fade,
quality/height contracts and bounds are unchanged. Nonhidden band clipping stays
on its existing path. Returned points65536, refinement work16384 and depth32 remain.
Density error estimates and affine checks are heuristic, not universal certificates.

feature_partition_v1/validation.json now PASS on all nine stages: microscopic
BL, production complexity, immutable original reference, nearest-face changes,
finite hidden support, rotated/sheared source geometry, existing BL kernels,
and production complexity MPI2/MPI4. Hidden support exercises moderate and10x
smaller walls at two positions, unit and finer isotropic sensors, independent
parallel-body integrals, source volume and quadratic moments. Its same-point
analytic check rules out metric evaluation as the former omission's cause.
Unit-density relative errors are below0.011% against the unchanged0.05% gate;
these cases retain about36k–51k points each. accuracy_summary.json contains exact
numbers and links to the hashed receipt/log. This count is still substantial;
these correctness checks make no speedup or affordability claim.

Fifteen failed source variants reconstruct byte-identically using saved base
commits/patches/hashes; source_reconstruction.log records that check. Historical
failed receipts and geometry/cap failures stay preserved. Source reconstruction
is not binary replay. The resumable runner pins sources, objects, binary, build
commands, helper, command, environment and per-rank logs; no full-suite rerun.

Next substantial step: couple actual composed BL and geometric edge bands with
the existing first-height MPI engine, establish a complete accepted coarse-grid
construction plus release/coarsening pilot, and then wire that qualified policy
through the production wrapper. Do not remove the production guard based only
on these quadrature tests. Curved/interacting-wall BL, composed gradation,
conservative current+BDF/turbulence, restart/SU2/CGNS, RANS/M6/viscous accuracy
and complete lifecycle cost remain required. Prior first-height kernel/engine
proofs remain limited initialization/protection evidence, not full remesh success.

Earlier entries below are historical.

# Goal3 target-aware quadrature checkpoint: MPI complexity passes; hidden support still rejects — 2026-10-13

Working folder /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated.
Branch codex/native-3d-bl; preceding published 9a5058b6b035eac771da0723963e1b2c23cddb31.
Whole Goal3 ACTIVE. Last focused runner6070 is terminal; all owned builds/runners
are terminal. No CFD timestep, full adaptation or cluster/performance campaign.

New IntegrationRule3D density callback lets hidden-support adaptation monitor the
actual bounded/graded original-P1 sensor plus actual-query BL composition and
frozen core, rather than a geometry-only surrogate. CSolver builds rules after
sensor grading/synchronization, reuses only byte-value-equal four donor tensors
and core within the immutable geometry lifetime, retains storage/cell diagnostics,
and reports accumulated target-quadrature time/build/reuse counts. 2D trial cadence
is preserved. A callback absent from direct kernel calls monitors unit-sensor BL.

Prototype: positive boundary-vertex polyhedron decomposition matching the house
clipper's Moments; affine-distance probes route to existing band clipping;
nonlinear regions use largest-estimated-error angular subdivision, degree5 retained
vs degree6 comparison rules, local3/5 radial order choice checked against5/7 probes,
and both radial errors included in the summed1e-4 density-error estimate. Radial
absolute allowance is volume-weighted alongside relative error. Original source
barycentrics/actual rounded query points and sensor/BL/fade policy stay unchanged.
Returned-point65536, work16384 and depth32 guards remain. Sampled affine detection
and embedded error estimates are not rigorous general curved-field certificates.

Latest feature_partition_v1/validation.json FAIL: seven stages PASS, hidden support
FAIL. Serial microscopic1case7263assertions; actual complexity1/82; original ref1/5;
nearest-face1/4757; existing BL19/3486. Actual complexity MPI2 passes43/40 assertions
and MPI4 passes22/19/22/22. Hidden support still reaches the returned-point ceiling:
retained65411, panel896, cones105, local125, estimated_error0.002544. Its diagnostic
density2.471511 is PARTIAL during panel replacement, not a completed integral and
cannot be compared as an accepted estimate to the independent oracle2.4775664410.
The extra finer-sensor parallel-body oracle is written but not reached/qualified
because unit-sensor construction rejects first. No gate, metric or cap was relaxed.

Ten prior adaptive failures have complete source patches/base/hashes and saved
receipts/logs. verify_experiment_sources.py reconstructs all ten byte-identically
in disposable /tmp files (source_reconstruction.log PASS). This proves saved source
versions, not binary replay. The campaign now saves/resumes explicit MPI stages and
requires a passing test record per rank. The first source/build guard refusal after
edits during a live build is preserved; affected objects were rebuilt before tests.

Next: change geometric integration strategy or error allocation substantively.
Do not repeat cap/order sweeps or accept using the known oracle. Current rule still
spends too much on the finite-wall transition; qualify hidden support and stronger
sensor independently before full remesher enablement. Existing first-height engine
MPI checkpoint9a5058 remains qualified within its limited scope. Curved/interacting
BL construction, full metric completion/removal, gradation, transfer/RANS/restart/
output and complete performance remain mandatory. Production BL guard unchanged.

Earlier entries below are historical.

# Goal3 first-height engine transactions pass MPI1/2/4 — 2026-10-13

Working folder /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated.
Branch codex/native-3d-bl; preceding published 8a7a749483bfad181ee56d2e98bed58e8eb57418.
Whole Goal3 ACTIVE. Builds77333/41176 and focused MPI runner93770 are terminal;
no owned heavy job remains live. This step used no CFD or cluster runs.

EngineOptions now carries a bounded, collectively consistent frozen marker-height
policy. Before metric seeding, complete physical face-star transactions construct
private wall children and their transition with reserved identities, owner routing
and Directory publication. Accepted input and original sensor donors are unchanged.
Every private split/collapse/move/reconnection/joint reconstruction checks height;
strict metric completion separately checks it again. Empty policy releases height.
Initialization shares existing work/time/memory ceilings. Its exclusive timer and
height/publication counters are exposed in existing remesher detail output.

Evidence: integration_evidence/native_3d_bl_v1/first_layer_engine_v1/validation.json
PASS on MPI1/2/4, two cases per rank, including N=M/M<N migration/return, immutable
input, independent first-height checks and complete physical skin. Impossible
height rejects without a private publication/input change; one-transaction budget
retains a conforming rejected private mesh and unchanged input. Inconsistent/NaN
policies reject collectively before mutation. Latest assertion counts: MPI1 22;
MPI2 29/34; MPI4 38/33/33/43. Wall checks are local counters, so their test assertion
is collective; initial MPI4 failure (empty-wall workers had zero checks) is retained.
No mesh contract or acceptance gate was weakened. Local test elapsed values are
not performance qualification.

LIMITS: the one-round initialization/protection probe does not assert full remesh
success and uses a constant sensor, not production composed BL. Production BL guard
is unchanged. Curved/interacting-wall construction, coordinated repair of blocked
surface edits, full target completion/removal, transfer/RANS/restart/gradation and
whole-lifecycle performance remain required. Hidden-wall quadrature still fails
its unchanged 5e-4 gate by 0.47%; use error-controlled integration next, not higher
fixed orders or relaxed caps. Full requirements retained in GOAL3_REQUIREMENTS.json.

Earlier entries below are historical.

