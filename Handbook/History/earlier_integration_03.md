## Active goal: repair both cross-grid RAE2822 adaptation paths

Activated at the user's request on 2026-10-06; goal ACTIVE, unbudgeted.
Work only on codex/native-integrated in SU2_NativeIntegrated; AdapNoExt untouched.
Current changes remain uncommitted. One heavy job at a time, MPI<=4, build-j2;
foreign test_driver PID918696 is no longer live (checked Oct7).

Requested contracts: RANS starts from user Euler seed3592points/6952triangles,
h0=1e-5,growth1.2,thickness.02; Euler starts from triangulated original RANS
seed13937points/27642triangles and has NO BL metric. Adaptive boundaries,
q>=.18,Simpson L<=1.8,original-reference hausd<=1e-6,wall-height relative
error<=1e-8 and admissible conservative solution transfer remain required.

Euler completed both adaptations on appv6,v10 and now COMMON appv20.
Latest casev10 exit0,129.241s,MPI4;10162/12288triangles,
qmin.269974/.206052,Lmax1.79918/1.79959,all residuals0.
Case v3 has independent saved-flow/topology/reference and original-connectivity
P1 metric audits PASS; qmin.244718610215/.223654157706,
Lmax1.799974214812/1.797694270119. Only final CFD density criterion met;
no aerodynamic grid-convergence claim. Later common-source replay still required.

RANS v21/appv28 COMPLETE exit0,415.109s,MPI4, two accepted adaptations
and conservative transfer/resumed CFD. First12615points/24833triangles,
qmin.20000165,Lmax1.79998593,29jointcommits52.2479s,transfer.0715395s.
Second18591points/36263triangles,qmin.221504,Lmax1.79999,transfer.170946s,
all four mesh residuals0; completion predicate stops after2complete sweeps.
Independent saved-flow/topology/reference/first-height inspection PASS0/1/2;
inspection.json,mesh_and_mach.png,surface_cp.png in casev21. Maximum relative
wall-height error3.77e-12. Independent complete sensor+geometric BL audits
PASS both adapted meshes: qmin.200001654519/.221503940968,
Lmax1.799985927001/1.799994857893, no shape/length failures at tolerance1e-8.
Reports independent_composite_metric_00001/00002.json; standalone independent
checker integration_evidence/audit_native_composite_rae.py.
CFD hit ITER2000: intermediate density-6.050995/SA-6.840865,
final density-7.956699/SA-7.186480; no configured-8 convergence claim.
Current binaryv28 SHA653f6697a38b743577a8d2bb291df71f760f997dfee7cf68eda8d9f77772fcf5.
Core42cases PASS np1/2/4 in native_joint_completion_core_v1 (gzip binary archive).
Actual captured two-point joint repair publication and participant rejection
PASS MPI1/2/4 in native_joint_captured_mpi_v1:42->46cells,
qmin.200002,Lmax5.4354 (local progress, not complete adaptation).
Broad NativeRemesher/NativeProducedBL2D adapter and CGNS checks PASS MPI1/2/4
in native_joint_adapter_cgns_v2; controller terminal exit0.
The v1 broad controller failed ENOSPC before its first MPI launch; no test result.

Common-source Euler v11/appv28 is terminal exit1,193.881s,MPI4.
First adaptation accepted5411points/10444triangles, saved-flow/reference
inspection PASS0/1. Second rejected2lower-wall shape cells,0length residuals:
qmin.0891642077,Lmax1.79993084. Centroids(.4918005,-.0498070) and
(.4754246,-.0514559). Adaptive surface, no BL metric. Preserved accepted
mesh/solution1 and mesh_adap_00002_rejected.su2/.vtu, failure CSV and
location plot/JSON. Early complete-sweep stop changes mesh1 versus appv20;
keep the early stop and repair the exposed shape-only wall operator gap.
Independent original-P1 metric audit for Euler v11 remains pending.

User reports insufficient shock adaptation2. Saved-field diagnosis completed:
casev21/SHOCK_ADAPTATION_ASSESSMENT.md and shock_detail.png, JSON measurements,
reproducible analyze_shock.py/provenance. Modest actual shock refinement:
median x span .021405c->.016764c in fixed x.50-.68,y.07-.18; upper region
.052028c->.021795c. Solution1 sensor asks h_x~.019379c at shock on y=.12;
solution2 asks .008138c, but no mesh3 was built. Local independent sensor-only
length audit474mesh2edges,max1.640028<1.8. Sensor allocation reproduces6000:
3144.041(52.4%) beyond1c from(.5,0),1532.116(25.54%) beyond10c. HMAX20 floor
only78.217. Investigate metric demand/recovery and outer allocation; do not
blame cavity reconstruction or BL alone, or claim noise without evidence.
Appv28 source provenance remains pinned for completed Euler/RANS evidence.
Next controlled target experiment: same donor flow/budget/BL contracts,
inspect outer Hessians, compare pressure-only/higher Lp; extra adaptation
would sample the sharper final target but isn't proof of robustness.

v20/appv27 first adaptation accepted, second terminaltimeout124 at302.712s
because unnecessary remaining sweeps continued after q/L completion.
No deadline extension happened. v28 all-contract early stop fixes that cost.
Independent complete composite RANS metric audit, broad adapter/CGNS checks,
and actual multi-point cross-owner regression are now complete.
Production Euler shape-only compound wall repair, common-app Euler/RANS
validation, source cleanup and commit remain.

Private Euler repair evidence: native_euler_joint_surface_probe_v1 contains
self-contained C++ probe, exact saved-flow original-P1 target inputs and
48/56cell rejected patches, source/binary hashes, logs and reproduction commands.
Four coordinated boundary splits with free-apex movement finish both patches
qmin.182441861462,Lmax1.798842097984. A private intermediate decreases shape
quality (.12060->.11270), so ordinary published single-operation guards block
it. This is NOT integrated production code or MPI/full-grid validation.
Next: bounded atomic compound surface proposal, complete apex-star dependency
closure for Euler, private reconnection/movement and final unchanged acceptance
contracts. Do not relax published shape/geometry/length gates.
Goal ACTIVE, not complete; no scaling claim beyond4ranks/36k triangles.
Prior actual RANS v19/appv26 rejected44 length cells before joint integration.
Earlier completed case v7(appv11),exit1,41.017s:
all shape,height,reference checks PASS;139length residuals,qmin.185078,
Lmax15.0981. Its _rejected.su2 and failure CSV are saved. Earlier v6(case)
had2shape+171length failures,12172points/23951triangles.

Root fixes implemented:
- Freeze sensor P1 separately; compose existing geometric BL metric at query
  points using retained original geometry, including remote imported cavities.
  Native CFD Metric_* output now describes SENSOR only. Direct manual-kernel
  target APIs and MMG nodal-BL behavior are retained.
- Guard unrepaired midpoint slivers, allow shape-driven insertion and progressive
  deletion of poor seed rows; preserve Euler apices in boundary edge splitting.
- Retain unchanged vertices' authoritative target samples to avoid shared-point
  ulp differences from rounded donor-edge projections. Exact output consistency
  check remains; MPI regression reproduces the one-ulp cache discrepancy.
- Allow flips which strictly shorten oversized diagonals while preserving the
  shape floor; prioritize length deficits. Reverse long-edge restoration fails.
- v12's strict old-vertex retention experiment worsened RANS (casev8:40shape,
  240length,qmin.0365067,Lmax204.609,34.441s); reverted to tested v11 insertion.
  v13 guards surface REMOVE/REDISTRIBUTE bulk shape and maximum edge length;
  the previous prescribed-layer exception let them regenerate coarse transitions
  during every sweep, even from resolved BL stars. Initial HEIGHT/SPLIT layer
  construction remains allowed. New multi-row BL coarsening regression added.
- BL work allowance uses geometric rows*original wall edges, not coarse nodal
  density weighting; first RANS estimate22312.5triangles versus historical2.8M.

Core v13 all31 cases PASS MPI1/2/4. Diagnostic appv14 RANS casev10
reproduces v9,exit1,47.025s:5shape+134length (union134),qmin.13583,
Lmax92.0367,height/reference PASS. Trace: bulk phase reaches165 length residuals
at sweep2,then stalls near134; largest edge unchanged from sweep1 onward.
Late surface phases create no new deficits. Further sweeps alone are not a fix.
v15 changes bulk size-insertion location to balance Simpson metric lengths
(same principle as existing surface split), instead of Euclidean midpoint.
New graded isotropic fixture tests the actual inserted point and both half-lengths.
Build v15 COMPLETE/pinned; core v15 all32 cases PASS MPI1/2/4.
App SHA00855d9706fdcae82866b7499044e7a8ddb824361f291880d44650595876b68b.
RANS casev11 COMPLETE,rejected,45.430s,MPI4:24176triangles,3shape+109length
(union109),qmin.121522,Lmax19.9974,height/reference PASS. Metric-balanced
insertion reduces the plateau but does not finish the mesh.
Next diagnosis: shape-only movement rejects legal shape/size tradeoffs.
Draft size-aware movement reduces unique oversized-spoke deficits, preserving
q>=.18 and existing per-edge caps; graded fixture reproduces the old blockage.
Folder integration_evidence/rae2822_transonic_v1/nativefix_rans_euler_seed_v11.
Core v16 all33 cases PASS MPI1/2/4. App SHA4b632941df10dbfa1733e06f7f185bcde1f91b9282637ad16794088e538ef940.
RANS casev12 COMPLETE,rejected,52.07s:24478triangles,4shape+66length
(union66),qmin.145823,Lmax10.1556,height/reference PASS. Movement helps,
but 12 residual cells touch first-layer apices; ordinary bulk movement rejects
those entire stars. Draft v17 permits only tangential apex motion, preserving
all incident wall altitudes and old per-edge caps; new coupled regression.
Other 54 residual cells still require bulk repair; no success claim.
Appv17 was NOT used for CFD: core first-rank run failed4cases/8assertions
because the new bulk height check called the reference policy with marker0.
Corrected both zero-marker lookups and per-protected-cell missing-wall guard.
Standalone existing NativeMesh2D tests now PASS,305assertions/18cases.
Build v18 COMPLETE. Core v18 all35 cases PASS MPI1/2/4, including direct
cross-owner tangential apex publication; app SHA096a2d8299fd3eb5567613d19406bf8384a47e9501733b603599dfede7770ea9.
Post-test archive copy hit ENOSPC on /media/rausa/4TB (not a test failure).
Partial corev18 test_driver removed; intact build binary still matches recorded
hash; corev18 archive restored/verified, with explicit archive_recovery record.
Recovered10.44GiB before restoring650MiB test binary. Losslessly gzip-compressed
THIS TASK's old executable
archives only, verifying decompressed SHA256 against existing manifests before
removing original copies. Each archive_compression.json explains restoration.
All grids/solutions/source snapshots/logs preserved; latest appv18 retained.
Compression COMPLETE. RANS casev13 COMPLETE/rejected on appv18,MPI4,43.127s.
Draft v19 adds size-deficit priority to movement selection (matching flip size
repair). Previously healthy oversized cells waited behind ordinary shape
smoothing in the bounded phase. Added functional MPI test: one ordered round
must repair a graded oversized star before smoothing a fitting star.
Core v19 all36 cases PASS MPI1/2/4. App SHAaa5347a97b38b39f758d99082b248089fa61a56121a44283a98c09ba23cdc96b.
Casev14 rejected41.335s,23982triangles,2shape+61length,qmin.154751,Lmax19.8959.
v20 EXPERIMENT changes native pointwise BL target to SENSOR/BL intersection
without the legacy tangential coarsening floor. This STRENGTHENS sensor constraints,
keeps h0/growth/thickness and all acceptance thresholds; it is a target-definition
change, not yet validated. The old floor has a hard band switch hidden by its old
nodal interpolation; native point queries expose it. New flat-wall test exhibits
10,000x jump in default/legacy mode and verifies native-mode intersection has no
such switch, stays SPD, and keeps analytic normal size. MMG/default batch policy
unchanged. Buildv20 COMPLETE,pinned app SHA16dbfa9fc35d3fc3f7300414c01b554bc1f0ff9a4f03dfc49cd2aedf0f38e168.
All44 wider core/legacy cases PASS serial (6811assertions); one legacy-only
fixture fails MPI2 because it assumes33 LOCAL floor points and master stdout
on every rank. Reproduced on prior appv19 test_driver in
native_legacy_floor_np2_baseline; baseline failure is not a new regression.
The MPI-compatible native suite (37cases) PASS MPI1/2/4, with np2/np4 saved
in native_cross_grid_core_v20_mpi. Identical immutable test archives hardlinked
between v20 scopes to recover650MiB; original paths/hashes remain valid.
RANS casev15 COMPLETE/rejected42.896s:24482triangles,shape/height/reference PASS,
42length residuals,qmin.180033,Lmax8.12272. Native pure intersection removes the
remaining shape failure without coarse-seed complexity growth, but remesher
still does not finish. Preserve this target-definition change as EXPERIMENT
until common-source validations finish; no accepted RANS mesh/transfer yet.
Euler casev10 launched on appv20,MPI4,300s timeout,unchanged NO-BL config.
Next investigation: private insertion fans need local reconnection when one
new point cannot satisfy both shape floor and graded size target.
Synthetic probe native_private_flip_probe_v1 found oldq.200149,private fanq.140478,
existing flips repair q.181905 and remove requested oversized diagonal,
L288.913 -> max183.236 (perimeter remains oversized; not a complete mesh).
Archived BEFORE header/predicate/search source and hashes reproduce old rejection.
v21 draft reuses existing flips to reduce private shape deficit before insertion
publication; never restore size-requested edge. Shared RepairShape also replaces
surface's separate private flip loop, allowing tied deficits to improve locally.
New captured-fixture regression added. Standalone all19 NativeMesh2D cases
PASS324assertions. Added direct MPI/imported-composition version of the fixture:
private reconnection must publish across owners, preserve perimeter and retain
new point without restoring requested oversized diagonal. Buildv21 COMPLETE;
common39case suite PASS MPI1/2/4; RANSv16 rejected44length failures.
Next: bounded actual-cavity snapshot/replay, rather than another aggregate-only trial.
Temporary per-phase trace still present; consolidate/remove after diagnosis.
Goal ACTIVE; Euler latest completed casev10 passes on appv20.
Final density residual -7.991470261 reached ITER2000 cap, short of -8;
accepted adaptation/transfer does not establish CFD or aerodynamic grid convergence.

After both paths pass on common sources: inspect actual flows/grids, independent
metric/reference audits, broader NativeRemesher/BL/rejected/CGNS MPI checks,
update reports and commit selected code/evidence. Do not claim goal complete yet.


## Authorized next goal and long-term direction

User instruction on 2026-10-06: after completing the active cross-grid repair,
autonomously select and pursue the next feasible goal. Final target: full native
2D/3D anisotropic UNSTEADY adaptation, MPI-aware and optimized so adaptation does
not cost more than CFD work. Do not replace or prematurely complete the active
repair goal. No new goal object yet (one active goal already exists).

Next feasible goal selected: make validated repeated native 2D adaptation
cost-effective. Profile accepted RAE Euler/RANS workloads, measure adaptation vs
CFD time between triggers with explicit counts/ranks/hardware, fix dominant
remesher/MPI costs and retain quality/geometry/conservative-transfer gates.
Set measurable workload-specific acceptance criteria; do not claim a universal
cost ratio from one run. Use these results to support native 2D unsteady lifecycle
and conservative state/history transfer, followed by genuine 3D cavity/geometry/BL
operators and unsteady integration. 3D and unsteady native support are still absent;
current code supports primal static 2D triangles. Preserve evidence and handoff.

## Current follow-up: RAE2822 transonic Euler/RANS and rejected diagnostics

User mesh integration_evidence/rae2822_transonic_v1/mesh_RAE2822_euler.su2,
SHA940d8aed5d9ee0d6dc0a9f5b43a3973e7c76ed5d9123188c6e81d048cf6b6d26.
Exact no-BL seed3592points/6952triangles shared by both fresh cases.
euler_no_bl_seed_v1 completed on4ranks,exit0,28.372s,two accepted adaptations;
6952 ->8667 ->12788triangles. All3density criteria(log10RMS<=-8), topology,
mesh/restart/VTU scalar pairing, admissible flow, original-polyline checks PASS.
See its inspection.json,RESULTS.md,mesh_and_mach.png,surface_cp.png.
Force changes are substantial; NOT grid-converged aerodynamic validation.

rans_no_bl_seed_v1 explicitly stopped by user during native remeshing: terminal
exit1,851.186s,user_stop.json distinguishes cancellation from native rejection.
Initial coarse RANS solve met density+SA residual criteria atiteration847.
BL h0=1e-5,growth1.2,T=.02 made nodal complexity824836.8507; sensor target4000.
ONE TE node148 at(1,.00017),isotropic M=1e10I,dualarea7.365066e-5,contributes
736506.639 (89.29%). Frozen centroid complexity1220091.959; about2.82million
unit triangles estimated. Native work performed hundreds of thousands of
transaction rounds, not an idle solve. Root investigation: coarse-mesh nodal/P1
representation spreads extreme sharp-corner and wall metrics over donor areas.
Do NOT restart this same thin-BL campaign or silently weaken target/quality.

Rejected diagnostics IMPLEMENTED and VALIDATED. CNativeRemesher exports incomplete
native candidates as _rejected.su2 and _rejected_failures.csv when WRT_ADAP_MESH
isYES; accepted-state publication stays behind the existing COMPLETE guard.
Diagnostic SU2 output is independent of accepted mesh format (CGNS remains supported).
NativeRejectedOutput1/2/4 PASS, independently checked topology and exact failure
coordinate pairing; accepted geometry/flow/reference unchanged. Existing eight-
cycle NativeRemesher accepted-state control passes4ranks,both transfer methods.
Archived diagnostic app and759source byte hashes: native_rejected_build_v1/evidence.json.
Only two production source files changed: CNativeRemesher.cpp/.hpp.

Actual old Euler failure reproduced at euler_rejected_replay_v1,exit1,96.454s;
input/config/history/initial restart/VTU all BYTE-IDENTICAL to euler_native_v2.
Candidate5318points/10256triangles,36bad cells independently confirmed against
frozen donor P1 metric.34upperTE nearx=.99623-.99664,y=.000894-.000982;
2lower surface near(.66673,-.02656),(.69068,-.02313).qmin1.88613895e-10,Lmax1.79947.
Exactpositive/manifold/reference checks pass; separate near-duplicatepoint gate
FAILS(two pairs at~1e-11chord),minimum area1.471e-27. Do not hide this limitation.
SU2 and ParaView rejected files plus CSV/JSON/PNG locations are in that case.
See its RESULTS.md. Specific transaction provenance was not recorded.

BL_COMPLEXITY_DIAGNOSIS.md and bl_complexity_diagnosis_v1.json localize excessive
RANS demand. Reproducible inspector and flat-wall self-check demonstrate P1
representation inflation22.88x on a coarse normal interval. No BL metric remedy
has been implemented. Next implementation choice: composite geometric BL oracle
with frozen CFD sensor,or explicit staged BL construction; preserve finalh0.
Do not rerun stopped RANS unchanged,lower its target silently,or accept bad grids.
No own CFD/build/test jobs remain. Root defect and next work are documented;
this follow-up is NOT a new activated goal.

Old euler_native parser failure and rans_sa_native NOTRUN remain preserved.
Work only on codex/native-integrated,AdapNoExt untouched. One own heavy job,
MPI<=4,OMP/OPENBLAS1; check host handles before any launch. Prior goal complete.

## Completed campaign and next actions

All requested integration/robustness/scaling validation is complete within the
explicit native2D scope. Its runs are terminal; follow-up status is above. Completion proof is in
INTEGRATION_COMPLETION_AUDIT.md and integration_evidence/integration_completion_v1.json.
Do not restart any old supervisor or repeat passed campaigns without a new
change, failure or unresolved question.

Test-only BL-step/NACA-phase changes and the closure helper return fix are
committed as ffb3fb4c46. Current test_driver SHA256:
839340d6f5f05c89072b1efa35831ebbf0e716f74b9a16e0313fbb701304832d.
Normal rebuild native_phase_step_build_v1: two objects+link,exit0,109.385s.
All759 production source files remain unchanged since executed ADv14; final
coverage is executed_source_coverage_v2.json.

The final closure runner PID621669 is gone. Its state is terminal1 because a
Python helper omitted return row after the actual one-rank TWO_PASS rejection
passed. native_closure_failure_classification_v1.json records the bookkeeping
error; remaining2/4 negatives passed independently in native_twopass_rejection_v2.
The original failure and all completed steps remain preserved. All9NACA targets,
phase/field/ownership checks and both partial accepted-mesh contracts PASS in
native_phase_partial_complete_v1.json. General [AdaptationMPI]40cases/rank also
PASS1/2/4 in integrated_general_transfer_v1. No required runtime gates remain.

Future work should start from the measured limits, not from a broad scalability
claim: profile native reference/collision/transaction costs on representative
meshes; improve proven bottlenecks while rerunning the same frozen targets and
accepted-state controls. Native3D/CAD and high-demand thin-BL MPI capacity remain
unproved and require separately scoped work. Do not infer support from this goal.

Every progress update must name the actual **testcase directory**, not just the
repository. For audits name saved-mesh input and report destination. Current
JSON labels and historical PIDs are insufficient without host process checks.

## Checkout and resource invariants

AdapNoExt remains `feat_adap_noExt` at
88828474db587652ee1b0c09010de838ddbcfd06. Its only pre-existing tracked change
is `.gitignore`; FILE SHA256 (not git-diff SHA):
7ccfe4565155e5b9b4c2241148628c35317e27c452345dde976da2ef816c564c.
Initial pin: integration_evidence/main_initial.json. Other workers' worktrees
must remain untouched. Never stage `T externals/{codi,eigen,medi,mel,meson}`;
these are dependency symlinks, not integration edits.

One own heavy job at a time; MPI<=4, build-j2, OMP_NUM_THREADS=1,
OPENBLAS_NUM_THREADS=1. Defer to foreign solver/build jobs. Never kill foreign
PID918696. Host process inspection requires escalated execution; print only
PID/PPID/comm/time/CPU, not full arguments or environment. Do not restart a live
handle, extend an existing timeout, or discard a failed evidence label.

## Reconciliation and supported scope

Native57f0550db4 + Stage G1e39b683ee and its pinned draft + cached CGNS output
e41e7eabc7 are combined. Draft SHA:
ff0a4c9588447bd89964ecae859fe44c465a102423b080619e4cc8018aaa2346.
B0Spike remains separate research. BRANCH_RECONCILIATION.md records dispositions.
Later cached origin/fix_periodic_rotation c39428c1da and origin/pr-images
29c71a64b3 were assessed, not imported: both remeshers and Stage G residual
capture reject periodic boundaries. No network refresh by this agent is claimed.
The latest ref/Stage G/main checks match these assessed pins.

One driver factory selects native, MMG TWO_PASS, or ordinary MMG; MMG remains
default. Native is experimental static single-zone primal-double 2D triangles.
Boundary sampling adapts on the immutable original marker polyline. Native3D,
CAD, mixed volumes, moving/time-domain and native adjoint paths are unsupported.
Complete-status gates protect accepted mesh/flow/reference. Preserve in-memory
boundaries, mixed output order, SU2 17-digit coordinates and CGNS marker descriptors.

## Executed proof and observed limits

- Fresh assertions-enabled primal MPI/MMG/CGNS build; output7 cases/rank,
  native/primal49 cases/rank, native-memory/default-MMG controls and auxiliary58
  cases/rank pass1/2/4. Independent mesh/CGNS output audits84PASS.
- AD halo-seed correction79ce4bc81d: clear only nonowned seeds before recording;
  taped primal halo exchange already carries owned seeds. Primal transfer halo
  semantics stay intact. v14 controls pass3 cases/26 assertions per rank1/2/4,
  ten lifecycles, counters past LIMITER_ITER, exact continuation differences0.
  This unchanged-ownership fixture does not establish sensitivity accuracy.
- Actual main SU2_CFD native production1/2/4 and all9 actual-metric/reference
  audits PASS. All12 binary restart/grid pairs are exact with admissible flow;
  every plotted scalar and coordinate matches restart roundedFloat32. Vector
  fields are outside the scalar-reader check. native_production_complete_v1.json
  supersedes the earlier runtime-only pending flag.
- Fresh smooth and abrupt moving nodal-step BL1/2/4, single/opposing walls and
  barycentric/conservative transfer: all192 independent P1/height/reference
  snapshots PASS. Exact continuous-adjoint and missing-reference negatives
  pass1/2/4, MPIexit1 with exact diagnostics. native_step_support_complete_v1.json.
- Engine pilot12, low-cut larger9, repeated8192-cell27, matchedAR10/100/1000
  controls9 all independently PASS. Matched tests scale geometry/h/tensor;
  they are not curved high-demand AR1000 airfoil proof. Conflicting constant
  metric h0_metric2.5>lengthcap1.8 gives valid incomplete output on1/2/4.
- Repeated8192 medians1/2/4(s): cyclic99.516/95.533/52.513;
  horizontal106.234/52.416/53.982; vertical97.598/51.900/40.365.
  Vertical4 range28.060–50.656 despite identical work/mesh. Shared workstation;
  rank-dependent work prevents identical-work speedup claims.
- Strip capacity: serial16384 timeout241.410s, two-rank16384 timeout241.910s,
  four-rank16384 PASS (11718output,adapt98.625s,whole176.847s), four-rank32768
  timeout241.040s. Larger sizes NOTRUN under predeclared stops. Timeouts cover
  entire job; no timed-out phase, infeasibility or invalid topology established.
- NACA h0=.0002,4k/6k/3k passes1/2/4. Serial4k/12k/3k accepted23368 then40968
  triangles, timed out during next coarsening902.433s. Serialh0=.0001 accepted
  38852 triangles, next remesh timed out902.055s. Both partial target/reference audits now independently PASS.
  h0=5e-5 and higher-rank failed-demand controls NOTRUN under stopping policy.
- Actual AD warm1/2,cold1 and warm/cold4 short lifecycles pass. cold2 diverged;
  static replay on exact saved grid/flow reproduced131 printed trajectories
  within one residual print unit. Classified CFD limit, not successful lifecycle.
  Warm4 final adjoint grows(log10RMS1.54155); cold4 -2.68074, neither converged.
  All four own-descendant interruption checks(primal/recording,1/2) pass with
  collective cleanstop/no-remesh/checkpoints; exact native rejection2 passes.

Memory caps2MiB transactionwork/256donors/128discovery/64cache are not whole-
process bounds. Reference/master-reader boundary isO(B). Encoded traffic includes
self buckets; World counters omit direct CPassiveComm/failure votes. Engine
strip has fixedny4 andB grows withN; no general2D/3D scaling claim follows.

## Actual case files

Start with GRID_GUIDE.md. Actual native CFD input+flow/restart files:
`integration_evidence/integrated_native_production_v2/np{1,2,4}`.
input.su2 pairs with flow_adap_00000.vtu and solution_adap_00000.dat BEFORE first
adaptation. mesh_adap_0000N.su2 pairs with flow/restart of the same index.
Initial flow is computed from freestream in memory, not loaded from restart.
These short solves are not converged aerodynamic validation.

Immediate donor/transferred conserved fields from the new timing fixture:
`integrated_airfoil_phase_timing_v1/audit_np{1,2,4}/native_airfoil_cycle_N_{donor,adapted}_solution.csv`.
These use exact global IDs/coordinates; they are CSV fields, not SU2 restarts.
The older baseline_v8 fixture saved grids/metrics/transfer integrals but no
nodal solution. Actual production outputs address that earlier artifact gap.

Terminal history (preserved, not queued): robustness_chain_v8(0), native_production_chain_v3(0),
integration_capacity_v2(0), goal_remaining_v1(0). goal_runtime_v7(1) preserves
cold2 divergence; its original follow-through/production followers stopped
without jobs. native_production_chain_v2(1) preserves config-parser rejection of
PARAVIEW_BINARY; correct repository token is PARAVIEW. Do not restart old chains.

