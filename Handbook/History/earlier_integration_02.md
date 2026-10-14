## Resumed unsteady and performance work — 2026-10-07

The user explicitly resumed the unsteady goal after the metric merge and made
performance essential, including profiling interpolation paths. New separate
branch codex/native-unsteady-performance is based on pushed2abbd11769; completed
branches and AdapNoExt are untouched. Worktree: SU2_NativeIntegrated.
Current evidence/case root: integration_evidence/native_unsteady_performance_v1.
Goal scope: static physical geometry, adaptive surface meshes, primal2Dtriangles,
WINDOW_AVERAGE Euler/RANS, dual-time orders1/2, conservative solution and history
transfer, rejection/restart/SU2/CGNS, MPI1/2/4. Cost must be measured at practical
cadence. Profile native transactions, metric work and interpolation/history;
use existing phase timing and Linuxperf rather than introducing a profiler.
The old paused goal's work is now authorized again; tool status may still show
paused because only the user/system can resume its control state.

Native now admits static 2D WINDOW_AVERAGE with BDF1/2.
The shared driver and existing conservative/barycentric history transfer are reused.
Immutable scope app: native_unsteady_build_v1/SU2_CFD, SHA2565973119ef9aa4120c40af25c2b09e1c4ebf0a696f0d558ed2dbc7214b69a1207.
Six actual vortex_v2 cases (orders1/2, MPI1/2/4) completed two adaptations each.
All independent original-connectivity P1 metric, q/length, current/history
integral and final positivity audits PASS; see independent_unsteady_audit.json
inside each case. Double output is the existing VOLUME_OUTPUT_PRECISION=DOUBLE.
The capcheck reader now reports that precision correctly. Original VTUs are donor
snapshots; rewritten restarts n/n-1 are paired with the next adapted mesh.
NativeSupport2D + DistributedTransfer tests pass at MPI1/2/4 in
native_unsteady_history_v2; initial v1 fixture omitted required TIME_STEP and
failed before native support checks. Failure evidence is preserved.

Performance is NOT met at freq3. MPI4 vortex totals: CFD.093s, metric.052s,
remesh.925s, replace.0198s (transfer.0101s). Native remesh does not scale on this
small case; CFD/metric do. Baseline real RAE MPI4 profile is terminalPASS,
176.610s elapsed, one accepted BL adaptation then CFD. Corrected CPU attribution
in profile_rans_bl_startup_np4/sampling_summary.json:34421samples, native21009,
metric3875, CFD9494, transfer9, setup34. Native13986samples have MPI frames
(66.6% of native), suggesting waiting/imbalance/collectives. Initial root-only
classifier was incomplete and is retained as sampling_summary_root_only_initial.
Do not confuse these CPU samples with wall time. Native transaction/metric work
is the priority; existing wall transfer timers resolve short projections.

RAE MPI1 comparison completed:247.781s versus176.610s MPI4 total; native
phase totals83.717s versus107.281s, with different distributed reconstruction
trajectories (serialjoint46/6.91s, MPI4joint97/64.12s). Native scaling is weak;
CFD benefits from MPI. This supports eventual separate remeshing communicator
work, not a current M-rank capability claim.

RANS time-domain coarse-to-BL SA/SST MPI4 cases plate_{sa,sst}_bdf2_np4_v1
completed two accepted adaptations. Independent geometric BL composition,
frozen sensor, current/history conservation and altitude audits PASS; maximum
height relative error4.45e-16. These are lifecycle controls, not wall-shear/drag
accuracy validation. Six SU2 window-boundary restarts reproduce uninterrupted
results exactly at MPI1/2/4 for both orders. CGNS output plus actual four-rank
CGNS BDF2 restart matches within8.89e-16. Mid-window average persistence and
pre-first-adaptation native-reference checkpoints are not validated.

Resolved-vortex physical-time.9 control: cadence100,300steps,dt.003,1225input
points ->752finalpoints, elapsed3.128s; fixedgrid2.111s. CFD1.388s, metric.103s,
remesh1.162s, replace.0223s (transfer.0123s), output.00476s. Cost narrowly below
CFD with little margin; densityP1-lumpedL2 3.840e-4 adapted vs3.364e-4 fixed,
Linf4.239e-3 vs4.477e-3. No accuracy/convergence certificate. Sparse output
omitted pre-adaptation donor VTUs; new build forces ordinary requested output
at native window ends under WRT_ADAP_MESH, reusing existing writer.

Bounded FIFO query-cache trial preserves original P1/actual geometric BL math
and authoritative unchanged-vertex samples; at most2048entries with conservative
per-entry admission estimate128bytes. Dynamic computed samples may be evicted;
seeded vertex samples never enter the FIFO. MPI field/engine/support/history/
CGNS/rejected-output suite passes42cases per rank at1/2/4 in
native_unsteady_cache_mpi_v2. Immutable trial app native_unsteady_build_v2
SHA2563888c9067df010618b4f045ecb03a0c2d8c3805a925ad3dcf442913e7ea9e587.
Completed case profile_rans_cache_np4_v2: nativecommits/operations/transport match
baseline; adaptedmesh and donor restart are byte-identical. Coordinated repair
64.118s ->26.759s; bulk split87.752s ->50.497s. Privatequeries132585546,
evaluations27775647, dynamicevictions12045836. Final CFD continued and exited0:138.878s versus176.610s baseline. Nativephase
sum107.281s ->69.547s (35.2% reduction); full adaptation108.716s ->70.9245s.
Final solution is byte-identical too; one matched end-to-end reduction21.4%. Validated implementation ready for the separate-branch checkpoint.
Post-cache cadence, all five SA order/MPI variants and BL-to-Euler cases
completed; independent target/history/height audits pass. Barycentric unsteady
case completes;4->2rank window restart differs1.35e-12. SA/SST saved current,
history and continued turbulence scalars are finite and nonnegative. Native
unsupported-window MPI2 exits1 with the intended diagnostic; three native/BL
adapter cases pass at MPI1/2/4. See NATIVE_UNSTEADY_PERFORMANCE.md.
Both actual RAE unsteady cases are terminalPASS with independent original-P1,
reference/geometry/height and positive current/history checks:
rae_rans_window200_np4_v1 (Euler mesh -> BL,12252/13386points) and
rae_euler_from_bl_window100_np4_v1 (accepted RANS BL -> Euler,7964/7233points).
Saved mesh_mach_windows.png previews, actual meshes, VTUs/restarts are in each
folder. Total adaptation-related cost / CFD26.28%RANS,41.09%Euler. RANS first
BL construction alone remains more expensive than its first CFD window. These
are developing-flow controls, not certified temporal/aerodynamic accuracy.
Initial strict whole-domain conservation audit failed because open curved
farfield resampling changes domain area. Default CLOSED sliver-policy correction
using independently measured near-constant farfield state passes unchanged1e-10
gate (RANS policyresidual<5.3e-13). Initial failure/diagnosis preserved; sparse
previous-history donor integral conservation is not independently established.

Shared full BDF2 restart writer now refreshes primitive diagnostics around its
history swap; compact restarts bypass it. Primary transfer/continuation were
already correct, but old previous-history pressure diagnostics disagreed by up to .56%. New Euler/SA/SST MPI4 V3 cases have consistentP/T/Mach/velocity below
4.5e-16, identical meshes and continuedprimary/turbulence fields toV2. The
baseline diagnostic regression failure is retained. V3 4->2rank full checkpoint
restart passes; combinedfinal MPI1/2/4 suite passes45cases/rank. Immutablefinal
app native_unsteady_build_v3, SHA256ac36d778c1286abe67eb2dc775ba0f513479ee3da3e4a112feeeded86460833a.

One heavy job, MPI<=4, threads1, builds-j2. Bounded 2D lifecycle/cost checkpoint
is complete; the larger native unsteady goal continues. Next push
is same-target MPI scaling and communication/imbalance cost, then converged
physical-time/cadence accuracy. Mid-window metric-average persistence and
pre-first-adaptation native-reference checkpoints remain gaps. Do not call
production affordability or 3D support complete. Immutable test-driver archive
deduplication reclaimed 2.035 GiB without removing case/evidence content; see
archive_deduplication_v1.json. See NATIVE_UNSTEADY_PERFORMANCE.md.

## Validated metric integration — 2026-10-07

Branch: `codex/native-metric-integration`, based on native main
`9450c0880e2b2c1c39dfc98bc2c9655844ab8731`, with incoming merge parent
`b14a14ea1dec771834d7d691fca27614605df39d`. The user authorized merging and
pushing to rois1995/SU2. AdapNoExt and codex/native-integrated are untouched.
At merge completion the unsteady goal was paused; it is now explicitly resumed.
The completed merge had no owned jobs; see the current section for live work.

Read METRIC_ROBUSTNESS_INTEGRATION.md for the reconciliation, independent
Hessian/WLS review, residual locations and performance limits. Original P1
donors store sensors only; geometric BL is evaluated at every actual, private
and remote query. Finer sensor requests and main's fade are retained. Noise is
zero; the hard-normal reset is excluded. Shared composition feeds thin-BL
integration and remesher queries; wall tensors are not spread into coarse donors.
The measured coarse BL floor is 6564.71, above the historical 4000/6000 sensor
budgets. Actual RANS explicitly requests composed budgets 10000/12000.

Final app: integration_evidence/native_metric_integration_build_v4/SU2_CFD.
SHA256: 498bb9fa3916635ee9bb9d05a54270ceff2525b30deb887cbc65dc98f7e9f69c.
There are 760 production source pins. Core_v3 has 47 cases per rank,
output_mpi_v3 has 44 and adapter_v4 has 3; all pass at MPI 1/2/4.
Serial Hessian_v3 passes 105 cases, and Hessian_mpi_v2 passes 23 per rank
at MPI 2/4; their Hessian production sources are unchanged. Reuse
build-integrated-v2 with -j2, one heavy job, at most four ranks and library
threads set to one. Never relink immutable archives. Final dry build has no work.

Final evidence root: integration_evidence/native_metric_integration_v4.
Run integration_evidence/check_native_metric_integration.py to recheck current
sources, binaries, MPI groups, actual artifacts and preserved incoming data.
completion_audit.json passes. post_frozen_v1.json is TERMINAL_PASS, nine stages;
exec sessions 55058 and 60405 are terminal with exit zero.
All twelve frozen coarse/fine WLS/QR runs pass with noise zero and unchanged
flow. Maximum MPI sensor relative difference is 1.42e-10, below 1e-9.
Independent Gaussian integration agrees within 0.0194% on the fine grid and
0.0030% on the coarse grid. Frozen nodal sensor fixed points pass, but composed
and quarter-edge P1 sensor transport residuals remain. Actual Euler cycle 0
and RANS cycle 1 hit the 80-sweep sensor cap. Continuous or composed gradation
is not certified. Mesh acceptance gates and sensor/geometry policy are intact.

Actual four-rank folders under integration_evidence/rae2822_transonic_v1:

- metricmerge_euler_cross_seed_v1: two accepted meshes, 5722/7649 points;
  minimum q 0.250666/0.342287, maximum length 1.708282/1.799994.
  Total 148.756 s; CFD 87.703 s, adaptation 42.675 s. Adapted density
  residuals are below -8.
- metricmerge_rans_cross_seed_v2: two accepted meshes, 12399/14234 points;
  minimum q 0.180096/0.265992, maximum length 1.799997/1.789548,
  relative height error at most 2.10e-9. Total 339.040 s; CFD 125.068 s,
  adaptation 196.112 s. The cost objective is not met. RANS has not reached
  configured convergence: density -7.911398/SA -7.336547 at iteration 2000.

Both cases pass independent mesh, flow, reference and height checks. Surfaces
adapt; both transfers have zero inadmissible states and recovery patches, and
CFD resumes after each. GRID_GUIDE.md links the actual meshes, solutions and
previews. RANS v1 was prepared only and was never run.

Incoming findings and v3 pass/rejected reports are unchanged. The 57 raw files
(224.02 MiB) in metric_branch_v3_reference have a SHA/size inventory. Failed
predictor-iterator, restart-filename, synthetic-CFD-fixture, missing-reference,
initial-quadrature and cancelled-campaign evidence remains local; compact
receipts are pinned in preserved_integration_attempts.json. Coarse sidecar
preparation retains original geometry/features verbatim and verifies bindings
for the known original mesh; the missing-reference guard remains enforced.

No 3D, unsteady, CAD, large-rank or converged-force claim is made. Future cost
work should target measured candidate/collective-round costs and repeated
gradation/root trials. Geometry-safe continuous field gradation is a separate
investigation. The later explicit user request above authorizes resuming this goal.

## Artifact cleanup — 2026-10-07

User requested removal of unnecessary/superseded test disk usage.
Completed in integration_evidence:864 generated executable/object files removed,
39.22GiB unique content reclaimed;12035 retained files verified unchanged by
size/mtime/inode. All meshes, solutions/restarts, failure evidence, plots, logs,
source snapshots and runners retained. Current build-integrated-v2 and appv28,
Euler baseline appv20, latest AD archivev14 and recent actual/MPI replay binaries
remain. Old AD build retains setup/compile commands/logs but compiler outputs
are removed, so it requires rebuilding. Older archived executables (including
superseded gzip versions) are removed; their historical test results and hashes
remain valid evidence, but those exact deleted executables are not directly
replayable. Affected manifests explicitly record retention status; original
manifests copied into cleanup_20261007/original_manifests.
Exact paths/counts:integration_evidence/cleanup_20261007/inventory.json;
readable report:integration_evidence/cleanup_20261007/README.md.
AdapNoExt and other agents' work/builds untouched. Keep only current/selected
baseline and required replay executables at future cleanup checkpoints.

### Follow-up cleanup — 2026-10-07

Removed7further superseded executable archives (appv28/v29 and older test drivers),
shared2byte-identical current test-driver copies by hard link, reclaiming4.17GiB.
Total across both cleanups:43.39GiB. Verified14521retained files unchanged.
Current appv30, current build-integrated-v2, comparison appv20 and latest AD
archivev14 remain; every mesh, solution/restart, rejected-grid diagnostic, plot,
log, source snapshot and runner is retained. Current test archive manifest paths
remain replayable and SHA-verified; never build/relink inside immutable archives.
Original manifests for deleted binaries are backed up. Exact paths/bytes:
integration_evidence/cleanup_20261007_followup/inventory.json; readable README.md
in the same folder. Keep only current/selected baseline executables going forward,
and share immutable identical archives instead of repeatedly copying them.

## Final common-source validation — 2026-10-07

Appv31 final campaign TERMINAL_PASS; exec session80005 closed exit0.
ParentPID680570/validationPID689650 are terminal; no owned CFD/test controller.
759production sources and all case/MPI artifacts rechecked by
integration_evidence/check_native_cross_grid_completion_v31.py; proof integration_evidence/
native_cross_grid_v31_case_audit.json. SolverSHA
449bca7c124990a12c0f95f3c4d87ce94e2d100e9148b0b4e5a7e6276a5dbe85,
testSHAeadbcd87fb51c059858153c295af9151e6c66b9bdbdcc3fe5a574ade85868e20.
Build evidence/source snapshots native_cross_grid_build_v31; -j1 build PASS.
Current source removes temporary phase scans/collectives/full replay capture and
unused corner/probe experiments. Normal rejected meshes/location reports remain;
appv30 archives retain removed experimental source. No target or gate relaxed.

Actual final cases under integration_evidence/rae2822_transonic_v1:
- nativefix_euler_rans_seed_v14:MPI4 exit0,187.383s, two accepted meshes;
  final6529points/12728triangles,qmin.182441861462,Lmax1.798842097984.
  One joint surface transaction,.0177985s. Solver106.515s/adapt80.251s.
- nativefix_rans_euler_seed_v23:MPI4 exit0,421.243s, two accepted meshes;
  final18591points/36263triangles,qmin.221503940968,Lmax1.799994857893,
  h0relativeerror3.77165e-12. Solver220.04566s/adapt200.7638s.
  First BL construction107.776s;29joint transactions51.7703s.
All four adapted meshes byte-identical to appv30/v13/v22 accepted controls.
Every saved-flow/geometry/reference/height inspection and independent P1/composite
metric audit PASS. Conservative transfer twice/case,0inadmissible states/0recovery
patches. CFD resumed; Euler adapted density residuals below-8, RANS final
density-7.956698575/SA-7.186480 atITER2000 (not configured convergence).

Final MPI1/2/4 groups, same compiled test binary:
- native_release_core_v1:42cases/rank, full shape/size/boundary rollback gates.
- native_release_output_mpi_v1:44cases/rank, configured CGNS/rejected output/
  default MMG/general MPI interpolation and conservative history transfer.
- native_release_adapter_captured_v1:3cases/rank, eight-cycle actual adapter/
  produced BL, captured two-point cross-owner publication and rejection.
Immutable identical current test archives share one inode; no build/relink there.
Main final runner run_native_release_validation_v1.py, state
native_release_validation_v1.json,9stages exit0. Original incorrectly broad
supplemental filter's serial-projection MPI abort and the cancelled-before-launch
corrected-v2 attempt remain recorded; neither is called a pass.

Current reports:RAE_CROSS_GRID_VALIDATION.md, RAE_CROSS_GRID_COMPLETION_AUDIT.md,
ROBUSTNESS_SCALING_RESULTS.md and GRID_GUIDE.md. RAE repair goal COMPLETE.
Source implementation committed as 73b4bf9434; final runnable evidence recheck
PASS after normalizing tuple/list JSON representation in the checker.
No owned CFD, build or test controller remains. Next authorized milestone:
static 2D native unsteady adaptation, reusing existing MPI history transfer and
time-window driver; implementation and lifecycle validation are still pending.
Static primal2D triangles/original polyline only, up to4ranks/36kaccepted cells;
no3D/unsteady/CAD/large-rank or grid-converged force claim. Cost remains high
(75%/91%of CFD time), and coarse-seed BL startup exceeds its initial CFD cost.

## Historical continuation: compound Euler wall repair (2026-10-07)

Production now has a bounded atomic unconstrained-height surface fallback:
up to8private boundary splits, complete free-apex movement and private flips,
q>=.18, maximum length nonworsening and size deficit nonincreasing (strictly
reduced for size-only repair). Positivity, original physical geometry, artificial
perimeter and participant certificates remain enforced; protected/prescribed
wall layers are excluded. Complete dependency rings are imported/reserved only
for the coordinated fallback. Ordinary Euler imports restored to appv28.

Appv29 SHA82df169f892480f0de334f71fd0072033795f801607b173494a6c8cd1eb1ba9a
was a control with broader ordinary Euler imports:43core cases PASS MPI1/2/4
(native_joint_surface_core_v1), but actual Euler v12 rejected adaptation2,
218.709s:2length cells,0shape,height,geometry failures. qmin.1880064082,
Lmax2.310971419, upper wall near(.6148,.0575). Accepted mesh/solution1 and
rejected2/VTU/failure CSV/location plots retained. Saved-flow inspection PASS0/1.
Do not call this complete. Broad ordinary imports have been reverted.

Reproducible production-kernel archive native_euler_joint_surface_probe_v3:
actual v11 shape patches (48/56cells) finish with4boundary splits,
qmin.182441861462,Lmax1.798842097984; actual v12 length patch (58cells)
finishes with3splits,qmin.249177269209,Lmax1.799775694864. Original frozen
sensor target retained. Standalone diagnostic original-P1 locator; no full
solver/MPI claim from these local probes.

Refined rebuild COMPLETE exit0, log/tmp/native_joint_surface_build_v2.log,-j1.
Appv30 archived, SHA3722ee354b386842b79c0bb409a7275ec413de221dbe70eed928fb55a3a0c723,
759production source hashes rechecked against/tmp/native_joint_surface_source_pin_v2.json.
Sequential validation campaign TERMINAL_PASS, exec session43537 closed exit0;
state integration_evidence/native_joint_surface_validation_v1.json,
runner run_joint_surface_validation_v1.py. Core MPI v2 PASS43cases on1/2/4ranks.
Actual Euler v13 COMPLETE exit0,315.106s,MPI4; independent saved-flow, geometry,
and original-P1 sensor metric audits PASS all cycles. Mesh1 byte-identical to
rejected-control v11:SHAe9d9a90a40e68d4be00113e6e6dd811e5536ba2758390edfb385a357052c0218.
One compound surface transaction adds4boundary points and repairs both old
shape failures in.0194721s; final6529points/12728triangles,qmin.182441861462,
Lmax1.798842097984,all native residuals0. Original AIRFOIL192edges->318->254,
FAR40->60->76; reference deviations<=9.38524e-7, feature retained.
Transfer.112503/.054428s,no inadmissible projected states. CFD resumed and
reached density-8.009458/-8.003233 on meshes1/2; no aerodynamic convergence claim.
Euler solve total142.0534s,adapt172.3432s (contended during run by a foreign
CPU0-pinned solver and compiler; not a clean scaling benchmark).
RANS v22 COMPLETE exit0,490.145s,MPI4. Both meshes and conservative transfers
accepted; saved-flow/geometry/first-height and complete composite target audits
PASS all cycles. Final18591points/36263triangles,qmin.221503940968,
Lmax1.799994857893,height error3.77165e-12; density-7.956698575/SA-7.186480
atITER2000, not configured convergence. Solve278.10677s/adapt211.5452s.
First adaptation accepted,29jointcommits53.1357s,
qmin.200002,Lmax1.79999; mesh1 byte-identical to previous accepted v21:
SHA93d79ecdfce03ce63347e154a8bbe1f563cc008afb3d0b9728b44caebe493213.
Do not restart the campaign or terminate foreign work. Main sequential campaign TERMINAL_PASS, exec session43537 closed exit0.
Adapter/manual CGNS/produced BL/captured two-point MPI group PASS np1/2/4,
native_joint_surface_adapter_captured_v1. Note its filter omitted the distinct
NativeCGNS2D production-loop tag; manual writer coverage is present.
Supplemental configured CGNS/default MMG/rejected-output/AdaptationMPI/
conservative projection+transfer group TERMINAL_FAIL, controller635079 exited,
label native_joint_surface_output_transfer_v1. np1PASS58cases/7218assertions;
np2aborted in a serial geometry projection helper with halo points, np4not run.
The broad filter included serial-only Conservative projection/transfer tests;
split these from the actual MPI tags before retrying. Do not mark the whole
configured CGNS/MMG/MPI group passed. Failure log and exact binary retained.
No owned CFD or validation controller is running.
Case proof:RAE_CROSS_GRID_VALIDATION.md and
integration_evidence/native_cross_grid_v30_case_audit.json.
Remaining: supplemental configured-output/conservative-transfer MPI controls,
review/selective cleanup and commit; no accepted-grid numerical convergence claim.
The main campaign completed core MPI1/2/4, Euler v13 lifecycle/inspection/
independent original-P1 audit, RANS v22 lifecycle/inspection/independent complete
composite audit, then adapter/manual-CGNS/captured two-point MPI1/2/4 checks.
CFD MPI4, timeout700s per case, all thread env1. Completed cases retain unchanged
prior input/config. Goal ACTIVE: supplemental MPI selection must be corrected,
then review/selective source cleanup and commit. Do not relabel the failed
supplemental attempt as passing or rebuild archived binaries in place.

