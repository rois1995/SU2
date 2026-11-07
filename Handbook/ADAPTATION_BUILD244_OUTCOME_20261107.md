# Full244 outcome — refused; user selects Mach-only next

Full corrected combined-Mach/pressure L1/50k attempt is TERMINAL. ZERO accepted
meshes. Actual geometry construction improvement did not complete the workflow.
Do not compare its final unknown-cache zero placeholders as measured defect counts.

## Preserved outcome

First quality pass exhausted at native5321.382052/round37761:
1017133cells,42q/18J/1717geometry/0size, all flags complete. All44shape-union
cells blocked,queue0,missing0.237reached44q/17J/1656geometry with46blocked cells
at native5291.79. Bulk net q/J throughput was similar; this is not a speedup result.

Late geometry construction, surface exit5330.408970, gives1018021cells and
1418geometry-bad cells (299fewer); only1016261shape cells measured then, so its
21q/10J placeholders are NOT whole-field shape counts. Fresh quality entry5333.173759
measures all1018021cells:414q/472J/1418geometry/0size. This again demonstrates shape
failures introduced by geometry reconstruction and the need for coupled reclosure.

Final native exit5347.387821/round37843 has1018217cells and ZERO measured cells:
all cache/census/shape/geometry flags false. Reason: metric update exhausts elapsed-
time budget. Final q/J/geometry counts are UNKNOWN, not zero or accepted. Global work
6235194<19414976; elapsed time, not global proposal-work allowance, ended the event.
New placement path executes48trials,3moves,3validated candidates; final publication
of all3moves is not established by those counters alone.

All30capture files sealed;1018217owned/metric rows, complete topology/reference/
rejections, metric cache incomplete. All1520source/runtime/input pins verified
unchanged before one pidfdSIGTERM to exact owned launcher3440421/start8864247 after
explicit first refusal made two accepted events impossible. Tool82306terminal1,
5613.368135s,pins unchanged; all10sampled owned host handles absent. Never repeat this
stop. A244/capture_ready.json,source_freeze_before_stop.json,owned_stop_once.json,
terminal_handles.json and status.json are authoritative. Original CFD mesh/solution
retained; no transfer/continued adapted CFD/disk restart established.

## Meaningful saved mesh

Existing bounded exporter terminal0/367.604136s,1553pins unchanged/all handles absent.
A244/inspection/rejected_mesh.vtu and .su2:198598points/1018217tets. Captured nodal
metric is complete,9row-major components in PointData. Independent meshio readback
passes5.104009s: finite SPD tensors(min eigen.0025878931094898477), exact array lengths,
matching SU2/VTU coordinates/connectivity. Runtime q/J/geometry CellData remain NaN,
cache_valid0, explicitly UNKNOWN. No ParaView GUI or final acceptance claim. Empty
bad-cell/cavity subsets reflect unknown cache, NOT absence of defects. Reference and
historical rejection records are preserved. No routine per-trial mesh exports.

## Next target and explicit user steering

The data supports further investigation of repair power/coupling: shape reaches a
fully blocked residual before geometry construction, and late geometry work creates
hundreds of fresh shape failures. Actual243complete-context placement+existing
optimizer already gives4q/3J/6geometry→1q/1J/5geometry, but Engine offers placement in
non-repairing construction and optimizer separately in quality. A possible combined
private quality candidate is recorded in the backlog; it is NOT implemented and must
preserve existing preimport/reservation, identity/work budgets and full publication.

The user has now explicitly selected MACH as the sole adaptation sensor. This input
change takes priority for the next assessment/attempt, using existing implementation
244with the same saved real CFD seed,L1,50k and unchanged constraints/budgets. New
R/m6_inputs/euler_mach_only50k_v1/run.cfg changes ONLY ADAP_SENSOR; run_m6.euler_config
is updated and checked to generate it exactly. Historical combined-target inputs
and qualified CFD seed remain immutable. R/metric_target244_mach_only assesses actual
production Mach-only L1 metric without remeshing before the next complete two-event
M6. No competing quality implementation before the changed-target outcome. Old
combined-target comparisons are not same-target Mach-only benchmarks.

Completion remains TWO accepted Euler meshes, conservative admissible transfer,
finite continued CFD and actual isolated adapted disk restart, then dedicated
BL/MPI qualification and RANS500k. All remain open.
