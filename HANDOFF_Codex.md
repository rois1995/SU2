# Codex handoff — native integration

Updated 2026-10-06. Goal ACTIVE; see NATIVE_INTEGRATION_GOAL.md.
Worktree: /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated.
Branch: codex/native-integrated. Do not write into AdapNoExt or other workers' worktrees.
AdapNoExt remains feat_adap_noExt at88828474db587652ee1b0c09010de838ddbcfd06;
its original tracked .gitignore modification is preserved, SHA256
7ccfe4565155e5b9b4c2241148628c35317e27c452345dde976da2ef816c564c.

## Reconciliation

Pinned inputs combined: native57f0550db4, Stage G1e39b683ee plus assessed/hashed
dirty draft, and cached origin/fixCGNSOutput e41e7eabc7. B0Spike from b0-repair
c27a187a11 remains standalone research, excluded from production/default tests.
Other branches and historical/deformation refs are classified in
BRANCH_RECONCILIATION.md. No remote refresh is claimed. Original worktrees untouched.

One remesher factory serves direct/goal loops: native, MMG TWO_PASS or ordinary
MMG. MMG remains default. Native TWO_PASS and native adjoint are rejected.
Preserved in-memory boundaries, mixed element order, double precision and CGNS
SU2MarkerName descriptors. Native scope: static single-zone primal-double2D
triangles, immutable original marker polyline with adapted boundary sampling.
No native3D/CAD/mixed/moving/time-domain/adjoint scalability claim.

## Verified integrated results

Fresh primal MPI/MMG/CGNS build completed in integration_evidence/build-integrated-v2
(-j2, debugoptimized with assertions, tests enabled). Corrected MPI output
harness passes all7 selected cases at1/2/4; full selected matrix passes49cases
at1/2/4 (11453 assertions at1; rank-dependent11610–12620 at2/4).
NativeMemory and default-MMG controls pass1/2/4; auxiliary58cases also pass1/2/4. Exact logs/evidence:
integrated_output_controls_v6, integrated_primal_controls_v6,
integrated_primal_memory_v6, integrated_mmg_default_v6.
Independent output audits and initial scaling pilot completed successfully.
Initial cyclic pilot passes through1024input cells. Fresh low-cut2048-cell group
passes1/2/4 with independent audits: adapt9.453/6.403/4.797s, single samples.
Low-cut4096-cell group also independently passes1/2/4:28.019/15.195/9.205s.
Low-cut8192-cell group independently passes1/2/4:98.936/52.893/28.138s.
The16384-cell serial case timed out (124) at241.41s; no finished mesh/JSON.
This is a cost-budget observation, not a construction failure. All27 repeated8192-cell
partition controls and their independent audit now pass; real-field demand limits remain pending.

Earlier failures preserved: missing preconfigure stamp (v1); old serial output
count assumptions (v2); test private-member/Catch compile error (v3); duplicated
replicated SU2 reader rows (v4); tiny three-cell mixed fixture's empty CFD sparse
pattern at4ranks (v5). Fixed test harness with owned point/global volume gather,
halo agreement, master's public file-reader boundary rows and MPI file barriers.
Expanded the same mixed triangle/quad/triangle motif to45vertices/48cells for4ranks.
The tiny-fixture empty-partition abort is a CFD limitation, not a mesher result.

## Live supervisors: check JSON and host PIDs before acting

- ad_repair_controls_v14.json / run_ad_repair_controls_v14.py, PID52130:
  fresh AD build passed. Original GoalSwap v9 passed1 but failed2 with exact
  owned transfer and large continuation errors. Tight Jacobi diagnostic v12
  reproduced it with unchanged ownership; clearing halo seeds restored exact
  continuation in v13. Final v14 PASS3 cases/26 assertions per rank at1/2/4 (all continuation differences0), and
  archives fresh executables plus hashes of all tracked production/test sources.
  Its terminal success permits the native robustness campaign to proceed.
- robustness_chain_v8.json / run_native_robustness_campaign_v8.py, PID52131:
  waits for final AD controls, rebuilds primal probe and CFD application, then
  size escalation (tiles128..4096) with low-cut vertical geometric ownership,
  three repetitions/rank for all3 layouts, matched AR10/100/1000, incompatible
  AR25, independent raw audits, actual NACA conservative/resumed-viscous baseline
  and demand axes, and saved opposing-wall audits from the fresh matrix.
- goal_runtime_v7.json / run_integrated_goal_runtime_v7.py, PID52132: waits
  for robustness success, actual warm/cold goal loops at1/2; SIGTERM to only one
  descendant rank during primal/adjoint recording at1/2; native-adjoint rejection.
  Short lifecycle checks use a hashed prior converged checkpoint on the identical
  input mesh; they are not estimator-accuracy or convergence-performance tests.

Primal v6 completed successfully, including84 independent output audits and
all12 initial cyclic scaling cases (16..1024 input triangles). Pilot independently
passes exact orientation/topology/area/reference/target. Auxiliary v5..v8 stopped
on inherited serial harness issues (BOX coloring, shared reference files and
rank-local zero wall denominator); repaired tests now pass all58 at1/2/4 in v9.
Old idle robustness v5 and goal runtime v4 were explicitly superseded before
runtime work to add geometric layout3. Other old follower versions are terminal.
See ROBUSTNESS_SCALING_RESULTS.md for current measured results and limitations.

Only one heavy job at a time. Followers remain idle until prerequisite succeeds.
A failed prerequisite stops downstream work; terminal state is not proof of pass.
Host jobs are invisible in sandbox ps: use escalated host process inspection.
Never kill foreign PID918696 (ancient test) or other foreign jobs.
Use fresh evidence labels; never overwrite/restart an existing state.
The full goal still needs actual AD warm/cold/interruption checks, larger/partition
controls as warranted by pilot outcomes, classification of failed stress cases,
and a quantitative results report. Do not mark complete after a pilot.

## Reproduction and limits

Read ROBUSTNESS_SCALING_PROTOCOL.md. Hidden [NativeScaling2D] controls:
SU2_NATIVE_SCALING_TILES1..4096; LAYOUT1cyclic/2horizontal-contiguous/3vertical-geometric; AR1..1000;
MATCHED1(default affine-matched geometry/h0/tensor) or2(fixed geometry/h0).
Timing/traffic excludes audit gather. VmHWM includes startup and replicated
synthetic input. Constant-probe metric_height>1.8 proves wall-altitude/edge
incompatibility. Raw CSVs independently audited with exact binary64 orientation,
topology, area, perimeter/reference and frozen target. Incomplete is measured,
never an installation success or a relaxed target. Native default caps:
2MiB transaction work,256 donor cells,128 discovery regions,64 cached candidates;
whole-process memory is not bounded by the transaction budget. Replicated
original boundary and reader master boundary rows are O(B).

Dependencies externals/{codi,eigen,medi,mel,meson} are read-only symlinks to main
sources: leave T changes unstaged. Local preconfigure stamp already exists.
Ninja: prior scratchpad/ninjabin. Static MMG SCOTCH root:
/home/rausa/Software/scotch. Threads1; MPI<=4; builds-j2. New builds/evidence
here, not /tmp or AdapNoExt. Disk free approximately42GB on4TB,7.4GB on/tmp.

Initial refs/worktrees/main invariants and Stage G draft gzip/SHA are pinned in
integration_evidence. Source manifestsv1/v2/v3 plusv6/v9 overrides and each runtime's
archived executable/source hashes describe tested inputs. Prior completed native
baseline57f0550db4 evidence remains read-only in
AdapNoExt/BL_NATIVE_INTEGRATION_WORK and is not proof of this integration.

Evidence-harness update: future driver checks use fresh runtime_np1/2/4
working directories and copy each job's native artifacts into audit_npN.
This prevents a failed later cycle/rank from inheriting older snapshots.
Existing successful v6/v9 evidence remains unchanged; new runs archive
the changed runner and record their working directory explicitly.

AD executable snapshots and hashes are preserved in ad_build_archive_v9.
Idle robustness v6 / goal v5 were superseded before runtime to distinguish
240-second time budgets from unexpected failures and parse host comm names
with spaces safely. Only SIGTERM to their verified idle supervisor PIDs was used.

MPI adjoint correction in SwapMesh: transferred halo copies remain useful in the
transfer utility, but the discrete-adjoint driver clears their seed values before
recording (owned warm-start values retained). Recorded primal halo communication
already carries the owner contribution; seeding a copy duplicates it. This is a
production mesh-swap defect, separate from the earlier test-harness defects.
Unmodified-v9 and tight-linear-v12 failures are preserved. Downstream v7/v6
followers stopped automatically on that failed prerequisite; v8/v7 are fresh.

Remaining follow-through after scheduled campaigns: the runner diagnoses higher
complexity and thinner h0 on1 rank only. For each serial demand case that passes,
run2/4-rank controls with fresh labels/configs and independent audits, as required
by protocol step7. If actual goal lifecycle1/2 passes, consider a fresh4-rank
warm/cold control to distinguish any surviving solver limit from the old evidence;
the halo fix may change post-swap behavior. Do not assume old numerical limits
prove failure of the corrected integrated tree.

Live tail: integration_followthrough_v1.json / run_integration_followthrough_v1.py,
PID119961 waits for goal_runtime_v7 success. It measures larger vertical strips
separately at2/4 (up to65536 input cells; same240s/120s stop rules), controls each
successful serial airfoil stress at2/4, and rechecks actual warm/cold goal at4.
Never launch another competing follower: this tail starts only after the earlier
runtime is terminal. Unexpected failures stop for diagnosis. Source preparation
includes additional binary/checker SHA guards; because PID119961 was already
loaded while those guards were appended, externally verify these invariants
before it reaches airfoil/goal work (or supersede only while idle with a fresh label).

User grid inspection: see GRID_GUIDE.md. Independently verified8192-cell
1/2/4-rank and small64-cell4-rank probes have SU2/VTU exports plus provenance
in integration_evidence/grid_gallery_v1. Exact VTU coordinates and SU2 validity
were checked. A physical-coordinate crop preview is included. Saved integrated
BL/CGNS controls and the clearly marked prior-baseline NACA result are linked.


Interim partition audit: all9 cyclic8192-cell repetitions independently PASS
(structure and complete target), immutable copied datasets in
integration_evidence/partition_layout1_audit_v2; snapshot aggregation in
partition_progress_summary_v1.json. Medians/ranges in results. v1 audit had
zero cases after a Python3.8 method mismatch; v2 is the successful audit.
Horizontal controls are running; final27-case audit remains scheduled.
Observed foreign periodic2-rank AD run overlapped a later serial repetition;
shared-workstation timing caveat applies, never touch foreign jobs.

Completion measurement gap: ReplaceMesh logs are master-local, not rank MAX;
per-phase native maxima do not give exact total remesh wall MAX. After the
binary-pinned campaigns finish, use minimal test-only instrumentation and a
fresh targeted run to record actual remesh, replacement/repartition, transfer,
CFD and artifact timing separately. Do not modify/relink a live campaign binary.
This is required by goal/protocol, not a speculative optimization.


Prepared measurement closure: pending_airfoil_phase_timing_v1.patch and its
SHA/source manifest in integration_evidence. git apply --check passes; source
and binaries are unchanged, and this patch is NOT compiled or tested yet.
After robustness_chain_v8, goal_runtime_v7 and integration_followthrough_v1
are all terminal (diagnose failures before advancing), verify the input source
hash and apply it. Rebuild only UnitTests/test_driver with-j2 after host quiet.
Run the existing NativeAirfoil2D baseline with the existing pinned baseline.cfg,
fresh label integrated_airfoil_phase_timing_v1, ranks1/2/4 sequential,900s/rank,
save-audit; then independent airfoil audits and validate all9 timing CSV rows.
Do not use old binaries with newly modified source provenance.

Patch reuses protected lastReplaceTime/lastTransferTime and the real production
adapted-mesh writer. It records separate local phases reduced by MPI MAX:
solve (includes inner-iteration output), metric, complete backend Remesh,
ReplaceMesh, transfer, replacement minus transfer, and adapted-mesh output.
The subtraction is performed per rank before MAX; it is not MAX(replace)-MAX(transfer).
Per-phase maxima must not be added and called a synchronized overall wall time.
Test snapshot gathers, saved-mesh checks and external audits remain outside
remesh/transfer/output timing. Failed candidates record remesh timing with
accepted=false and zero replacement/output. Targets and acceptance are unchanged.

followthrough_external_guards_v1.json proves current build and archived engine
executables still have the campaign SHA. Checker SHA is pinned; compare it with
the eventual goal_runtime_v7 checker_sha256 before the4-rank lifecycle. No source
checker or binary changes are allowed until the existing follower finishes.
The prepared patch deliberately waits, so the already-loaded follower's omitted
extra guards cannot be invalidated by this new work.


Completion coverage gap: no executed native fixture has an abrupt spatial nodal
metric request (the eight-cycle BL control uses a smooth Gaussian). Prepared
pending_bl_step_metric_v1.patch/json reuse the same fixture with a moving nodal
step (.004/.014 tangential length), all4 wall/transfer combinations and unchanged
h0 sequence. Original NativeBL2D remains a wrapper calling smooth mode; hidden
NativeBLStep2D calls step mode. The shared helper now checks exact retained
reference/point count/accepted flow on incomplete. git apply --check passes,
but this patch is NOT applied/compiled/run. Apply together with the timing patch
only after all live pinned campaigns terminate and failures are classified.
After rebuilding, run NativeBL2D and NativeBLStep2D with fresh labels at1/2/4;
serial diagnosis first, preserve any unexpected failure before proceeding.
Use run_native_opposing_audits.py on successful step matrices and
independent audit_native_bl.audit for single-wall/rejected snapshots as needed.
Do not call the step request cell-discontinuous: actual retained target is P1.

Reconciliation revalidation: all other heads/remotes still match refs_initial;
no new committed source to reconcile. The Stage G dirty production diff still
matches recorded ff0a4c9588447bd89964ecae859fe44c465a102423b080619e4cc8018aaa2346.
This excludes tags/stash from the head comparison and does not refresh network refs.


## Interim completion audit (goal remains active)

| Goal deliverable | Current proof | Work still required |
|---|---|---|
| Branch/worktree reconciliation | Pinned ledger; other heads/remotes unchanged; Stage G draft SHA unchanged; main HEAD/gitignore invariant retained | Revalidate at final audit; classify any later relevant source before importing |
| Fresh integrated build and focused MPI gates | Primal output/native/memory/MMG/auxiliary1/2/4 pass;84 independent output audits; AD corrected mesh-swap1/2/4 pass | Actual warm/cold/interruption/native-adjoint rejection campaign is waiting |
| Robustness envelope | Eight replacements, changing smooth tangential/height controls, one/opposing walls, affine transfer, boundary/corner/contact/admission tests executed | Abrupt nodal step control; affine AR escalation/incompatible request; fresh real NACA demand axes and all saved opposing-wall audits |
| Practical scaling limits | Exact independent pilot and2048/4096/8192 controls; serial16384 timeout classified; cyclic9 repeats audited | 27 repetitions and final independent audit now pass; higher-rank size limits; real remesh/replacement/transfer phase MAX measurements; quantitative final classification |
| Reviewable branch/handoff/evidence | Committed source/docs, binary/source/config pins, preserved failed evidence, GRID_GUIDE and audited exports | Final requirement-by-requirement audit and a report whose claims match completed current-tree evidence |

Current source coverage checked in integration_evidence/executed_source_coverage_v1.json.
Against the executed primal v6 source manifest, only CDiscAdjSinglezoneDriver.cpp
and the opt-in scaling probe changed. The former is covered by fresh corrected
AD v14 gates; the latter was rebuilt and is executing in robustness v8. All759
Common/SU2_CFD production files recorded in the AD v14 manifest match current
sources. This is a source comparison, not a substitute for runtime/compiled-scope
proof or a claim that pending prepared test patches have passed.

Explicit support diagnostics to verify before final completion: hidden
NativeUnsupportedDerivative and NativeMissingRestartReference tests are compiled
but omitted by the successful49-case positive matrix. Execute each in fresh
1/2/4 processes after current campaigns finish; expected outcome is a nonzero,
non-timeout exit with its exact collective diagnostic. Also exercise actual
native+TWO_PASS config rejection (ADAP_BL_METHOD=TWO_PASS in the native NACA
input) before any primal cycle. Stage G runner already schedules discrete-adjoint
native rejection. Preserve separate expected-failure logs, commands and hashes;
never run these through a positive-Catch-pass checker or count an arbitrary
crash as successful rejection.


Horizontal repetition group completed: partition_layout2_audit_v1 independently
passes all9 cases (complete and structural); copied raw hashes match originals.
Points/cells/faces CSVs are identical across the3 repetitions within each rank.
Horizontal8192 medians1/2/4 =106.234/52.416/53.982s; no four-rank benefit for
that layout. Tables/caveats in results. Snapshot partition_progress_summary_v2.
Thus18/27 repetitions independently audited so far; vertical low-cut repeats
are now running. Main final27-case independent audit remains scheduled and
must pass before campaign advances to AR/incompatibility/actual airfoil work.


Audit provenance tightened before queued actual-airfoil/opposing audits start:
audit_native_bl.audit now hashes donor, frozen metric and candidate before/after
numeric evaluation, reports exact input hashes and rejects a mid-audit edit.
All callers were checked; numeric contract and source/test binaries are unchanged.
audit_native_airfoil also pins the immutable original input mesh and checks that
all inputs stay unchanged during its reference pass. Actual-airfoil/opposing
runner summaries now hash the runtime evidence JSON they validated. They archive
the auditor sources they actually execute; prior archived audits remain intact.

Runnable check: python3 integration_evidence/check_native_audit_inputs.py.
It passes against the saved four-rank opposing-wall conservative cycle7 and
rejects a trailing-newline edit applied only to a temporary donor copy during
actual audit evaluation. Originals are untouched. Numeric/input-hash evidence:
integration_evidence/audit_input_hash_check_v1.json. Fresh queued NACA audits
will exercise the new original-reference hash path; do not claim that path's
runtime verification from a syntax check alone.


Latest update: all27 repeated8192-cell partition-layout runs are complete and
pass the final independent structure/frozen-target audit in
robustness_campaign_v8/repeated/independent_audit.json. Low-cut four-rank times
vary28.060/40.365/50.656s (min/median/max), despite identical mesh/work hashes;
do not promote the fastest sample as reliable scaling. Plot/CSV/provenance:
partition_comparison_v1. Results below supersede the earlier18/27 interim status.

Fresh integrated NACA baseline is running; runner52131, baseline child264049,
initial MPI child264128 at last observation. Other supervisors52132/119961 still
wait for prerequisites. No additional heavy job was launched for visualization.
Actual earlier native-baseline NACA gallery is ready in real_airfoil_gallery_v1:
initial10216 triangles, four-rank cycles23328/25549/19487 triangles, exact VTU/SU2
exports and wall-detail preview. All nine earlier1/2/4 saved meshes independently
verified; only short viscous/resumed-solve checks, not converged CFD accuracy.
See GRID_GUIDE.md. This is prior baseline evidence, not fresh integration proof.


Latest controls: affine AR10/100/1000 MPI1/2/4 all9 complete and independently
pass, with current raw hashes reverified. Fixed-geometry AR25 MPI1/2/4 all3
structurally pass but are correctly incomplete; metric first height2.5>edge
cap1.8 proves incompatible demand. These are small engine-only controls, not
large curved-BL capacity or CFD replacement evidence. Fresh integrated NACA
baseline1/2 passed316.525/180.644s;4 is live (MPI284403 at last host check).

Measurement patch **pending_airfoil_phase_timing_v2.patch/.json supersedes v1**:
adds rank-local timing, cumulative process VmHWM and CFD ownership/overlap counts,
with stream checks. Both original source SHA and git apply --check pass. Still
NOT APPLIED/COMPILED/RUN. Apply only v2 (not v1) after all current supervisors and
binary-pinned followers finish, together with pending_bl_step_metric_v1. Sources
and binaries remain unchanged. Minimal validator self-check passed:
python3 integration_evidence/check_airfoil_phase_timing.py --self-check
After actual fresh1/2/4 timing runtime and independent mesh audits, execute:
python3 integration_evidence/check_airfoil_phase_timing.py integration_evidence/integrated_airfoil_phase_timing_v1 --output integration_evidence/integrated_airfoil_phase_timing_v1/timing_audit.json
It must prove every cycle/rank row, exact reported MPI maxima, global owned-point
count versus saved adapted mesh, cumulative RSS and transfer/replacement scope.
No actual per-rank measurement is claimed from the self-check alone.


Fresh baseline runtime now passes1/2/4 (whole-job316.525/180.644/107.176s).
Independent_v8 audits are live; the first four saved meshes pass at the latest
report. The new original-reference hash path has executed on an actual NACA
mesh, and all four donor/metric/candidate/original pins were reverified for np1
cycle0. Full nine-mesh audit remains required before baseline geometry/target
claims extend to all ranks/cycles. Process observation: audit supervisor288694
and its real child confirmed live. AdapNoExt invariants rechecked unchanged.

All27 current raw hashes and engine memory/ownership counters revalidated.
partition_work_memory_summary_v1.json and the results table report rank-max RSS,
encoded payload and final owned-cell imbalance. Horizontal4 imbalance1.26396
versus vertical4 1.06348; association with a timing plateau does not isolate its
cause. Zero memory admission rejections in these27 small-memory probes.
