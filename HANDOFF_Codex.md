# Codex handoff — current state, 2026-10-20

Worktree `/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated`, branch
`codex/native-3d-bl`. Full Goal3 **PAUSED at user request/incomplete**; Goal2 base
`c43a8a3907ea`. All owned build/test handles were terminal at the pause.
Leave AdapNoExt, foreign ClusterResults/dependencies/jobs untouched.
[Full requirements](integration_evidence/native_3d_bl_v1/GOAL3_REQUIREMENTS.json).

## Latest checkpoint

[Finite-component implementation, results and resume boundary](Handbook/GOAL3_MAPPED_COMPONENTS.md).
Published implementation/audit checkpoints `283587a6d1` / `feb83a3f68`; later C++
resource/fixture corrections and validation follow-up remain local, uncommitted.
This handbook update does not resume the goal or publish those C++ changes.

Official build PASS, binary SHA-256
`501393b67053e865b5c9f88c57295e72abaaed38cde049b21e480beee99eb3c1`.
Explicit v6 releases reference-certified planar seams; v5 remains facet-local.
Finite containment, physical border/junction protection, private movement, atomic
publication/persistence and frozen original BL identity are connected. Small
components scan <=16facets; larger ADT preparation/query storage is admitted.
Original sensor-only P1, actual-query BL/finer/fade and all mesh/transfer gates stay.

Scoped PASS:468rational samples/2340predicate checks;3finite-domain cases/
146assertions;1publication case/76assertions across v5/v6 policies.
Combined component selection FAIL: corner tangent-helper assertion uses an incomplete
bottom-face patch; complete-context fixture correction remains pending. No production
corner defect or missing forward-binding defect is established by that failure.
No new v6 MPI/N-M, complete transfer/real restart or independent output audit ran.

## Difficult annular BL remains unaccepted

[Exact case/evidence](integration_evidence/native_3d_bl_v1/mapped_component_geometry_v1/annulus_n1/).
Same frozen target initializes19.999579/budget20 with0.00219995heuristic indicator;
normalization is integration-limited. Separate composed audit18violations/max4.57957
remains; no wall-tensor spreading/reset.

Construction hits120s elapsed limit at7399/8192work:13private edits,75tets/31nodes,
68edge tasks/67cached invalid cells,6440joint trials/0accepted. Last saved candidate
q0.1988447689<0.2, J0.0695926908, cell16825. Accepted CFD state retained.
Private rejected JSON/binary/v6 reference and before/after trial parameters are saved.
Adapted rejected SU2 conversion is pending; current `.su2` files there are inputs.
Native120.206s/63.450CPU seconds; complete test149.118s under contention. These are
rejected diagnostics, not evidence of speedup, infeasibility or accepted affordability.

On explicit resume: correct the incomplete fixture, then use the exact failure to
reconsider missing construction/coordination before another fallback or higher limit.
The accepted Cartesian control also used0regional-core/front edits but3joint accepts;
regional exclusion alone is not an established cause. No correction stage ran here;
repeated-stage frozen-profile reuse remains unverified.

## Full scope and qualified historical evidence

The [general symmetry roadmap](Handbook/GOAL3_GENERAL_SYMMETRY.md) remains required:
arbitrary2D lines/3D planes; finite nonconvex, disconnected, multi-hole/multi-body
regions; simultaneous junctions; common Euler/RANS geometry; N=M/M<N; conservation,
admissibility, SU2/CGNS/persisted reference/real restart and an exterior workflow.
Original physical-border coarsening, mixed analytic/mapped joints, affine BL walls,
physical source envelopes and complete2D/exterior lifecycle remain open/unsupported.

[Historical v5 mapped lifecycle](Handbook/GOAL3_MAPPED_PARAMETERS.md): uniform Euler/
RANS-SA N1M1/N2M1/N4M2/N4M4,6->58tets, conservative current+BDF2 and independently
audited SU2/CGNS/reference reload pass in their own source/binary scopes. Its v5
annular BL rejects8192work/18edits. Do not relabel that evidence as current v6 proof.

Accepted inspectable grids, historical qualification retained:

- [Cartesian annulus](integration_evidence/native_3d_bl_v1/planar_symmetry_reference_v1/annulus_v10_n1/symmetry_annulus_bl_budget_20_m1_written_SU2.su2),
 194tets, full finite geometry/height/metric/transfer/real restart audits in its scope.
- [Spherical repeated RANS N4M2](integration_evidence/native_3d_bl_v1/sphere_rans_lifecycle_v1/n4_m2_dependencies_v1/update/event_written_SU2.su2),
 5440->3768tets/783nodes. Reuse qualified cold construction; no new viscous or affordability claim.

[Budget-aware BL policy](Handbook/GOAL3_BUDGET_AWARE_BL.md): M6 budget300k selects
lambda.875,295068+/-5769heuristic/integration-limited.100k sampled-profile conflict
is not exact infeasibility. Global uncertainty allocation/calibration, interacting
layers, composed gradation, viscous validation and complete accepted-event costs
remain open. [Outcome ledger](Handbook/SUGGESTION_OUTCOME_LEDGER.md).

## Operating rules and references

[AGENTS.md](AGENTS.md): substantial connected capabilities/difficult case early;
reuse qualified evidence with source/build/binary/config/input/helper provenance;
no full suite per tiny edit, fallback ladder or unnecessary CFD reruns. Keep pinned
C++ unchanged while an owned build/test is live; inspect actual handles first.
One heavy job, nice19/build-j1/threads1/MPI<=4; observe host RAM/contention. Compiler
scratch stays on4TB. Announce full new case folders, maintain handbooks and push
reviewable checkpoints. No cluster requests/submissions while working locally.

Claude Opus5.5 medium/high is authorized; normal authentication works. Reviews are
reasoning input, not numerical evidence. [Source/mapping policy](Handbook/GOAL3_SOURCE_ENVELOPE.md);
[cleanup receipts/restore](Handbook/DISK_SPACE_CLEANUP_2026_10_18.md), approximately
37.98GiB cumulative recovered; historical archived executables need restoration.
[Future SST/transition/nonideal transfer questions](Handbook/FUTURE_PHYSICS_TRANSFER_QUESTIONS.md).
Three-element meshes: `/media/rausa/4TB/Validation/ThreeElementAirfoil/30P30N/Meshes`.
