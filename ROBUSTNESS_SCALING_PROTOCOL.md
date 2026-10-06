# Practical robustness and scaling protocol

Predeclared 2026-10-06, before integrated pilot results. Campaign complete;
original definitions/stopping policy below are retained. Current outcomes and
requirement proof are in ROBUSTNESS_SCALING_RESULTS.md and
INTEGRATION_COMPLETION_AUDIT.md.

The deliverable is an observed envelope, with failed cases preserved, not a
claim that every anisotropic mesh can be adapted. Current native scope is
static single-zone primal-double 2D triangles on an immutable marker polyline.
Native3D, CAD projection, mixed volumes, time-domain, moving meshes and adjoint
native adaptation remain unsupported. MMG and Stage G are separate controls.

## Evidence classes

- Integration defect: build/API/routing, output, restart, transfer or collective
  failures introduced by combining branches. Fix and rerun affected gates.
- Feasible construction failure: a valid input and compatible metric/geometry
  ends with incomplete quality or coverage. Preserve residuals and rejection
  reasons; the driver must retain the accepted solution and mesh.
- Incompatible request: demonstrate a necessary-condition violation; report it
  separately and verify safe rejection. In the constant diagonal engine probe,
  h0/sqrt(n^T M^-1 n)>1.8 makes the prescribed wall altitude incompatible with
  the metric length bound. This is a manufactured-case proof, not a general
  feasibility detector for a spatially varying tensor.
- Resource/work limit: dependency/donor/round/time caps stop progress. Record
  the actual cap and residual contract; do not relax the target or fall back.
- CFD limitation: nonconvergence/divergence is recorded separately from valid
  remeshing/solution transfer. Existing Stage G notes identify a four-rank
  discrete-adjoint NACA divergence; it is not evidence of a native mesh failure.

## Ordered campaign

1. Fresh integrated primal MPI/MMG/CGNS build and focused1/2/4 regressions.
   Independent geometry/target/output audits, native allocation probes, default
   MMG controls, custom sensors, two-pass references and adjoint transfer.
2. Engine pilot: tiles1/4/16/64, cyclic ownership, AR10, ranks1/2/4, one fresh
   process per case. This gives16/64/256/1024 input triangles. Each job has a
   240-second budget. Stop escalation on a timeout or structural failure.
3. Independently audit the raw meshes (exact orientation of binary64 inputs,
   topology, perimeter markers, area, target quality/length and wall heights).
   Incomplete contracts are outcomes; invalid topology is a failing gate.
4. Escalate engine size (tiles128..4096, stopping on incomplete output, a
   240-second timeout, structural failure or120-second observed engine cost),
   using vertical geometric strips. Repeat the largest passing size three times
   per rank, with cyclic, contiguous and vertical geometric ownership. The
   original j-major contiguous blocks form horizontal strips with long cuts;
   vertical strips provide a low-cut control. All three are prescribed before
   repeated measurements; publish initial cut-edge counts. Compare a fixed input, target and contract; also publish
   output cell counts and commit counts, because rank-dependent work is not
   identical-work strong scaling. Do not quote reliable speedup from one sample.
5. Matched geometry/tensor anisotropy: AR10/100/1000 at tiles4 and1/2/4 ranks.
   Match h0 and the domain height to the tensor so metric-space difficulty stays
   fixed; this tests affine/precision robustness without adding target demand.
   Separately test MATCHED=2, AR25: the metric wall height is2.5>1.8 and therefore
   incompatible. Check that it is reported incomplete and structurally valid.
6. Real CFD/transfer control: current NACA0012 fixture at1/2/4 ranks, three
   changing levels4000/6000/3000, original h0=2e-4, adaptive physical boundaries,
   conservative transfer and resumed viscous solve. Audit every actual frozen
   P1 sensor/BL field and reference, output/restart and owned/halo admissibility.
7. Increase one demand at a time: complexity4000/12000/3000, then lower h0/HMIN
   together (1e-4, then5e-5). Use one-rank diagnosis followed by2/4 controls;
   stop at the first demonstrated failure/cost boundary. Preserve failed
   donor/target/candidate/accepted-state snapshots before deciding on a fix.
   Reuse the opposing-wall eight-cycle fixtures for changing tangential demand,
   coarsening and h0, with both transfer policies. Add a new narrow-gap fixture
   only if the existing coverage leaves a concrete unresolved question.
8. Fresh AD build: Stage G mesh-swap/recording-counter controls at1/2/4,
   actual warm/cold goal loop and interruption checks, explicit native rejection.
   Publish inherited solver limits instead of masking them with mesh changes.

## Measurement and report

Separate engine initialization, remeshing, transfer/repartition, CFD, output
and audits. Report wall-clock maximum across ranks, repeated distributions,
whole-process VmHWM, owned-cell imbalance, accepted/cross-owner transactions,
selection scans/time, MPI traffic/collective time and actual admission peaks.
The engine probe excludes its audit gather from timing/traffic, but its RSS
includes process startup and the replicated manufactured input. Its counters
cover World-instrumented exchanges/collectives, not every MPI implementation
allocation/call. It does not establish whole-process memory bounds.

The default2MiB transaction-work budget,256 donor cells,128 discovery regions
and64-entry candidate cache are explicit implementation limits; the replicated
original boundary and reader master boundary rows are O(B). Current scans,
all-rank metadata and ownership conflicts are candidate scaling bottlenecks.
Measure which dominates before changing the scheduler or data structures.
Publish the largest passing tested mesh and the first failure/timeout, with
hardware/load, source/executable/input hashes, commands and outputs. No general
large-rank/3D scalability claim follows from1/2/4 measurements.

Measurement scope clarification: World traffic/exchange-work/collective counters
include engine initialization and adaptation, while adapt_seconds excludes
initialization and selection_seconds_max measures adaptation selection only.
The maximum exchange-work counter can include the initial incidence-directory
transport, which uses no transaction ceiling; a value above2MiB is not itself
an admitted transaction violation. The probe enlarges a tiled rectangular strip
at fixed ny=4 and constant target, so boundary size grows with input cells. It
measures that workload, not a fixed-domain general2D/3D scaling law. Reference
construction and the SU2 import/transfer/CFD path are outside its timing.

## Predeclared follow-through after the common-size cost boundary

The serial16384-cell job exceeded its240-second budget on2026-10-06. This does
not establish the higher-rank boundary. After the main campaigns, measure
vertical strips at16384/32768/65536 input cells on2 and4 ranks, stopping each
rank's size axis on incomplete output,240-second timeout, structural/process
failure or120-second measured adaptation cost. These are single-sample capacity
controls, not additional repeated speedup estimates. Independently audit every
completed candidate. Run2/4 controls for each successful serial real-airfoil
demand axis, then warm/cold actual goal lifecycle at4 on the corrected AD tree.
Unexpected failures require classification before continuing. This ordering
retains one own heavy job at a time and MPI<=4.

Encoded payload-byte counts include self buckets and are not measured network
traffic. World collective counts/times omit direct CPassiveComm calls and
CollectiveFailure votes; they are not a complete MPI profile. Hardware is an
8-core Intel Core i7-9700 with no SMT. The campaign records initial load (5.63,
4.53,4.72); this is a shared workstation, not an exclusive benchmark allocation.
MPI uses the installed OpenMPI defaults without an explicit binding override.


## Completion-gap control: abrupt nodal target changes

Inspection of the executed native fixtures found a smooth Gaussian tangential
metric and affine/constant targets, rather than an abrupt spatial target request.
After the binary-pinned campaigns finish, add the prepared NativeBLStep2D
control by reusing the eight-cycle BL fixture: tangential length .004 for nodes
left of the moving center and .014 otherwise, same h0 sequence, geometry,
transfer policies and targets. This is an abrupt nodal request; SU2's retained
P1 interpolation remains continuous inside donor cells. It does not claim
support for inconsistent cell-discontinuous values at a shared vertex.

Diagnose on1 rank, then control at2/4 with a fresh evidence label. Use all four
existing one/opposing-wall and barycentric/conservative combinations. Independently
audit every accepted snapshot against its actual frozen donor field. Preserve and
classify incomplete candidates; check that original reference, point count and
all accepted flow values remain exact before reporting an observed construction
limit. Reuse the existing saved-mesh auditors; no target floors or new meshing
algorithm. Executed closure: the patch is applied and compiled. All96 step snapshots and
all96 unchanged smooth snapshots independently PASS on1/2/4; see
native_step_support_complete_v1.json. The original declaration above remains
the test definition, not a pending instruction.


Measurement closure refinement: use prepared pending_airfoil_phase_timing_v3
instead of v1/v2. v3 additionally exports actual donor/transferred flow CSVs
paired with their mesh IDs/coordinates, outside timers; they are not restart files. Besides independent phase MAX values, retain local rank duration
rows, cumulative Linux VmHWM, owned/total CFD points and local elements including
overlaps. Validate maxima against local rows and ownership against the actual
adapted mesh with check_airfoil_phase_timing.py after successful runtime. RSS
includes process startup and earlier test snapshots; it is not attributable to
one phase and phase maxima are not additive. Patches/checker self-check alone
are not measured evidence. Both patches were applied after the pinned campaigns finished. All1/2/4
instrumented runtime cases now pass; all9 independent phase/target/field verifications PASS in
integrated_airfoil_phase_timing_v1.


Actual main-loop/output coupling closure (historical declaration):
integrated_native_production_v1 was staged to run the real SU2_CFD application on1/2/4 with the short baseline profile, local
input mesh, noncompact double binary restart (including actual metric), ParaView
flow fields and accepted mesh/reference outputs. Its queued supervisor starts
only after robustness/AD/follow-through success and machine quiet. Pair restart
fields exactly with each donor grid using existing readers, and audit all nine
actual restart-derived frozen-P1 targets against the original reference.
This additional gate addresses the derived fixture's missing Output call; it
does not replace phase timing, step-target tests or full CFD convergence proof.
That freeze is discharged: corrected production_v2 runtimes and all9 actual
target audits passed under chain_v3 before either test-only patch was applied.
native_production_complete_v1.json is the completion proof.


Sequencing correction after executed failures: cold2 goal-runtime divergence was
reproduced on its saved mesh/flow by a fresh ordinary static adjoint process,
with all131 printed trajectories agreeing to one residual print unit. It is a
classified CFD solver limit, not a successful lifecycle. Its followers stopped
without launching. Independent primal production/output validation resumes only
after that classification, with fresh labels; remaining AD/interruption/capacity
requirements are unchanged. The first production attempt failed at config parsing:
use OUTPUT_FILES=(RESTART, PARAVIEW), as the repository enum defines. Preserve
both failed attempts and the production runner/checker/binary remained frozen while
native_production_chain_v3 ran to terminal0. Actual cases are integrated_native_production_v2.
