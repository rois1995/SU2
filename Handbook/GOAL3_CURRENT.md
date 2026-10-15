# Goal 3 — current implementation and qualification

Repository `/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated`, branch
`codex/native-3d-bl`. Goal3 remains ACTIVE and incomplete. The full objective and
acceptance gates are preserved in [the requirement audit](../integration_evidence/native_3d_bl_v1/GOAL3_REQUIREMENTS.json).

## Current construction work

[Support selection](GOAL3_SUPPORT_SELECTION.md) describes the latest connected
change and its actual failing target. Complete globally classified support avoids
an unnecessary78-cell request: a26-cell free cover repairs the same interior
patch. The minimum-cost search is bounded by eight apices/511 states, with unchanged
64-cell admission and original geometry/height/shape/full composed-edge gates.
Point-only placement is attempted first where no wall-apex reconstruction is needed.
The [shared-apex wall refit](GOAL3_SHARED_APEX_REFIT.md) then advances the same1402-cell
strict target88 insertions to1820 cells/46 long edges at4096 work. Local native and
independent geometry/height plus bounded MPI2 protocol pass; full mesh still rejects.

The saved1110-cell grid passes independent whole geometry/height/shape checks but
has174 oversized edges. One bounded continuation reaches1402 cells/137 oversized
edges; the new costed-support/cap run reaches1411 cells/133 long edges and still
rejects at4096 work, after only two insertions. That is the preceding support-only
result; the newer shared-apex result is1820 cells/46 long edges as stated above. Do not equate partial metric progress or private local
success with a complete adapted mesh. See the support handbook for current runs.

## Existing qualified scope

- One actual-query geometric BL policy is shared by weights/refine/coarsen/thin
  complexity. Sensor-only original-P1 donors, finer demand, fade and retained
  geometry are preserved; returned nodal metrics contain only the sensor.
- Recorded stronger planar construction/removal independently passes MPI1/2/4:
  591 BL cells to252/250/243 sensor-only cells; identical BL geometry/connectivity,
  qmin .213044, Jmin .050377, Lmax1.798735, exact first altitude and contained feet.
  Maximum centroid tilt1.05409 is a declared limit, not an orthogonal layer stack.
  Read [the reconstruction handbook](GOAL3_RESOLUTION_BACKTRACKING.md).
- Prior initialized RANS/SA current+BDF/transfer/restart checks independently pass
  N/M1/1,2/1,2/2. The matching four-rank campaign was interrupted; preceding complete
  matrices are historical evidence. SU2/CGNS/reference reload and actual restart
  were checked on the recorded planar field. No CFD convergence/viscous accuracy.
- Focused curved actual-query and single-sphere hidden-volume oracles pass. The
  interacting plane/hidden-sphere quadrature still exhausts its work limit.
- Sensor gradation and composed-field gradation are separate. The composed fade
  violates the tested sensor HGRAD transport bound. Preserve localization/finer
  requests; do not infer combined gradation or spread wall tensors.

## Remaining full-goal gates

Complete curved construction/removal from coarse input; stronger distinct fields;
features/junctions/interacting layers and thin-volume quadrature; declared first-row
alignment and coverage beyond necessary feet; compatible composed-gradation limits;
current connected MPI1/2/4 N=M/M<N with conservative current+BDF/SA transfer, strict
bounds, geometry/full lengths, SU2/CGNS/reference persistence and real restart;
actual small RANS and practical M6 comparison with viscous accuracy references;
complete accepted adaptation/CFD lifecycle cost, memory/work/imbalance and affordability.
Partial/graph/mixed analytic components remain unsupported. Full physical unsteady3D,
general CAD, adjoints and mixed-volume/structured-layer guarantees remain separate.

Keep these gates intact. Goal4 starts after full Goal3 completion. SST/transition
and non-ideal-gas questions are [saved for later](FUTURE_PHYSICS_TRANSFER_QUESTIONS.md).
No cluster submission or external messaging is authorized by this handoff.

Earlier details and exact handoff bytes are retained in
[history](History/INDEX.md), [curved BL](GOAL3_CURVED_BL.md),
[interior placement](GOAL3_INTERIOR_PLACEMENT.md), and
[assessment reconciliation](STATIC_ASSESSMENT_2.md). Historical ACTIVE/job notes
must not be read as current execution state; check the actual process/session.
