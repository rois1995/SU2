# How to proceed

Updated 2026-11-08. **Zero meshes accepted.** Read [HANDOFF_Codex.md](HANDOFF_Codex.md)
first for owned handles, active work and current provenance. Historical build notes
never override the current handoff.

## Current result and active work

Build254's first M6 event refused at its unchanged elapsed-time limit. Its terminal
census is complete: **741,236 cells; 251 q-bad, 205 J-bad, 95 geometry-bad, zero
size-bad**. All thirty capture files are sealed. All 1,520 pins were verified before
one exact owned MPI stop; every owned solver handle is gone. The driver continued
CFD on the original grid after refusal: those adap-named files are not adapted CFD.

The meaningful inspection export and independent readback have passed. The saved
`staged57/m6_runs/euler_build254_conditioned_mach_only_real_cfd50k/inspection/rejected_mesh.vtu`
has **150,080 points and 741,236 cells**, the captured metric tensor as PointData,
and correctly sized q/J/geometry fields as CellData. All 260 bad cells, 2,400
neighboring cavity cells and reference geometry are saved. The arrays, point
ordering, SPD metrics and positive volumes were independently checked. Large
artifacts remain local; hashes and reviewable metadata are pushed. The corrected bounded selection finds117bad cores owning wing faces and143
interior bad cores touching CURRENT wing vertices;NONE is free-interior. A
provisional original-reference-node classification was wrong and is preserved.
Six complete contexts fit the existing64old-cell cap. Both bounded diagnostic
checks are terminal and reproduce cached q/J and strict geometry counts/sums.

ONE connected correction256passed58serial cases and99cases on EACH MPI6rank.
The full same-seed two-event256M6 attempt is now RUNNING FROZEN:shared geometry guidance now uses the
canonical squared certified-face excess measured by the complete census. An
actual raw contraction removes10q/8J failures andone failed physical face,
reduces certified debt15%,keeps worst error unchanged and removesone point;
old area weighting nevertheless vetoes it. See
[the current correction](Handbook/ADAPTATION_GEOMETRY_DEBT256_20261108.md).
Qualification is sealed;do not repeat it. Let the full attempt drive the next step.
The scalar correction does not guarantee production reaches every bounded
candidate;no new operator,support bound,scheduler,metric or gate was introduced.
Read HANDOFF for actual live handles;all current256source pins remain frozen.

The preserved recovery correction254 is integrated and qualified: the existing QR
condition bound now applies to base quadratic Hessian fits, not only richer wall
fits. Unsupported first rings use the existing owner-complete second ring; a
failed final fit keeps the caller fallback. No sensor filtering, new option,
quality-aware construction or gate change was introduced. The actual same-seed
replay leaves flow and coordinates bit-identical and changes 71 Hessians. It cuts
initial quality failures from 106,454 q / 106,204 J to 27,016 q / 27,453 J.

The full attempt then reached guarded surface reconstruction and shape reclosure,
which earlier timed attempts had not reached. However, its late cycles churned
rather than converged. Best complete shape counts were 34 q / 20 J; the smallest
geometry-bad count was 60. Neither occurred on an accepted mesh. The final counts
above remain failures. See [the diagnosis](Handbook/ADAPTATION_TARGET_DELIVERY253_20261107.md)
and `staged57/m6_runs/euler_build254_conditioned_mach_only_real_cfd50k/terminal_assessment.json`.

## What the evidence establishes

- Sampled surface compression is broad in the saved CFD field. The metric requests
  roughly 4–7% chord spacing where existing triangles are already roughly 3–4%.
  This explains little first refinement in those strips; it does not identify the
  complete lambda shocks or establish physical shock resolution.
- The old leading-edge target has the expected swept-span direction and spacing
  ratios about 2.55–3.28, while actual triangle extents are much shorter and less
  anisotropic. Exact captured production edge lengths confirm very short edges.
  These measurements do not prove which edges geometry requires.
- Euler without requested BL constraints has no geometric metric composition:
  q/J and metric edge lengths use the Mach tensor; geometry is separately
  certified/refined. This is a source fact, not authorization to add curvature
  demands or change metric normalization.
- The corrected target improves the construction's starting shape substantially,
  but the remaining closure blocker is now **repair power and admission**, not a
  demonstrated need for another scheduling or geometry-speed project. Late
  optimizer commits can reduce debt while introducing more q-bad cells than they
  retire. Collapse/reconnect can make thousands of trials with no commits.
- Earlier bounded cavity controls are partial evidence. They did exercise
  geometry-constrained boundary smoothing (302 candidates, seven moves), despite
  `boundary=false`; that flag gates generic boundary topology. They did not
  exercise the dedicated `GeometryRepair` metric-change validator or nonlocal
  publication. Guidance scores are not automatically equivalent to terminal
  geometry certificates. Do not infer production admission from those controls.

## Next connected cycle

1. Use actual254 residuals, not unchanged-input assumptions from247/251. Select a
   small set of complete smooth-boundary and adjacent-volume cavities, including
   shape-only, shape-plus-geometry and geometry-only cases, with clean neighbors.
   Classify actual physical faces/reference associations, mobility and body/feature
   constraints. Separate interior cells from cells whose vertices touch the wall.
2. Reuse existing move, collapse, reconnection, retained-star optimizer and support
   import facilities. Compare original-star controls against complete affected
   contexts within existing bounds. Check the validator appropriate to the actual
   phase, exact geometry authority/support, refreshed parameters, q/J churn,
   geometry failures/excess, metric edges, point count and useful progress per
   second. A positive primitive or lower local guidance score is insufficient.
3. Test **one connected hypothesis** for the demonstrated residual limitation.
   A combined surface/volume candidate may be necessary when separate operations
   cannot leave a useful patch. Establish that it can be constructed and admitted
   on the actual residuals before integrating it. Do not introduce another local
   fallback merely because an easy fixture passes.
4. Qualify the affected behavior with focused serial checks and one MPI6 milestone,
   including curved seams, holes, separate bodies, genuine features, requested
   heights, cached/recomputed results, resource exhaustion, conflicts, empty ranks,
   dependency invalidation and complete publication. Then immediately run the full
   same-seed two-event M6 attempt again under unchanged budgets.

Keep validity-first construction. Do not add q/J ranking or vetoes during it,
a blanket minimum-edge gate, larger cavities, a scheduler, a geometry
representation, mandatory curvature metric or a norm/complexity/sensor change
from a paper or a scalar diagnostic alone. Preserve CAD as optional. Record
unproven alternatives in [the backlog](Handbook/ADAPTATION_BACKLOG.md).

## Completion remains unchanged

Two strictly accepted Euler CFD → mesh → CFD → mesh events at **50k metric
complexity**; geometry 1e-4, q≥.2, J≥.05, max edge1.8 and requested heights remain
strict. Report actual points/cells separately from complexity.

Validate conservative admissible transfer before publication: actual CV-mass
solve residual ≤1e-10 for every field/time level, all states admissible, scaled
TARGET integral defect ≤1e-12. Printed old/new domain integrals are not enough.
Require finite continued CFD on the adapted mesh and a fresh isolated actual
adapted disk restart with matching point order/coordinates and distinct input and
output prefixes. Resume-cycle numbering/overwrite and rejected-event consumption
remain open issues; isolation is not a fix for them.

After Euler succeeds, complete dedicated BL/MPI qualification on curved walls,
junctions, interfaces, subsequent operations, redistribution and requested
heights, then the corresponding **RANS500k** workflow. Generic MPI tests do not
establish that capability. Keep one owned heavy job, source freezing, explicit
resource/work/time limits, preserved failures and pushed reviewable checkpoints.
