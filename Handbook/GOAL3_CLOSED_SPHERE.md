# Complete closed-sphere construction and regional coarsening

2026-10-18; full Goal3 ACTIVE/incomplete. Final coarsening integration build, serial/MPI4 qualification and independent
checks PASS. The complete serial construction below retains its sealed sampler-only executed
source; final-source release and actual primitive integration are checked separately.
Do not claim MPI BL construction or a full CFD/transfer lifecycle from this mesh replay.

Worktree NativeIntegrated, branch codex/native-3d-bl.
[Evidence](../integration_evidence/native_3d_bl_v1/closed_sphere_construction_v1/).

## Missing capability and connected change

Cylinder workflows already pass. A closed spherical wall exercises curvature in
both surface directions and complete front closure without planar caps. The
starting accepted input has8tetrahedra, an explicit complete unit-sphere authority,
original identity P1 sensors and generated frozen geometry collars (32donors).
The target requests height.05, growth1.2, thickness.2, geometry.04 with the existing
wall ceiling.025, q>=.2, J>=.05 and every composed edge<=1.8.

The inherited edge-split surface planner creates448faces and604surface trials.
4096work cannot cover even its first band:2tetrahedra per triangle plus4per shared
edge gives3584band cells, or4188with planning. A single6144work probe also exhausts
during transition. Both retain their8input cells and exact private proposals.
This is a constructor work floor, not physical infeasibility.

The existing regional constructor now proposes a complete geodesic sphere,
subdividing only until actual geometry, first-wall-cell quality and full-metric
surface edges pass. One shared midpoint per edge preserves conformity. Work/cell
admission bounds refinement. Other analytic geometry, source interpolation,
composition, fade, first-height and final whole-region checks stay intact. No new
config option, testcase/solver-transition mode or local volume fallback.
Uniform proposals are limited for localized or strongly tangentially anisotropic
fields; they reject rather than weaken the target. This is one admitted convex
spherical body on one generation worker, not exterior/holey/general CAD meshing.

The removal replay exposed an existing shortcut: an admissible fine mesh bypasses
whole-region coarsening and spends its budget on local collapse searches. The
connected engine change permits a whole-region proposal when the coarsening intent
finds genuinely short metric edges. For an already admissible input, only a fully
validated smaller mesh can publish. Existing repair of an inadmissible input may
still need more cells and keeps its prior behavior. Non-improving proposals retain
the existing local path; all work remains charged.

## Complete serial native construction and independent audit

Same6144work/300seconds as the preserved failed comparison:5440accepted tetrahedra,
1127nodes,320wall faces,5604work, one transition front/one core region. Native
qmin.39823288, Jmin.10043711, maximum full edge1.64204404. Input bytes and geometry
authority unchanged. Geodesic planning uses162surface trials. No CFD advance,
Hessian/normalization rebuild or replacement of the original donors occurred.
Inspect geodesic_v1/accepted.su2 and its authoritative sidecar.

Independent exact audit passes all1127vertex links,138393overlap candidates plus
10720opposite-face certificates, positive volume/oriented skin, normal footprints,
height relative error3.20e-15, node authority1.12e-16 and maximum chord sag.017754.
Exported-tensor q/J arithmetic residual5.56e-16. All6726native full edges are covered
once and pass. Full composed tensors/edge integrals remain native measurements;
there is no independent spherical BL field oracle or viscous accuracy proof.
Prescribed height here is the unchanged altitude/foot contract on actual wall
triangles; it is not a claim of exact normal distance from the smooth sphere.
The independent audit checkpoints/resumes without native reruns (231.3seconds).

Sampler-only serial and MPI4 controls pass the new translated-sphere/atomic-refusal
case and existing primitive runtime. An independent retained oracle passes15actual
primitive Euler lifecycles/30events, covering N1M1 and N4M1/2/3/4, current+BDF/domain
accounting, transported sensors, output/reference/reload/restart/SU2+CGNS. These are
small primitive workflows, not the5440-cell BL sphere or converged CFD accuracy.
Final connected-source serial/MPI4 controls and the retained independent oracle
also pass all15primitive lifecycles/30events. Sampler-only earlier receipts remain
explicit historical evidence; the completed construction was not repeated.

## Performance and next decision

The complete native construction takes202.612seconds on this contended correctness
run, launcher206.178seconds. This is NOT an affordability or speedup result. The
failed earlier constructors did not produce equivalent accepted meshes. Saved
sampling/first-band/core clocks do not isolate the whole remainder; capture all
validation/transition/exclusive clocks in the next required lifecycle. Region
service point/edge counters are zero on its direct small-source seed path; that
must not be interpreted as zero actual metric queries.

A possible later validation-cost strategy requires proof before code changes.
[Lipman2014, Theorem1](https://arxiv.org/html/1310.0955v2) gives global bijectivity
under a nondegenerate orientation-preserving simplicial map and a bijective
boundary map on the stated compact manifold domain. Positive cells alone are
insufficient. Any replacement certificate must independently establish every
assumption and be checked against retained exact collision oracles. No such
shortcut is enabled here; first measure where the event time is spent.

The final saved-state removal succeeds5440->320 with483work/57.3837seconds,
within the remaining540/97.3875. Combined native work6087/6144 and time259.996/300.
Both stages preserve original source records and authority and pass independent
geometry/shape checks; release additionally passes every identity sensor tensor
and exact Euclidean full edge. Native validation takes50.3266/57.3837seconds in
release (~87.7%). It includes multiple checks; no collision-only attribution yet.
The prior540-work local-collapse replay rejects after4205private cells and is
preserved. A setup-only missing-reference initialization error is also preserved.

Pending: MPI BL construction/actual CFD lifecycle, wider
fields/interactions, independent composed-field/gradation and viscous accuracy,
complete event affordability. The separate M6 exact-facet conflict remains; do not
rerun its unchanged frozen field or change its authority without the pending user
preference. Full Goal3 is neither narrowed nor complete.

## Next substantive performance step

Profile/split the measured validation cost using the saved accepted meshes, without
another construction run. Inspect the existing exact ConformingDisjoint broad
phase for repeated contact checks at shared vertices/edges. A separating AABB plane
with a common extremal simplex may permit an exact cheap certificate when shared
identities/coordinates are verified; otherwise keep the existing exact predicate.
This is an unimplemented hypothesis, not an enabled shortcut or measured gain.
Keep complete validation and its negative controls; do not optimize tensor algebra
before accounting for the dominant measured phase. Full-event transfer/repartition/
solver/output cost still needs actual BL lifecycle measurement.
