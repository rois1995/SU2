# Goal 3 — current implementation and qualification

Repository `/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated`, branch
`codex/native-3d-bl`. Goal3 remains ACTIVE and incomplete. The full objective and
acceptance gates are preserved in [the requirement audit](../integration_evidence/native_3d_bl_v1/GOAL3_REQUIREMENTS.json).

## Current construction work

[Coupled actual-metric patch optimization](GOAL3_PATCH_OPTIMIZATION.md) now accepts
the actual hard50→85-cell cylinder replacement in15 trials, with native frozen
changed-edge progress/height/q/J and independent exact geometry/feet. The same
strict1402-cell target reaches1984 cells/25 long edges (previous38), but rejects
at300s/3985 of4096 work. All remaining lengths are physical:13 cylinder/12 rim,
none interior. No complete curved generation or speedup follows.

470 Jacobian assertions/five repair controls, independent whole geometry/no overlap/
98 exact feet/height/exported-tensor arithmetic, and bounded MPI2 donor-reference
protocol pass in their declared scopes. Exact stage/object/binary provenance and
preserved eager-scheduling regression are in the linked handbook/summary. Eager
scheduling is withdrawn; the engine retains published scheduling. All owned jobs
terminal. Next work must address the actual physical-wall reconstruction blocker
with primary-progress and phase-cost evidence, not budget/fixture ladders.

## Rejected recovery and required next milestone

The independent-branch/partial-continuation recovery can accept one actual blocked
edit, but the complete unchanged target still rejects: eager79 and staged30 long
edges, versus published25. This is not the next milestone. Preserve the rejected
implementation on `codex/native-3d-boundary-recovery-study`; retain published00e3
production policy and read [the construction reassessment](GOAL3_CONSTRUCTION_REASSESSMENT.md).
User2026-10-15 requires a complete accepted curved mesh under unchanged constraints;
reconsider construction strategy before another local fallback. Full Goal3 stays open.

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
