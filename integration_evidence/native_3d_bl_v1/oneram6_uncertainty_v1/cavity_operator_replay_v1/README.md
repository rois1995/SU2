# Exact original-star replay and bounded actual-context capture

Full Goal3 remains active/incomplete. No production source, field, profile,
acceptance gate or work/time ceiling changes in this experiment.

## Original-star result

The original five tetrahedra/7 nodes at edge25389--53318 are locally repairable
with the EXISTING native operators and actual geometric BL composition at every
query. All169 original donors whose boxes intersect the entire patch box are
retained, below the existing256 limit. Sensor coordinates and tensors match the
archived61330-node file exactly. The whole original patch box is beyond BL support.
No clipped-donor rescaling or adapted-node refit occurs.

Initial q=.3112495/J=.180850, edge length2.60882008168. Balanced insertion creates
10positive tetrahedra but fails J=.0401461<.05. The75%-metric position passes the
local Refine gate (q=.271699/J=.0878063). A represented midpoint also passes
(q=.224845/J=.0501260). Existing joint placement succeeds in THREE trials,
q=.273986/J=.0864703, reducing changed-edge squared excess.654190->.023844.
The trial phase takes.1202s,348point requests/211edge requests, zero invalid queries
or field failures. Launcher1.730s includes initialization/reference loading.

These are PRIVATE LOCAL PROGRESS edits, not final accepted meshes: fixed artificial
interface edges still reach3.16538>1.8. View cavity_2_localProgress.su2,
cavity_3_localProgress.su2 or joint_localProgress.su2; inspect the balanced
cavity_0_rejected.su2 for the sliver. The strict exact-on-edge SplitEdge rejects
rounded coordinates and InsertEdgeCavity correctly handles them. Boundary-only
ReconstructSplitPatch rejects this nonphysical interior edge as designed.

The exact original witness is NOT a captured late cavity. This result withdraws
any inference that its original connectivity establishes a missing volume operator,
a need for another BL relaxation, or the cause of the whole M6 timeout.

## Actual-context capture

`run.py --context` builds a small standalone diagnostic against the existing native
archives and reuses the existing frozen-volume driver/backend pattern. It runs
one four-rank exact selected-field M6 event without Hessians or CFD advancement.
The existing rank-local rejected_trial hook retains only16 recent old/private
contexts/rank. It performs no new collective, source substitution or mesh gather.
4MiB/rank diagnostic storage is explicitly charged to native admission; the
existing2GiB cap is unchanged. The observer sees proposal validation, before
external clearance/final dependency publication; a false candidate_geometry flag
means the private buffer is unverified and may contain an earlier proposal.

Frozen parameter.875, archive provenance22/fresh model epoch2 and
core.003215747252591252 remain unchanged. The archive300s event ceiling stays.
Flow/mesh snapshots verify backend input immutability. This diagnostic is not a
transfer/restart or complete turbulence-state atomicity test.

Capture is TERMINAL: backend atomically rejects at300.640738s, launcher308.962s,
zero CFD steps and input geometry/flow snapshots identical on all ranks.64exact
contexts retained from10403observations; after keeping the last observation per
feature,14distinct contexts remain. Three simultaneous commits,1765private later
edits and38800first-row publications do not establish acceptance or a speedup.

`diagnose_contexts.py` compares exact original connectivity/coordinates and exports diagnostic
cavities with separate original-ID maps. The mesh skin label is for viewing,
never a CFD boundary condition. The ring is bounded sampling, not a whole-event
causal bottleneck trace or a capture of the complete private mesh.

## Reproduction and provenance

From NativeIntegrated: `OPENBLAS_NUM_THREADS=1 OMP_NUM_THREADS=1 nice -n19 python3
integration_evidence/native_3d_bl_v1/oneram6_uncertainty_v1/cavity_operator_replay_v1/run.py`.
Add `--context` only for the bounded MPI event, then run `diagnose_contexts.py`. One owned
heavy job at a time. Compile-j1; libraries1; nice19; local affinity0 or0--3.
Receipts retain actual compile/link commands, source/input/build hashes, linked
archive hashes and helper binary hash. Executables/objects are not committed.
The serial-only config changes solely ADAP_NATIVE_RANKS4->1; actual-context config
is the unchanged archive. All case links close within this repository.

Preserved setup failures: relative __file__ while recording provenance; missing
explicit CConfig include during first compilation; archive ranks4 refused by
serial CConfig. These all stopped before any native reconstruction. No acceptance
or geometry rule was weakened to correct them. No broad regressions or CFD reruns
are needed for this standalone diagnostic; existing production qualifications stand.

## What the captured contexts establish

Rank0 has10recent volume-edge stars of5--10cells with evolved connectivity/new
points; none is the earlier original-only witness. Rank1 includes a58-cell wall
patch and a7-cell wall split failing q=.140049/J=.0235604. Rank2 ends with44cells
and no physical faces:24cells retain original connectivity/coordinates,7new
vertices; its full box has289candidate donors, beyond the single-field256cap.
Rank3 ends with55cells/15physical faces/14new vertices. These observations do not
prove any patch infeasible, or that the last reported obstruction dominates time.
The exact old/private data now permit local replay without another whole-wing run.

The candidate_geometry=false buffers are explicitly UNVERIFIED: their default
q=J=1/metric_failure=0 values are not quality measurements, and reason text can
come from an earlier attempt. Never diagnose feasibility or accept such a mesh
from that flag or text alone. All64old/private pairs remain available, including
repeated growth attempts; viewing grids are emitted only for14distinct features.

The callback adds observation work/collective failure participation; this is not a
paired performance comparison. Sampled four-rank RSS sum2.126GiB is not an aggregate
peak; GNU time reports739452KiB maximum single-process RSS. Available host memory
remained about37.7--38.2GiB in the retained samples. Maximum native requested
storage2090751228bytes remains below unchanged2GiB admission. Raw input links are
closed; retained actual-context data/viewing artifacts occupy about1.7MiB.

Next: replay the captured evolved volume stars with existing placement/connection
operators and SAME original field, then study the expanded wall/transition patches.
Preserve complete original support; a289-box-candidate patch requires an existing
streamed/owner-routed path or exact relevant-support filtering, never truncation or
a raised donor cap. Do not add another local fallback from the original-star result.
Full Goal3/M6 construction, candidate reassessment, transfer and viscous accuracy
remain unqualified.

`verify.py` checks existing receipts without native execution. Exact runtime
controller variants are archived because the current launcher adds external group
watchdogs and corrects its generic context scope label. The raw context receipt
retains its original-star generic scope text; context.cpp, command and this report
identify the actual M6 capture correctly. Two helper executables are preserved
locally as lossless gzip files, verified against their executed binary hashes;
compression saved606757379bytes. They are not added to Git.

An offline helper was initially called inspect.py and shadowed Python inspect
when NumPy loaded. It was renamed; no native evidence/run was affected. Earlier
setup failures and exact provenance are retained. No broad regression or new
production source change was warranted for this diagnostic checkpoint.
