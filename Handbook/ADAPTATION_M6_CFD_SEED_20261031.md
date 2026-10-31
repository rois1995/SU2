# Saved real Euler CFD seed — 2026-10-31

Latest user policy: reasonable CL/CD suffice; stop CFD and proceed to mesh
adaptation. This explicitly supersedes the assistant's earlier all-five-residual
four-order gate. Residual histories remain reported; do not call this a
residual-converged aerodynamic reference. All strict mesh/geometry/height/transfer
acceptance gates remain unchanged.

## Accepted reusable input

`staged57/m6_inputs/euler_original_force_seed_v1` is a dedicated independently
validated copy with read-only files. Original61330points/303359cells, Euler
Mach.8395/AOA3.06, same grid/native authority. Final selected v3checkpoint local1126,
global3004. CLmean.2695353537415,CDmean.0135358565364 over200iterations; ranges
1.114e-7/3.121e-8. Every restart coordinate matches original mesh point order;
all conserved fields finite,minrho.627665889,mininternalenergy103999.140494.
Restart SHA1eaee788bd62bd92a92ec539577dcbcfc0f8042dcbcec23adb04e825bb01a880.
Qualification SHAe5782d139bc9d398780f04e77800757c01012dabd05c2a3b51b3065000b40fc9.
qualification.json explicitly records residual_criterion_met=false and the new
user-authorized force policy. Its output hashes include the grid,authority,
canonical/numbered restart,config,original+continued history,terminal and actual
runtime provenance,and exact historical generator helper snapshot.

v3user_force_stop_receipt.json preserves explicit latest authorization and verified
owned launcher3014067 SIGTERM. All ten handles independently ABSENT and1405pins
unchanged before helper edits. Its original failed generator outcome(code1 from
owned stop) remains generator_outcome.json; no claim of natural converged exit.
Saved-input publication is independently checked after that terminal outcome.
`m6_seed_force_acceptance_checks.json`: force-based acceptance, unstable-force
rejection, history continuity, actual mesh/state audit and immutable hash recheck.
Original/adapted disk restart obligations are distinct: this is original-grid CFD.

run_m6.py REQUIRES the qualified seed directory. It rechecks saved artifact hashes,
coordinates and positive state, copies baseline_euler_seed.dat into a unique run,
sets RESTART_SOL=YES/SOLUTION_FILENAME=baseline_euler_seed, and keeps output
RESTART_FILENAME=restart_flow distinct. Every first remesh uses the same saved
field plus the same recorded deterministic ITER1 continuation; never recompute a
full initial solution from freestream. Later events use conservative transfer.

The new BEFORE baseline is euler_build179_real_cfd50k_before, TWO events, unchanged
179CFD/source,50kcomplexity/native5400/event/outer14400,MPI6,nice19,libs1. Actual
cycle0prints restart solution,CL.269535/CD.013536; startup verifies1401pins and
all81mapped libraries. All further comparisons use this input, not startup179.
After baseline terminal, ONE connected parallel-slot/debt-ownership/invalidation/
counter change, serial+oneMPI6 milestone, full two-event AFTER from identical seed.

## Neighboring saved solutions

Original /media/rausa/4TB/Verification/OneraM6 has only Meshes/NewMeshes; no saved
CFD configs/restarts/histories. Eight CGNS mesh files inspected metadata-only have
no FlowSolution nodes. Preserved oneram6_coarse_v1/flow_checkpoint is120iteration
unconverged RANS/SA/no-slip; not selected over the more evolved matching Euler
solution. Search receipt: v2nearby_solution_search.json. Leave user files intact.

## Historical preparation, not live state

The following is retained preparation context before the latest user steering.
The authoritative current live job is in HANDOFF_Codex.md, not these old handles.

**Original-grid M6 Euler CFD continuation RUNNING**, exec80366:
`staged57/m6_seed_runs/original_grid_cfd179_v3`.
Python3013761,time3014065,timeout3014066,mpirun3014067;
ranks3014072,3014075,3014078,3014081–3014083. Exact ten identities/startticks in
startup_verification.json, all verified live;1405pins unchanged,81actually mapped
libraries all covered. C++/binary/helpers/config/inputs FROZEN. Same179CFD and
original61330points/303359cells, nice19,MPI6,libs1,ADAP_LOOP=NO,COMPUTE_METRIC=NO.
Reads copied baseline_euler_resume_input.dat from validated v2checkpoint1877
(exact hash in resume_source.json and v2resume_checkpoint.json). FixedCFL5,no adaptation,linearFGMRES cap20/error1e-6,
MG0. Local3122update allowance plus original1878updates = total5000; outer3600s.
Qualification unchanged:rho log<=-6,all5residuals down>=4orders from ORIGINAL
freestream history,stable final200CL/CD,exact coordinate order,finite positive
rho/internal energy. No qualified seed or accepted mesh yet. Live history must
combine prior_history.csv + history.csv using command.resume_offset1878; using
only current history would incorrectly reset the residual reference.
At local436(global2314),rho log-5.65630;final200CL and CD BOTH stable, but
smallest of five residual reductions remains2.268orders, so not qualified. Earlier
local59linear7iterations/logresidual-6.6603,AVG_CFL5. Snapshot100→200state changes
are recorded in state_drift100_200.json:relativeL2~1.2e-5to5.9e-5,relativeLinf
~2.1e-4to1.0e-3. Stable integrated forces alone do not establish a stationary field. Latest hostload9.71/11.14/14.33,MemAvailable~31.6GiB.
One owned heavy job. Numbered restarts every100plus final; no earlier file overwrite.

v2TERMINAL after authorized convergence-stagnation stop. Five complete200-step
windows600–1599:mean rho log-5.60to-5.38(no improvement),CD ranges3.42e-5to5.99e-5
above~1.35e-5 allowance. Evidence:convergence_stability_assessment.json and
convergence_stop_receipt.json. Only verified launcher2953589 SIGTERM. All ten
identities ABSENT and1395pins unchanged BEFORE helper edit;terminal_handles.json.
Latest complete numberedcheckpoint1877independently coordinate/admissibility
validated(61330points,minrho.627664,mininternal103998.683),same original grid.
Saved qualification_history.csv,resume_checkpoint.json,and exact historical helper
source_snapshots/run_m6_seed.py. v2failure/exit1 is preserved; no convergence claim.
No restart from freestream. Continuation code fixes zero-based history joining so
the first resumed update is retained and the original residual reference survives.
m6_seed_continuation_checks.json supports config/history/provenance/state checks;
actual continued convergence remains pending. No production C++ changes/builds.

New run_m6.py REQUIRES a qualified seed directory, copies baseline_euler_seed.dat
and sets RESTART_SOL=YES with separate restart_flow output. Before/after first
remeshing uses the same deterministic ITER1 continuation from that saved seed.
Checks: m6_seed_input_checks.json; config/history gates and real61330point
coordinate/admissibility positive/negative checks PASS. These checks do not prove
convergence, seed resume, accepted adaptation or adapted disk restart. First seed
launch v1 terminated during manifest setup (str.resolve bug), before any CFD
launch; v2 corrected Path normalization and verified live manifest/runtime pins.
No C++ changes/builds before the requested unchanged-code saved-CFD baseline.
Latest user requests reuse of saved CFD near original mesh if available. Search
original /media/rausa/4TB/Verification/OneraM6 found only Meshes/NewMeshes, no
CFD configs/restarts/histories. Nearby CGNS metadata check is recorded in
179seed-v2/nearby_solution_search.json. Preserved local flow_checkpoint is
unconverged120iteration RANS/SA/no-slip, incompatible with current Euler slip-wall
seed; leave it intact for possible later RANS work. Continue current Euler solve
without discarding progress. Its qualified saved field is reused for all first
remeshes. Current convergence state is reported above; no qualified seed yet.


## Historical CFD plan (superseded by latest user force policy)

## Saved original-grid CFD solution — latest user requirement

After179terminal and before the next full two-event M6, solve the ORIGINAL M6
mesh at unchanged Euler physical conditions. Reuse the current CFD solver and
existing restart/config facilities, ADAP_LOOP=NO; one owned nice19 MPI6 heavy
job, single-threaded libraries and resource receipts. Select a bounded iteration
cap and qualify with residual reduction plus sustained CL/CD stability, finite
fields and positive density/internal energy. Initial bounded qualification plan:
5000 iterations/3600s outer allowance (not an adaptation time-budget change),
retain Mach.8395/AOA3.06, Euler/ROE/MUSCL/current limiter and original geometry.
Use existing CFL adaptation capped20 rather than fixedCFL1; preserve MGLEVEL0
initially. Independently require density log residual<=-6, all five Euler residuals
at least four decades below their initial finite levels (historical proposal;
latest user explicitly waives this as a seed prerequisite), and final200-iteration
CL/CD ranges<=max(0.001*abs(mean),1e-5 for CL/1e-6 for CD). Record history at every
iteration including forces. These are practical initial-solution criteria, not
experimental agreement or grid convergence. Do not publish a seed merely because
the cap or a solver convergence flag was reached. If the solver stops before a
full stability window, continue from its checkpoint. Preserve failed attempts. Record the precise criteria and
actual history; reaching an iteration cap alone is not convergence. Preserve numbered checkpoint files and history so a timeout cannot destroy the
last complete state. Resume only a coordinate/admissibility-validated checkpoint
with known iteration/history provenance, rather than restarting from freestream.

Actual CFD preparation outcome: adaptive-CFL/cap20 with five linear iterations
stalled across five200-step windows after600iterations; mean density residual
stopped decreasing and drag range stayed above its qualification threshold.
Preserved v2evidence, verified owned-only stop, all ten handles absent and1395pins
unchanged. Continue from complete checkpoint1877with fixedCFL5,linear cap20/error
1e-6 (same Euler physics/flux/limiter/grid), keeping every qualification criterion.
A corrected zero-based history join retains the first resumed update, preserves
the original residual reference and caps both segments at5000updates total.
Historical helper source is archived/hash-verified; legitimate helper/control
changes do not invalidate an independently preserved evolved solution. No fresh
freestream solve, no native budget increase, no adaptation C++ changes yet.

The read-only CFD follow-up does not establish a new failure of v3yet: force
stability improved, while residuals and local field changes still need its bounded
outcome. Do not switch schemes or lower qualification just from a different test
case's settings. If a terminal nonlinear-limiter issue is demonstrated, existing
LIMITER_ITER is a possible control to assess: the actual single-zone driver calls
CFluidIteration::Solve, which increments InnerIter each steady iteration; Euler
preprocessing stops updating the limiter after that threshold while the flux
continues consuming stored values. The separate CFluidDriver multizone reset path
is not used here. This is source semantics, not proof that freezing solves this case.
This control remains unselected pending v3outcome evidence; no extra CFD run now.

Restart source review: CFVMFlowSolverBase loads SOLUTION_FILENAME; output uses
RESTART_FILENAME. Steady binary input is basename.dat, whereas adaptation output
has numbered cycle names. Set READ_BINARY_RESTART=YES and use separate seed-input
and output basenames. The reader does not certify restart coordinate/order equality;
check every coordinate row against the exact original grid explicitly. Existing
ITER1 history and initial_cfd_receipt(flow_converged=false) cannot qualify this seed.

Save original mesh/native reference, restart, configuration, history, solver/runtime
and hashes in a dedicated immutable input directory. Check restart point count,
ordering/coordinates and state admissibility. Future attempts COPY the checkpoint
into isolated run directories, use RESTART_SOL=YES with verified filename mapping,
and distinct output names. Prove first remeshing consumes this saved field (or a
recorded deterministic restart continuation), not freestream. Subsequent events
continue from conservatively transferred solutions. No source/metric fixture or
case-name reconstruction dispatch. Do not overwrite the saved restart.

The new solution changes the first target metric. Latest user explicitly requests
BEFORE/AFTER runs from this new saved CFD input, superseding the previous restriction
on a new old-code baseline campaign. Run current179 unchanged CFD first, with the
new seed and identical target/gates/native5400/time/work/memory settings. Preserve
terminal evidence before implementing the connected change. Then run the changed
implementation with the same copied/pinned seed and runtime/config (except source/
binary provenance). Existing counters define common comparison; newly added
passive counters must not be pretended to exist in the before run. Future results
use this saved-input baseline. Compare matching quality time, actual q/J/geometry
counts, commits/s,q-bad removed/s, wait share and rank debt spread, with actual
initial/quality-entry workloads explicit. Build179startup data remains historical;
never mix its field with the new baseline for a same-input speedup claim.
