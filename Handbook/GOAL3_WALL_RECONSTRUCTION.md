# Goal 3 wall reconstruction — current frontier

Repository: `/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated`.
Branch: `codex/native-3d-bl`. Full Goal3 ACTIVE and incomplete.
Cases: `integration_evidence/native_3d_bl_v1/wall_reconstruction_v1`.
See STATIC_ASSESSMENT_2.md for the published earlier qualification; current repair qualification is separate.

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
Previous native result before coordinated movement:117 refinement edits,418 private cells, work4096 exhausted;
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

## Current connected implementation

Private actual-field2/3/4-cell reconnection now coordinates up to eight constrained
interior vertices before requiring descent from an individual move. Geometry,
first height and normal footprints are checked on the complete simultaneous pose;
shape caches are invalidated when coordinates change. All intermediates remain
private. The saved whole-cube proposal is a reconstruction witness, not the exact
last terminal engine cavity.

Mean-only and individual metric-direction movement made no improvement over the
13-flip residual. Simultaneous movement plus reconnection uses152 trials,14 flips
and3 moved vertices and brings shape loss2.1209455 to0. Independent audit confirms
exact coverage/disjointness, retained skin and wall footprints: qmin .2274514,
Jmin .0503909, minimum footprint .2193074. Lmax6.73871217 still fails because the
saved patch retains long physical-interface edges. Passing this witness does not
establish accepted mesh generation.

The engine now attempts this repair after a joint wall proposal fails shape. It
imports full admitted-cell donor support rather than support for previous queries,
renames moved existing vertices within reserved ranges, collectively charges
actual trials to the existing global work budget, and rechecks full composed
metric/progress and geometry before publication. Donor-cap or incomplete-shape
proposals remain refused. Original dependencies and all strict gates are retained.
The current private repair accepts at most64 input cells and128 replacement cells;
larger rebuilt proposals are skipped, not silently accepted. This remains a limit
of the private path, distinct from global mesh feasibility.

The complete stronger-field run now accepts3 joint wall reconstructions (previously0),
104 refinement edits and453 cells, but exhausts work4096 including3305 repair trials.
Independent final audit: volume1, qmin .225576,Jmin .054521, exact first height,
minimum normal-foot barycentric0, longest edge2.5437949>1.8. The event is rejected;
removal and CFD replacement are not reached. Do not interpret its42.55s diagnostic
time as an accepted-adaptation performance result or a scaling comparison.

Evidence folders:
- `integration_evidence/native_3d_bl_v1/connectivity_replay_v1/replay`:
  saved private proposal and independent audit.
- `integration_evidence/native_3d_bl_v1/connectivity_replay_v1/integration`:
  focused connected-repair, existing construction/removal and curved controls.
- `integration_evidence/native_3d_bl_v1/wall_reconstruction_v1/sensor`:
  complete required gate, rejected mesh, log, frozen source snapshots and audit.

Seven focused serial integration controls PASS (855 assertions), including the
existing complete construction/removal controls and curved joint geometry. The
current compiled private witness also passes the independent audit. All owned
jobs are terminal; no MPI or new CFD/restart/performance campaign was run.

## Next substantial step

Address the remaining surface/first-row/transition arrangement together. Use the
current rejected mesh and frozen donor source to replay complete wall-apex
requirements, and build a coordinated surface-strip proposal with multiple child
points, transition insertion and reconnection. Assess the whole proposal against
full shape, height, footprint and composed-edge/progress gates. Avoid another
sequence of isolated direction variants, larger ceilings or target relaxation.
Finish the full stronger-field construction/removal before claiming this capability
or starting its larger MPI qualification campaign. Existing focused controls
establish compatibility only, not general curved/interacting-wall robustness.

The expensive stronger case is explicitly tagged `[.][NativeFinerSensorBL3D]` to
avoid every unfiltered Catch invocation. It remains a required failing Goal3 gate;
run it explicitly through the resumable collector. This is a schedule, not a waiver.

See [evidence summary](../integration_evidence/native_3d_bl_v1/wall_reconstruction_v1/summary.json),
[reproduction instructions](../integration_evidence/native_3d_bl_v1/wall_reconstruction_v1/README.md),
[current overall goal](GOAL3_CURRENT.md), and [full requirement audit](../integration_evidence/native_3d_bl_v1/GOAL3_REQUIREMENTS.json).
