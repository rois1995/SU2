# Build247 outcome: cheaper queries, first event still refused

2026-11-07. ZERO accepted meshes. Euler50k two-event workflow, conservative
admissible transfer, continued adapted CFD and actual disk restart remain open;
then dedicated BL/constraint/MPI qualification and RANS500k. CAD remains optional.

R=`integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57`;
A=`R/m6_runs/euler_build247_canonical_mach_only_real_cfd50k`.

## Actual result and preserved state

Same saved real CFD seed,MACH only,L1/50k,byte-identical config to Mach-only244,
strict geometry1e-4,q>=.2,J>=.05,maxedge1.8,requested heights,native5400/outer14400.
Build247 retains canonical single-triangle query keys AND computation,with bounded
scratch/work and exact authority/support/tolerance/coordinate identity.

First event REFUSED native5347.699778/round34816:
1032602cells/1148q-bad/484J-bad/1044geometry-bad/0size-bad. ALL cells measured;
all cache/census/shape/geometry flags complete. Still first shape pass;no shape
closure or subsequent residual-geometry construction/reclosure established.
Exact refusal:phase ownership exceeds event deadline;prior ownership retained.
Final ownership decisions declined migration (predicted gain about205s,cost220s);
this does not turn positive residual counts into an accepted mesh.

All30owned/metric/reference/rejection files sealed;1032602paired row counts complete.
1520source/build/runtime/helper/config/input pins verified unchanged before ONE
pidfdSIGTERM only to verified MPI59208/start10786446. All ten recorded handles plus
wrapper58990absent;tool14272TERMINAL1/5546.863332s,pins unchanged.
A/{capture_ready,source_freeze_before_stop,owned_stop_once,status,terminal_handles}.json.
Never repeat the historical stop. No adapted transfer/continued CFD/restart claim.

## Comparison and next target

Baseline244terminal:1032169cells/2035q/967J/1046geometry/0size.
247has887FEWER q-bad,483FEWER J-bad and2FEWER geometry-bad cells.
Both stop at their unchanged time budgets;fixed-budget wall times are not a causal
speedup result. Quality-entry field counts were identical. Quality ages4658.117179s
versus4645.907007s;net q removal22.606988/s versus22.475482/s across the full pass.
Remaining-count improvement is material,but neither mesh closes the strict gates.
A/terminal_comparison244.json records exact work,service,churn and source hashes.

247commits85783versus83052. q retired635923/introduced530617;J559825/454105.
Last positive window removed493q/211J net over48.211s;the subsequent same-round
exit emits zero delta counters. These cannot be added or treated as a stalled window.
45shape entries have all features blocked;queue18275,blocked features655,
missing features0,tracked private collectives0. Terminal q-surface211/interior937;
J-surface121/interior363. Interior cells may still share constrained vertices.
Shape-bad union1525;all1525have an edge<.9,1441have an edge<.5. This is correlation,
not proof construction caused the residuals. No quality-aware construction authorized.

Immediate next target is SPEED:deadline refusal while useful q/J removal and
unblocked queued work remained. Boundary geometry queries/smoothing remain the
measured expensive path;their timers overlap. Repair power and geometry closure
remain independent unmet requirements. Do not infer closure from commits,debt,
memo hits,balanced ownership or a passing local patch.

One bounded diagnostic selects actual residual cavities within existing64cells,
covering interior,boundary,feature-adjacent,geometry-only and clean controls.
Use existing operators/query tracing to assess unchanged-context reuse,nearby-query
misses,valid early screening and certificate margins. Existing raw-collapse-plus-
optimizer gap remains a hypothesis;no competing implementation or larger-cavity
campaign. Choose ONE connected change from measured work/useful-patch progress,
validate affected behavior,then immediately full same-seed two-event M6 again.

## Inspectable terminal mesh and analysis correction

Export8143TERMINAL0/430.702597s,1555pins unchanged,all three owned handles absent.
Independent meshio reader92301TERMINAL0/3.587198s,reader handle absent:
201203points/1032602tetrahedra,complete finite SPD nodal tensors(min eigen.002704938038),
correct PointData/CellData lengths,exact SU2/VTU coordinates/connectivity and measured
bad counts confirmed. ALL2525bad cores/90430neighboring cavity cells retained;
reference and historical rejection records complete. A/inspection/rejected_mesh.vtu
and .su2 are diagnostic,not accepted. captured_nodal_metric is nine row-major
components in PointData;surface extraction retains this volume tensor. No ParaView
GUI qualification. Save meaningful terminal meshes,not every trial.

The existing analysis helper keyed service by(round,rank),but this run emits a
window AND exit at34816. The assertion failure is preserved. The unpinned helper
now keys by stage too and includes both deltas only at exit. Actual exit churn
invariants pass;legacy baseline records remain unchanged. No production/pinned
export source changed. A/terminal_analysis247_{duplicate_round,join_qualification}.json.
Refreshed terminal location/histograms are census61;older census40snapshots must not
be described as terminal. Exit ladder zeros are only that zero-work exit delta.

[Current handoff](../HANDOFF_Codex.md),[canonical design](ADAPTATION_QUERY_CANONICALIZATION246_20261107.md),
[backlog](ADAPTATION_BACKLOG.md). Full Euler/BL/RANS goal and every strict gate remain.
