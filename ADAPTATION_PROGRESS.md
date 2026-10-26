# Native adaptation progress

Companion to [ADAPTATION_TODO.md](ADAPTATION_TODO.md). This file records reviewable
changes and their validation status. The TODO remains the acceptance ledger.
Updated: 2026-10-26.

## Current result

**DC-17 and DC-18 remain open. No full M6 adaptation has passed final acceptance.**
The current work connects construction to distributed quality repair and corrects
bounded geometric integration. Validation46 is building; its changes are unvalidated.

The final requirements remain q ≥ 0.20, J ≥ 0.05, metric edges ≤ 1.8, first height
within 10%, geometry preservation, the configured 500k metric-complexity target,
conservative transfer and restart. The 500k target is not a literal tetrahedron count.
Curvature uses `ADAP_HAUSD` in mesh units, independently of h0.

## Checkpoints

| Local checkpoint | Change | What the evidence establishes |
| --- | --- | --- |
| `fdb6ad2683` | Persisted curved import and original-source runtime queries | Small cubic Euler/RANS workflows complete with conservative BDF2 accounting and SU2/CGNS reload. Full M6 remains separate. |
| `79bfba6f8f` | Reuse the collision search while retaining exact contact checks | Connected serial/MPI checks pass. Full M6 validation41 still rejects its private mesh. |
| `e398807a2c` | Reactivate curved demands after neighboring edits; admit compatible concurrent proposals | 105 serial cases, 105 cases per MPI2 rank and all-worker curved runtime checks pass. Two simultaneous commits are exercised. Full M6 validation43 fails final acceptance. |
| `c02010b08d` — WIP | Connect validity-only construction and bounded point-placement repair; revise radial integration | Preserves failed M6 and validation44 evidence. Subsequent validation45 still fails, as recorded below. This is not a validated capability checkpoint. |

Current unvalidated correction, validation46:

- Replace the new single-point search with the existing bounded topology optimizer
  for repair cavities. It can swap edges/faces and collapse interior vertices;
  cavity boundaries remain fixed. Exact interface, height and frozen-field checks
  still decide whether a proposal can proceed.
- Align validity-only repair with the optimizer's minimum normalized q/J objective.
  Final quality thresholds and the changed-edge excess constraint remain unchanged.
- Integrate the quadratic collapse Jacobian in the two-point quadrature weights,
  preserving polynomial moments. Avoid cancellation that produced zero weights at
  rounded endpoints. Retain the higher-order error checks and existing work limits.

## Latest evidence and failures

| Campaign | Result | Consequence |
| --- | --- | --- |
| Full M6, validation43, MPI2 | Rejected at the 1800-second native limit: 26,567 private edits; 218,099 cached invalid cells. 238,305 metric trials refused intermediate shape. | Scheduling improvements alone do not complete construction. The accepted CFD state is retained. |
| Validation44 | Build passes. Repair 10/11, volume 8/10 and connected serial 104/105 cases pass. | Fix the mapped fixture's missing tolerance; preserve planar integration; address sphere point usage and curved-runtime repair cost. |
| Validation45 | Build passes. Focused checks: 19/22 cases, 1,066,854/1,066,866 assertions pass. | The mapped fixture and planar complexity regression are fixed. Sphere density/volume/first moments finish within the cap, but eight zero weights and a second-moment regression remain. Cubic Euler/RANS still exhaust repair work with seven invalid cells. |
| Validation46 | Build active; independent weighted-rule moment check passes. | Production integration and repair are **not yet validated**. No six-rank or full M6 result exists for this change. |

## Next acceptance steps

1. Resolve every focused failure above without increasing limits or weakening final gates.
2. Run connected serial and six-rank MPI checks, including concurrent curved
   transactions, final mesh checks, conservative transfer and reload.
3. Reassess the difficult full M6 workflow under the agreed constraints. An accepted
   Euler mesh is followed by the remaining RANS/BL integration and qualification.

Other open DC-17/DC-18 requirements include changing-frame gradation, the M6
complexity budget, general curved BL support/chord/quadrature consumers, FIRST_ROW
surface-error demands, fitted-plane seam behavior and automatic curved initialization.
The existing normal-only relaxation contract remains in force while the question
about tangential relaxation outside the protected band is unanswered.

## Detailed records

- [Current handoff](HANDOFF_Codex.md): active job handle, resource limits and immediate next action.
- [Curved construction and failure history](Handbook/DC18_CURVED_SOURCE.md).
- [DC-17/DC-18 completion handbook](Handbook/DC17_DC18_COMPLETION_2026_10_25.md).
- [Validation43 result](integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/validation43/summary.json).
- [Validation44 result](integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/validation44/summary.json).
- [Validation45 result](integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/validation45/summary.json).
- [Validation46 scope and status](integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/validation46/summary.json).

Checkpoints are local. Remote export remains pending the destination approval
previously requested after automatic approval review rejected the push.
