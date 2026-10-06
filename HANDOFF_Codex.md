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
Initial pilot passes through1024input cells; larger/repeated/real-field limits remain pending.

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
