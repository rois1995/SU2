# Codex handoff — native integration

Updated2026-10-06. Goal ACTIVE and unbudgeted. This is the current checklist;
earlier chronological handoff entries remain available in Git history.

Worktree: `/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated`.
Branch: `codex/native-integrated`.
Every progress update must identify the **active testcase directory**. Read
`current_run.working_directory` or the live supervisor's command; for audits,
identify the actual saved-mesh input directory and report destination. A prior
PID or old state-file label is not proof that a case remains live.

AdapNoExt must remain untouched: `feat_adap_noExt` at
88828474db587652ee1b0c09010de838ddbcfd06, only its pre-existing tracked
`.gitignore` change, SHA256
7ccfe4565155e5b9b4c2241148628c35317e27c452345dde976da2ef816c564c.
`integration_evidence/main_initial.json` pins the invariant. Do not write into
another worker's worktree. Leave `T externals/{codi,eigen,medi,mel,meson}` unstaged:
they are read-only dependency symlinks to main.

## Resource policy and actual supervisors

One own heavy job at a time, initially MPI<=4, OMP_NUM_THREADS=1,
OPENBLAS_NUM_THREADS=1, builds-j2. Defer to foreign builds/solver jobs; never kill
foreign PID918696 or other foreign jobs. Host processes are invisible to sandbox
ps: inspect host PID/PPID/comm with escalated execution, not full command lines.
Use fresh evidence labels and preserve failures; never restart a live handle or
extend an existing timeout merely because an observation expired.

The foreign build finished. All four original supervisors are now terminal:
robustness succeeded, goal-runtime stopped on cold_p2 adjoint divergence, and its
two followers stopped without launching work. Host process inspection confirmed
those handles are gone. Do not restart any original version.

| State / runner | Current result |
|---|---|
| robustness_chain_v8 / run_native_robustness_campaign_v8 |Terminal0; all48 opposing-wall audits PASS. Additional48 single-wall audits independently PASS in independent_single_v1 |
| goal_runtime_v7 / run_integrated_goal_runtime_v7 |Terminal1; warm1/2 and cold1 short lifecycles pass, cold2 diverges in the final adjoint solve |
| integration_followthrough_v1 |Terminal1; prerequisite failure, no capacity or4-rank jobs launched |
| native_production_chain_v1 |Terminal1; prerequisite failure, no production jobs launched |
| native_production_chain_v2 |Terminal1; np1 config parsing rejected PARAVIEW_BINARY, no solver iterations |
| native_production_chain_v3 / run_native_production_checks_v3 |Terminal0; all1/2/4 actual production runtimes/outputs and all9 independent target/reference audits PASS |
| integration_capacity_v2 / run_integration_capacity_v2 |Terminal0;2-rank16384 timeout,4-rank16384 complete/independently PASS,4-rank32768 timeout; larger sizes stopped |
| goal_remaining_v1 / run_integrated_goal_remaining |Terminal0; all four interruption controls1/2, exact native-adjoint rejection2, actual warm/cold4 lifecycles PASS |

The cold2 failure was replayed as an ordinary static-mesh two-rank adjoint using
its cycle2 mesh and actual saved flow. That fresh process, with no adaptation or
mesh replacement, diverged too. All131 printed residual/sensitivity rows agree
within1e-6 log10 (one printed last-place residual unit); sensitivities are exact
at printed precision. Flow/restart pairing is exact and density/internal energy
positive. This isolates the observed divergence from the in-process replacement
lifecycle; it does not prove converged sensitivities or every transfer property.
Pins: goal_cold_failure_classification_v1.json, diagnostic evidence and runnable
diagnose_goal_cold_failure.py. Preserve the failed goal runtime as a CFD limit;
Interruption/native rejection and corrected-tree4-rank lifecycle controls now PASS in goal_remaining_v1. Warm4 final adjoint residual grows (log10 RMS1.54155); cold4 final log10 RMS-2.68074, neither final primal/adjoint is converged. These are lifecycle proofs, not sensitivity accuracy.

The first independent production attempt used an invalid testcase output token.
Repository OUTPUT_TYPE maps PARAVIEW to binary XML; PARAVIEW_BINARY is absent.
Fresh cases retain the same physical request and use OUTPUT_FILES=(RESTART,
PARAVIEW). Native source is unchanged. Pins: native_production_config_failure_v1.
The v3 runner reuses the existing archived production application (no duplicate
large executable), rechecks all759 production sources, and validates restart,
ParaView, metric, mesh/reference pairing and nine frozen-P1 targets. Allowing this
independent primal gate after classifying the AD CFD failure does not make that
failed lifecycle a pass. The production v1 folder retains the parser failure;
actual new runs live in integrated_native_production_v2/np{1,2,4}.

All three recent chains are terminal0. **Current freeze: native_validation_closure_v1 (PID621669)**. It runs rebuilt smooth/step controls+full audits, exact support/restart negatives, phase/field NACA1/2/4+target/timing audits, partial larger/thinner targets and actual native+TWO_PASS rejects1/2/4. Do not mutate its tools/sources/executable while active. Verify current state; old PID listings are historical.

Earlier freeze (now discharged): do not rebuild production/test binaries or mutate
run_native_production_checks_v3.py or check_airfoil_phase_timing.py. Its loaded
hashes are pinned. Both prepared patches were applied after the three chains finished. Normal test rebuild passed (two objects+link,109.385s); binary839340d6f5f05c89072b1efa35831ebbf0e716f74b9a16e0313fbb701304832d. Production sources/binaries unchanged. Current smooth/step1/2/4 runtime checks PASS;96 smooth independent snapshots PASS. Phase and further closure are in progress. The remaining AD runner accepts fresh labels and selected cases for diagnosis
without repeating passed controls. It reuses archived AD inputs/application,
rechecks759 production sources and the checker, archives its actual runner and
signals only one own descendant. Unexpected failures still stop the chain;
classify before resuming remaining cases. After these gates, apply the
instrumentation/step patches and finish all required validation. Read current
JSON and host handles before acting; the listed PID is not a permanent proof.

## Reconciled source and supported scope

Pinned inputs combined: native57f0550db4, Stage G1e39b683ee plus its assessed
hashed dirty draft, and cached origin/fixCGNSOutput e41e7eabc7. Stage G draft SHA:
ff0a4c9588447bd89964ecae859fe44c465a102423b080619e4cc8018aaa2346.
B0Spike/b0-repair stays standalone research, excluded from production/default tests.
Full decisions/pins: BRANCH_RECONCILIATION.md, refs_initial.txt, worktrees_initial.txt,
stage_g_draft.json and stage_g_draft.patch.gz.

Later cached-ref changes were assessed: origin/fix_periodic_rotation c39428c1da
(three rotational Jacobian/multigrid/limiter commits) and origin/pr-images
29c71a64b3. These are separate scope: both native and MMG remeshers reject periodic
boundaries; Stage G residual capture rejects periodic markers. They were not
imported. Assessment pin: periodic_branch_assessment_v1.json. No network refresh
by this agent is claimed. Revalidate all heads/remotes and the Stage G draft at
final audit; older unchanged-ref assertions are superseded by this assessment.

One remesher factory serves direct/goal loops: native, MMG TWO_PASS, ordinary MMG.
MMG remains default. Native+TWO_PASS and native adjoint are rejected. Preserve
in-memory boundaries, mixed-element output ordering,17-digit SU2 coordinates,
CGNS SU2MarkerName descriptors and accepted mesh/solution/reference state.
Native remains experimental static single-zone primal-double2D triangles;
physical boundary sampling adapts on the immutable original marker polyline.
No native3D/CAD/mixed/moving/time-domain/adjoint or general scalability claim.

## Executed integrated proof

Fresh assertions-enabled primal MPI/MMG/CGNS build: build-integrated-v2.
Output7cases/rank1/2/4 PASS; native/primal49cases/rank1/2/4 PASS; native-memory and
default-MMG controls1/2/4 PASS; auxiliary58cases/rank1/2/4 PASS. Independent output
checks84PASS. Evidence: integrated_output_controls_v6, integrated_primal_controls_v6,
integrated_primal_memory_v6, integrated_mmg_default_v6; auxiliary v9.
The old3-cell/6-vertex mixed fixture aborting in empty CFD sparse-pattern setup
at4ranks is an inherited CFD limitation, not a mesher failure; its expanded
45-vertex/48-cell fixture passes. Earlier harness/build failures remain preserved.

AD production correction79ce4bc81d: SwapMesh clears only nonowned adjoint seeds
before recording. The taped primal halo exchange already carries the owned seed;
seeding a transferred halo copy duplicates that contribution. Transfer utility
halo copies remain intact. Original-v9 and tight-Jacobi-v12 failures are preserved;
zero-halo diagnostic v13 restored exact continuation. Permanent v14 controls pass
3cases/26assertions per rank1/2/4, exact zero continuation differences, ten
lifecycles, restart/RSS and counters past LIMITER_ITER. This unchanged-ownership
fixture does not prove partition-independent iteration histories or sensitivity
accuracy. Archived AD executables: ad_repair_archive_v14.
SU2_CFD_AD SHA41d4295ffbe6f3f56b13ae94d03c60661bd091c52bd1459771e223b97f6d3255;
test_driver_AD SHA299045eb1f866034b168b96ed7a70d1cb026afe053a58a14fa48fc79ad539adc.

executed_source_coverage_v1.json compares source coverage: since primalv6,
only corrected AD driver and opt-in scaling probe differ; v14 and rebuilt v8
cover them respectively. All759 Common/SU2_CFD production hashes from ADv14
were rechecked unchanged. This is source coverage, not a substitute for compiled
scope/runtime. Both prepared patches were applied after the three chains finished. Normal test rebuild passed (two objects+link,109.385s); binary839340d6f5f05c89072b1efa35831ebbf0e716f74b9a16e0313fbb701304832d. Production sources/binaries unchanged. Current smooth/step1/2/4 runtime checks PASS;96 smooth independent snapshots PASS. Phase and further closure are in progress.

## Measured robustness and scaling

Consult ROBUSTNESS_SCALING_PROTOCOL.md and ROBUSTNESS_SCALING_RESULTS.md for
exact datasets, commands, thresholds and distributions; this section is current.

- Engine pilot16/64/256/1024 triangles: all12 cases independently PASS.
- Low-cut2048/4096/8192 input cells: MPI1/2/4 independently PASS. Single-sample
  adapt seconds9.453/6.403/4.797,28.019/15.195/9.205,98.936/52.893/28.138.
- Serial16384 cells: timeout124 at241.410s under240s whole-job budget, no finished
  mesh/JSON; cost observation, not demonstrated infeasibility.
- All27 largest-common8192 repetitions (three layouts×three ranks×three repeats)
  and final independent structure/full constant-target audit PASS. Evidence:
  robustness_campaign_v8/repeated/independent_audit.json. Median adapt1/2/4:
  cyclic99.516/95.533/52.513s; horizontal106.234/52.416/53.982s;
  vertical97.598/51.900/40.365s. Vertical4 ranges28.060–50.656s despite identical
  mesh/work hashes. Do not call the fastest sample typical or infer universal
  speedup from rank-dependent work on a shared workstation.
- partition_comparison_v1 has plot/CSV/provenance. partition_work_memory_summary_v1
  pins rechecked raw inputs, rank-max process VmHWM, encoded payload and final
  owned-cell imbalance. Horizontal4 imbalance1.26396 versus vertical4 1.06348;
  this association does not establish the plateau's cause. No admission-memory
  rejections in these27. Initial-directory exchange work above2MiB is outside
  transaction admission; encoded traffic includes self buckets.
- Matched AR10/100/1000 small straight-wall controls1/2/4: all9 complete and
  independently PASS. Geometry/h0/tensor scale together; not a curved1000-AR
  airfoil or large-demand proof. Fixed-geometry AR25 controls1/2/4: all3 valid
  but incomplete, necessary-condition incompatibility h0_metric2.5>edge cap1.8.
- All96 smooth eight-cycle snapshots (single/opposing walls, barycentric/conservative,1/2/4 ranks) now pass independent frozen-P1 structure/target/height and flat-reference audits. Single-wall48 evidence: integrated_primal_controls_v6/independent_single_v1; all144 input pins rechecked after evaluation. Reusable run_native_bl_audits.py covers both wall variants for future step runs.
- Actual sensor/BL NACA baseline h0=2e-4,4k/6k/3k: all three-cycle viscous/
  conservative-transfer/resumed-solve runtimes and all9 independent original-
  reference/frozen-P1 audits PASS. Evidence integrated_airfoil_baseline_v8 with
  independent_v8. Whole-job1/2/4 times316.525/180.644/107.176s include CFD,
  gathers/checks; not isolated remesh timing. Final triangles19502/19575/19487.
- Larger NACA4k/12k/3k serial: timeout124 at902.433s. Accepted23368 then40968
  triangles; timeout during subsequent coarsening from40968. Cycle0 exact
  donor/metric/candidate/original bytes match an executed baseline audit;
  cycle1 needs its own independent audit. No cycle2 candidate. Pin:
  airfoil_complexity_cost_limit_v1.json; not a construction infeasibility proof.
- Thinner NACA h0/HMIN1e-4 serial: timeout124 at902.055s. Accepted38852 triangles,
  timeout during next remesh from38852 (incoming8382shape/15685length residuals).
  Completed cycle0 needs independent audit at h0=.0001. No cycle1 candidate;
  5e-5 axis was not run under predeclared stop. Pin: airfoil_height_cost_limit_v1.json.

Whole-process memory is not bounded by native transaction caps:2MiB work,
256donors,128discovery regions,64candidate cache; original boundary/master reader
rows are O(B). Engine RSS includes startup/replicated manufactured input. World
counters include init+adapt and omit direct CPassiveComm/failure votes; adapt
seconds exclude init and audit gather. Strip ny=4, boundary grows with size.
Hardware8-core i7-9700/62GiB RAM, noSMT; shared host and installed OpenMPI defaults.
Selection/reference/collision/all-rank metadata are candidate bottlenecks, not
measured dominant causes. Do not change scheduling/data structures without proof.

## Grids, solutions and audit provenance

GRID_GUIDE.md links actual files. integrated_airfoil_gallery_v8 has fresh verified
initial+three4-rank SU2/VTU exports and wall-detail preview (exact coordinate/
connectivity round trips and audited input pins checked). Prior native-baseline
real_airfoil_gallery_v1 remains distinct; grid_gallery_v1 contains audited strip
exports. Original NACA input is case/airfoil_input.su2; configs reference its
identical QuickStart copy. Donor/adapted/metric/transfer summaries live in
runtime_npN and copied audit_npN. CASE_FILES.md explains exact names.

The derived NACA fixture did NOT save nodal flow/restarts: Run/Postprocess/Update
bypassed Output and used the solution in memory. Metric/transfer CSVs are not
flow snapshots. The fresh production cases integrated_native_production_v2/np{1,2,4} contain
local input.su2/run.cfg and request actual restart+ParaView conserved/primitive/
metric fields plus accepted SU2 meshes/reference sidecars. Read v3 state and the
case evidence for actual output availability. Serial cycle0/1 mesh+flow outputs
now exist; production_available_flow_v1.json pins exact restart/grid pairing and
positive conserved-state checks (5233/11811 points). This availability proof is
not completion of the full runtime/target audit matrix. v1 retains the invalid-token parser
failure. run_template.cfg/prepared.json pin the corrected profile; restart-to-mesh
mapping self-check passes, All1/2/4 production runs now pass four-cycle mesh/flow/metric output checks.
Whole-job times430.517/155.435/99.552s include all driver phases, not isolated
remeshing or a reliable speedup distribution. Final triangles19502/19575/19487.
All12 actual VTU scalar-field/coordinate sets exactly equal their paired double
restart rounded to Float32; wrong-pair and NaN self-checks reject. Vector fields
are outside this reader/check. Pins: production_scalar_pairing_v1.json and
native_production_runtime_summary_v1.json. All nine frozen-P1/reference target audits now PASS, and their input pins were
rechecked. Every audited CSV tensor exactly matches its actual double restart
component-wise. Consolidated completion pin: native_production_complete_v1.json.

Audit tools hash donor/metric/candidate before/after evaluation and original
NACA reference before/after its check. Input changes are rejected. The runtime
original-reference path executed in actual fresh NACA audits. Auditors archive
sources and runtime-evidence hashes. Runnable check:
`python3 integration_evidence/check_native_audit_inputs.py` passes actual saved
four-rank opposing cycle7 and rejects a deliberate temporary-copy edit; originals
unchanged. Earlier archived audits are preserved.

## Required remaining actions — goal is not complete

| Deliverable | Required closure |
|---|---|
| Reconciliation |Final revalidate main/heads/remotes/Stage G draft; classify later relevant changes |
| Integrated validation |Actual Stage G warm/cold/interruption/native-rejection pipeline and corrected4rank follow-through; actual native production/output coupling matrix |
| Robustness |Saved96 smooth one/opposing-wall audits PASS; partial larger/thinner NACA target audits; abrupt nodal step controls and every saved one/opposing-wall target audit; explicit support diagnostics |
| Scaling |Higher-rank capacities; actual MPI MAX remesh/replacement/transfer/CFD/output, local rank timings/RSS/ownership; classify every failed/stopped axis |
| Handoff/review |Commit tested source, exact evidence and final requirement-by-requirement report; do not substitute narrow checks for full requirements |

After production v3, capacity v2 and goal_remaining_v1 finish and failures are classified:

1. Independently audit completed timeout-run snapshots: complexity12000 cycle1
   at h0=.0002; height1e4 cycle0 at h0=.0001. Use audit_native_airfoil.py on each
   audit_np1, with its own original airfoil_input.su2 and fresh report labels.
   Do not demand whole-run verified status from these partial datasets. No
   missing timed-out candidate can be audited.
2. Verify source/patch hashes, apply ONLY pending_airfoil_phase_timing_v3.patch
   (v1/v2 superseded; retain them as historical evidence), plus
   pending_bl_step_metric_v1.patch. Both are now applied and the normal test rebuild passed. Smooth/step1/2/4
   runtime controls passed; finish the active closure audits and phase proof.
3. Run existing smooth NativeBL2D plus hidden NativeBLStep2D, fresh MPI1/2/4,
   serial diagnosis first. Step length .004 left/.014 right of moving center;
   same eight heights/centers and four wall/transfer combinations. This is
   abrupt nodal data with continuous frozen P1 interpolation, not inconsistent
   cell-discontinuous values. On incomplete, verify exact original reference,
   point count and accepted flow retention; preserve candidate/residual evidence.
   Use run_native_bl_audits.py for all96 accepted one/opposing-wall snapshots
   in each fresh smooth/step run; use audit_native_bl.audit for rejected candidates.
   Run smooth and step in SEPARATE evidence folders: snapshot names are identical.
4. Fresh existing NativeAirfoil2D baseline, label integrated_airfoil_phase_timing_v1,
   ranks1/2/4,900s/rank, save-audit, then full independent airfoil audits. v3 records
   solve(with inner output), metric, full backend import/adapt/export, replacement,
   transfer subset, per-rank replacement-minus-transfer and actual mesh writer.
   MAX values are separate and not additive. Local rows include cumulative Linux
   VmHWM and owned/total CFD points/local elements (overlaps included). Actual
   donor/transferred conserved solution CSVs match dense global IDs/coordinates
   of paired mesh snapshots and remain outside phase timers; not SU2 restarts.
   Execute check_airfoil_phase_timing.py against actual saved evidence with a
   fresh output timing_audit.json. Its synthetic self-check passed, not runtime.
5. Execute compiled hidden NativeUnsupportedDerivative and
   NativeMissingRestartReference separately at1/2/4, plus actual native+TWO_PASS
   config rejection1/2/4. Each needs a nonzero NON-timeout/non-signal exit and its
   exact diagnostic, never the positive Catch-pass checker or an arbitrary crash:
   - Native adaptation does not support continuous or discrete adjoint configurations.
   - Native solution restart requires the original geometry sidecar:
   - ADAP_BL_METHOD= TWO_PASS requires ADAP_REMESHER= MMG; native cavities build the BL with METRIC.
   Discrete-native AD rejection now passes at2 ranks in goal_remaining_v1; normal-build support/restart/TWO_PASS controls remain in the closure.
6. Final completion audit against NATIVE_INTEGRATION_GOAL.md and protocol,
   covering every gate/invariant/artifact with authoritative current evidence.
   Fix integration defects, report demonstrated construction/resource/CFD limits
   separately, and keep the goal active until all required work is proven.

Actual Stage G sensitivity-summary checks are lifecycle/finite-value checks;
old final history SENS_GEO may remain stale while summary totals are correct.
No converged-estimator/sensitivity-accuracy proof is implied. Old4-rank divergence
is not proof for the corrected tree; a fresh4-rank control remains required.

Build dependencies: SCOTCH `/home/rausa/Software/scotch`; ninja executable:
`/tmp/claude-1000/-media-rausa-4TB-SU2-Versions-SU2-AdapNoExt/bae33fc3-5b05-48ef-a3e8-4d222dffb2fe/scratchpad/ninjabin/ninja`.
Use PYTHONDONTWRITEBYTECODE=1; new builds/artifacts stay in this sibling worktree.


Latest capacity/AD closure: capacity_ad_followthrough_complete_v1.json pins both
terminal states.2-rank16384 whole-job timeout241.910s, no candidate metadata;
4-rank16384 complete,11718 output cells, engine98.625s versus whole176.847s,
independent constant-target audit PASS;4-rank32768 timeout241.040s. Whole-job
budget includes initialization/gather/output, so timed-out phase is not established.
2-rank32768/65536 and4-rank65536 were not run under stopping rules. Failed serial
NACA demand axes receive no higher-rank controls by the predeclared successful-
serial rule. Four one-descendant SIGTERM tests cleanly propagate interruption,
save checkpoints and stop before remeshing; exact native AD rejection precedes
primal solve; warm/cold4 complete three cycles and saved-mesh checks. No AD solver
convergence/sensitivity-accuracy claim. Current test source changes are not yet
committed while the measurement/closure campaign runs; preserve their build pins.

Partial audit CLI returns0 when computation finishes even if residuals remain:
review both resulting JSON contracts/reference checks explicitly at completion;
the closure supervisor's exit0 for those commands is not a target pass by itself.


Current closure progress: fresh smooth and abrupt nodal-step fixtures each PASS
MPI1/2/4 and all96 independent original-flat/frozen-P1/height audits (192 total).
All audited donor/metric/candidate pins rechecked. Exact continuous-adjoint and
missing-reference diagnostics pass1/2/4 with exit1 (no timeout/signal). Consolidated
pin: native_step_support_complete_v1.json. Active case at the last check:
integrated_airfoil_phase_timing_v1/runtime_np1; read its evidence current_run for
actual rank/cwd. Full phase/field matrix, nine target audits, partial larger/thinner
contracts and actual native+TWO_PASS rejection remain in the active closure.
No additional build/source/tool changes are allowed until that handle terminates.
