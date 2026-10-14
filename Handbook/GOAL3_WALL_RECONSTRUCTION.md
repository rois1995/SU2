# Goal 3 wall reconstruction — current frontier

Repository: `/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated`.
Branch: `codex/native-3d-bl`. Full Goal3 ACTIVE and incomplete.
Cases: `integration_evidence/native_3d_bl_v1/wall_reconstruction_v1`.
All owned build/test/audit handles are terminal at this checkpoint.

## What is implemented

Requested wall children and transition are carved as a single replacement, with
complete wall-apex-star dependencies and bounded node identities. Placement scores
the wall children, and failed geometry identifies an obstructing interface from
actual proposed normal points. Existing interior points are reinserted privately
so rebuilding a patch need not erase already resolved interior distribution.
Cavity growth is back to12 attempts; existing64-cell cavity and work/time ceilings
remain. All published mesh/metric/height/footprint/atomic gates remain intact.
No Euler/BL solver or input-grid transition modes were introduced.

Two focused serial geometry cases pass (including old interior-coordinate
preservation, normal wall children, finite interface/coverage and atomic rejection).
The independent witness certificate is geometry-only, not a full-field remesh.

## What the stronger target demonstrates

Source is the same frozen original six-cell scalar P1 field `diag(4+12*x)` plus
actual square-wall BL (h0 .1,g1.2,T.3,core4,unchanged fade).
Current native result:117 refinement edits,418 private cells, work4096 exhausted;
zero accepted joint wall reconstructions. Independent coverage/shape/height/footprint
checks pass, but full composed wall edge0,2 is4.3997755 (limit1.8). No CFD replacement
or layer-release stage occurred. Rejected output and all previous failures are kept.

A generated control576-cell mesh independently passes these same metric/geometry/
height/footprint acceptance contracts: qmin .5819092,Jmin .3105078,Lmax1.6644668.
It is named `feasibility_control_not_native.su2`. It proves declared-target
feasibility, not native generation or zero centroid tilt. Thus the native failure
is operator blockage under these contracts; global impossibility is not supported.

A numerical LP on the earlier terminal patch diagnoses the single-background
visibility restriction. It is not an exact infeasibility proof. Neither increasing
growth12→64 nor a single proxy/carving search fixed the stronger case. The latest
multi-point approach improves overall search progress but still needs reconnection
under the actual varying field. Timings from rejected runs are diagnostic only.

## Next substantial step

Reuse exact saved cavities and original source for local replay. Add bounded
metric-aware tetrahedral internal reconnection/face-edge flips to the complete
multi-point proposal, and inspect bad wall-child/transition topology directly.
Keep the interface, requested height, original sensor field and final strict gates.
Do not raise ceilings, relax the target or repeat whole CFD/large MPI campaigns.
Finish a complete serial stronger-field adaptation before the connected MPI matrix.

The expensive stronger case is explicitly tagged `[.][NativeFinerSensorBL3D]` to
avoid every unfiltered Catch invocation. It remains a required failing Goal3 gate;
run it explicitly through the resumable collector. This is a schedule, not a waiver.

See [evidence summary](../integration_evidence/native_3d_bl_v1/wall_reconstruction_v1/summary.json),
[reproduction instructions](../integration_evidence/native_3d_bl_v1/wall_reconstruction_v1/README.md),
[current overall goal](GOAL3_CURRENT.md), and [full requirement audit](../integration_evidence/native_3d_bl_v1/GOAL3_REQUIREMENTS.json).
