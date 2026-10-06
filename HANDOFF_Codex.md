# Codex handoff — native integration

Updated 2026-10-06. Integration goal complete; RAE follow-up outcomes below. Branch `codex/native-integrated`;
worktree `/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated`.

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
