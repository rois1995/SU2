# Codex handoff — start here

Worktree `/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated`.
Branch `codex/native-3d-bl`, based on Goal2
`c43a8a3907ea2655e4ebbb6286ab16c3fa625d8b`. Full Goal3 ACTIVE/incomplete.
**Curved-target milestone achieved:** the native engine now constructs and accepts the
complete original1402-cell curved target under unchanged constraints on MPI1/2/4.
All give1368 cells/423 nodes/64 wall faces;0 long edges, Lmax1.795194775. Serial
qmin .211386792/Jmin .052846703; independent exact geometry/all vertex links/no
positive overlap/normal feet/height/authority and exported-tensor arithmetic pass.
Construction work2996/3036/2908, within4096;300s ceiling unchanged. This is **not
full Goal3 completion**, cold construction, CFD transfer/restart, wider physics or
performance qualification.

Inspect the actual native result:
`integration_evidence/native_3d_bl_v1/region_construction_v1/native_complete_v1/curved_grid_adapted.su2`
and its unchanged `.native_ref` authority sidecar, or `accepted_metrics.vtu` in the
same case folder. VTU fields are measured mesh quality, not fabricated CFD output.
Read [native outcome/provenance](integration_evidence/native_3d_bl_v1/region_construction_v1/native_summary.json)
and [construction details/limits](Handbook/GOAL3_REGION_CONSTRUCTION.md).

The connected C++ constructor plans surface edits, exact-height wall apices,
shared interfaces and a closed transition/core using actual frozen queries, without
Python/proxy/saved-grid dependencies. Existing config supplies height/growth/thickness.
The engine stages a bounded visible region on one worker, imports original source
through the bounded protocol and uses existing versioned MPI publication. Initial
scope: one complete radial wall with planar caps, visible ball, <=256 imported donors.
This is MPI-aware publication, **not parallel region construction/scaling**.

MPI outputs have different admissible triangulations/placements; qmin differs by
~1.25e-5. Inputs have equal coordinates and normalized donors/geometry. Do not claim
bitwise deterministic output or transported-metric qualification. All ranks pass
strict shape/height/geometry/full-length gates and accepted-input immutability.

The first native attempt rejected the primitive512-face height-check limit; its
complete private candidate and receipt remain archived. Region validation now checks
the whole skin/topology and applies the same height/foot predicate per incident cell.
Primitive64/128 limits and all physical acceptance thresholds remain unchanged.

Next: qualify complete coarse cold construction and removal on the supported geometry,
then full CFD/current+BDF transfer/restart and materially different fields/geometries.
Preserve every full Goal3 gate; do not extend the old local-fallback chain or infer
large-domain support from this visible-region result. Rejected recovery remains
archived on `codex/native-3d-boundary-recovery-study` at39a1f487.
All owned jobs are terminal. Current build/binary/source provenance is pinned in the
native receipts; older39a1f487 prototype-only oracle binaries have been superseded.
Foreign `ClusterResults/` and dependencies remain untouched; no cluster run was started.

[Full remaining Goal3 gates](Handbook/GOAL3_CURRENT.md),
[requirement audit](integration_evidence/native_3d_bl_v1/GOAL3_REQUIREMENTS.json),
[permanent rules](AGENTS.md), and
[deferred SST/transition/non-ideal-gas questions](Handbook/FUTURE_PHYSICS_TRANSFER_QUESTIONS.md).

One owned heavy job, nice19/threads1/build-j1/MPI≤4, bounded sampled affinity. No
cluster submission or messaging. Leave AdapNoExt, foreign ClusterResults and
dependencies untouched. No production Euler/BL modes, weaker gates, field/target
resets, budget ladders or local-fallback chains in place of complete construction.
