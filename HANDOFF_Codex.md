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
NativeMemory and default-MMG controls pass1/2/4. Exact logs/evidence:
integrated_output_controls_v6, integrated_primal_controls_v6,
integrated_primal_memory_v6, integrated_mmg_default_v6.
Independent output audits and scaling pilot currently follow sequentially.
No practical scaling envelope has yet been established.

Earlier failures preserved: missing preconfigure stamp (v1); old serial output
count assumptions (v2); test private-member/Catch compile error (v3); duplicated
replicated SU2 reader rows (v4); tiny three-cell mixed fixture's empty CFD sparse
pattern at4ranks (v5). Fixed test harness with owned point/global volume gather,
halo agreement, master's public file-reader boundary rows and MPI file barriers.
Expanded the same mixed triangle/quad/triangle motif to45vertices/48cells for4ranks.
The tiny-fixture empty-partition abort is a CFD limitation, not a mesher result.

## Live supervisors: check JSON and host PIDs before acting

- primal_chain_v6.json / run_primal_chain_v6.py, PID4057483: output/full matrix,
  memory/default-MMG, independent output audits, then12-job scaling pilot.
- extended_chain_v5.json / run_extended_chain_v5.py, PID4057485: waits for primal
  success, audits pilot, validates auxiliary sensors/BL/adjoint transfer, builds
  fresh AD at-j2, then GoalSwap1/2/4.
- robustness_chain_v1.json / run_native_robustness_campaign_v1.py, PID4060658:
  waits for extended success;3 repetitions/layout/rank at largest common passing
  pilot size; matched AR10/100/1000; incompatible AR25; independent audits;
  actual NACA conservative transfer/resumed viscous baseline1/2/4 and demand axes;
  opposing8-cycle/two-transfer audits reuse the fresh matrix outputs.

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
SU2_NATIVE_SCALING_TILES1..256; LAYOUT1cyclic/2contiguous; AR1..1000;
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
here, not /tmp or AdapNoExt. Disk free approximately61GB on4TB,7.4GB on/tmp.

Initial refs/worktrees/main invariants and Stage G draft gzip/SHA are pinned in
integration_evidence. Source manifestsv1/v2/v3 plusv6 overrides and each runtime's
archived executable/source hashes describe tested inputs. Prior completed native
baseline57f0550db4 evidence remains read-only in
AdapNoExt/BL_NATIVE_INTEGRATION_WORK and is not proof of this integration.
