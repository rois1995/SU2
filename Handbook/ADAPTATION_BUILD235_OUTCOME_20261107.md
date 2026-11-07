# Build235 complete-attempt outcome

The goal remains TWO strictly accepted same-seed M6 Euler adaptations at50k metric
complexity, conservative admissible transfer, finite continued CFD and actual
adapted disk restart; then BL/constraint/MPI qualification and RANS500k. ZERO
adapted meshes accepted. All geometry/q/J/size/height gates and budgets unchanged.

R=`integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57`.
A235=`R/m6_runs/euler_build235_geometry_coupled_real_cfd50k`.

## Refusal and preserved state

First native event naturally refused at the time budget: engine5334.959465711s,
round36852. Final private working state:1,017,408tetrahedra,3,840q-bad,2,068J-bad,
1,686geometry-bad,0size-bad. ALL final cache/census/shape/geometry flags are complete.
This differs from227: these final counts and exported measurements are available,
not unknown. Native summary `final=0` means no accepted output, not an empty private
working state. Source/runtime/input1519pins remained unchanged.

All30owned/metric/reference/rejection files were sealed with SHA256, owned binary
row/byte counts and final-census joins. Only then was verified launcher2473948/
start6446871 sent SIGTERM ONCE via pidfd: one refused event makes two accepted events
impossible in the configured two-event attempt. NEVER repeat this stop. Wrapper
session38789 ended1 after5595.034596s; all11exact owned host handles absent.
A235/{capture_ready,owned_stop_once,terminal_handles,status,source_freeze_before_stop}.json.
No accepted transfer, continued adapted CFD or adapted restart occurred.
Sampled solver-tree RSS maximum16,616,550,400bytes; largest sampled individual
process2,856,984,576bytes. Shared pages may be counted repeatedly; neither number
is a unique physical-memory peak. No observed resource pressure required the stop.

## Actual inspection files and independent readback

A235/inspection contains:

- `rejected_mesh.vtu` and diagnostic `rejected_mesh.su2`:198,318points/1,017,408tets.
- `bad_cells.vtu`:all7,090q/J/geometry-bad cells, none omitted.
- `cavities.vtu`:all220,573one-node-neighbor cells of those cores, none omitted.
- `reference.vtu`, exact `reference_records.jsonl`, native point/cell ID maps and
  384bounded historical rejections. Historical reasons are not automatically the
  reason a final cell is bad; reference tessellation is visualization, not a certificate.

PointData `captured_nodal_metric` contains nine row-major symmetric components at
all198,318points. It is the current transported nodal tensor, not a reconstruction
of an immutable original continuous field or omitted query-time composition.
Surface extraction retains this same3Dfield; no separate surface tensor is invented.
CellData q/J/geometry excess, geometry-bad flags and metric-edge min/max are measured.
The bad/cavity views also carry all six metric-edge lengths and feature/height flags.

Unchanged qualified exporter ended0 after427.843911s, all input hashes unchanged;
all3export handles absent. Independent meshio readback passed all four VTUs, exact
array-length checks, finite/symmetric/positive-definite nodal tensors, exact native
bad-count reconciliation, unique IDs, and SU2/VTU coordinate/tetrahedron agreement.
Minimum tensor eigenvalue.00258789310949; maximum metric edge1.7999977267898075.
A235/inspection_reader.py and inspection_reader_status.json preserve the checker
and output hashes. No ParaView GUI qualification. Diagnostic SU2 uses generic
marker-name fallbacks and is not an accepted CFD restart artifact.

## Fair comparison and remaining blocker

Initial restart is byte-identical219/225/227; correctedL1/50k and saved real CFD
seed unchanged. Construction stage counts and checked operation/query/cache work
match227, but construction-entry wall time902.062s versus730.001s consumed more of
the same budget. Quality-entry max elapsed969.422253992s versus775.819045555s.
No isolated source-speed attribution follows from wall time.

Nearest COMPLETE227 census by QUALITY AGE is round36864:quality4342.589895436s,
4606q/2421J/1711geometry-bad.235EXIT quality4365.537211719s has3840q/2068J/1686geometry.
The age gap is22.947316283s, not zero.235has999more cells. These descriptive counts
do not establish a causal speedup. The nearest absolute-native-time227census is a
separate secondary comparison;227incomplete EXIT remains excluded.

Final66.020675920s interval removed861q/523J bad cells while geometry-bad increased3,
with1213commits and0construction edits. Boundary smoothing49.21summed-rank seconds
versus interior4.03; upper15474calls/1337refusals,22.63Mtraversal work. Timers overlap.
Exact union memo7040hits/8434misses: misses do not identify nearby-position reuse.
Final q surface/interior472/3368; J374/1694. Interior means no physical face and can
still touch constrained vertices. Shape-bad union5466;28stuck,queue63623,missing0.
16unique global drift samples remain supported; max sampled relative Frobenius
.0352871, not a whole-target norm. Primary-rechecked data/joins/prefix hashes are in
A235/terminal_quality235_assessment.json and quality_comparison227.json.

Next target: SPEED OF USEFUL SHAPE CLOSURE, analysis first. Net retirement continued
and the queue did not exhaust; boundary/geometry work is disproportionately costly.
Review existing unchanged-face/prepared-scope reuse and cheap screening; distinguish
exact repeats from unclassified misses before testing a rigorous movement/slack
bound. No saving from such bounds is established. Also inspect actual remaining
cavities to determine whether the stuck subset instead requires repair power.
Short-edge correlation does not prove construction caused the failures; keep
validity-first construction. Caller-support coverage questions stay hypotheses in
the backlog until actual cavity controls demonstrate them.

Shape must close before residual geometry/size closure and any shape reclosure,
within the same budgets and every strict gate. Geometry-only closure was NOT
exercised here; its rate cannot be inferred from geometry retirement during shape
repair. Return to full two-event M6 after one connected, demonstrated, qualified
change; do not launch a new baseline, scheduler, larger-cavity campaign or CAD-only
representation. Transfer/admissibility/continued-CFD/disk-restart obligations persist.
