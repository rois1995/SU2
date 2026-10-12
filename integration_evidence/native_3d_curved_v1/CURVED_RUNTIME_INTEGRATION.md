# Connected curved runtime integration

Repository: /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated.
Branch: codex/native-3d-curved-local. Goal 2 remains active and incomplete.

The blanket analytic-reference refusal has been replaced by an explicit, MPI-
consistent positive frozen surface tolerance. The normal remesher supplies
ADAP_HAUSD. Direct kernel callers with default zero tolerance still reject.
Sphere face error uses a conservative supporting-plane/radial bound; bump faces
use a Hessian bound for the complete graph-interpolation remainder. Nodal residuals
and arithmetic envelopes are included. Geometry violations enter selection even
when metric lengths already pass. Private geometric refinement must reduce the
area-weighted squared bound deficit, retain strict mean-ratio/sliver gates and
avoid increasing frozen changed-edge excess. Other edits cannot increase the
geometric deficit; coarsening cannot reintroduce a geometry violation.

Curved candidates use the existing streamed actual-centroid/edge import path after
projection, including private repair proposals. Sensor-only original-P1 priority,
declared exterior target and exact edge integration remain unchanged. New native
identities reserve space beyond the current source records. Split metadata stores
the actual projected point. The engine no longer rejects movement before the
boundary operator has projected and checked it against all incident charts.

Before publication, an indexed distributed view of the current working mesh finds
unedited tetrahedra near the complete replacement. A strict integer separating-axis
predicate detects interior overlap. Exact segment clipping additionally permits
closed contact only in the simplex of shared identities/coordinates, rejecting
unrelated tangencies and nonconforming contact. Replacement cells are checked
against one another and imported unedited cells. Source/index/transport/predicate
scratch is byte-admitted; resource refusal preserves the accepted CFD state.

Current limitation: curved commits are globally serialized; the source comment
requires collision reservations before admitting concurrently displaced cavities.
The current-mesh collision index is rebuilt for a curved proposal. This is an MPI
correctness implementation, with no efficiency/scalability claim. Existing planar
batching is retained. Cylinder, general nonconvex charts/CAD, fuller curved cases
and performance remain separate work.

The actual initial-mesh sidecar loader already supported analytic authority.
The new test uses it, rather than injecting the driver's retained reference.
A coarse cube with two bottom marker components declares
z=-0.01*x*(1-x)*y*(1-y), ADAP_HAUSD=0.003 and a prescribed 0.5I sensor. Geometry
alone drives a complete edge-star refinement. The actual CFD driver replaces six
tetrahedra by eight, transfers constant current and both BDF states, writes SU2 and
preserves its original analytic sidecar. MPI1/2/4 includes all supported M values
and therefore both M=N and M<N. The sensor is prescribed; this is not a solved
Euler-derived metric or a physical unsteady case.

Focused evidence: curved_runtime_v3/validation.json PASS, three cases per rank
on MPI1/2/4, using the configured68-object driver and full source/archive/member/
binary pins. Independent accepted-grid audit PASS: exact positive volumes, oriented
skin conformity/closure and no interior overlap; source coverage at vertices,
actual centroids and exact edge intervals; shape/sliver/constant-sensor length;
positive transported current/BDF states; actual SU2 output and unchanged sidecars.
The mesh volume is1.0002083333333334, closer to analytic1.0002777777777778 than the
initial1.0. The sampled wall error is0.00015625, below0.003; this independent sample
complements the production conservative whole-face bound, not a general Hausdorff
proof. Cases: curved_runtime_v3/mpi1/curved_runtime_m1_mesh_00001.su2 and corresponding
MPI2/4 M=N/M<N grids, source/mesh/state JSON, configs and native references.

Retained attempts: v1 is a controlled owned-build stop after static review caught
a nonexistent fixture helper; v2 records a duplicate-local-variable compile error.
V3 corrects both and tests actual initial sidecar loading. The first broader serial
selection passed99/100; its sole failure was the obsolete blanket-refusal diagnostic
assertion. Its receipt/log/source pins and assessment remain in
curved_runtime_regression_v1. The corrected broader attempt is
curved_runtime_regression_v2; update the result only from its terminal receipt.

Remaining full Goal 2 gates: complete sphere/cylinder/bump and feature coverage,
repeated refinement/coarsening and curved restart/CGNS, nonuniform/conservative
current+BDF transport with new/lost-domain accounting, solved CFD-derived Euler
metric/remesh/continue/restart and feasible resolved nonuniform/shock controls.
Do not close any full requirement from this first accepted prescribed-sensor event.
No geometry/quality/length/BL/fade/NOISE0/transfer/MMG gate was weakened. The permanent
AGENTS.md cadence was followed: connected code first, focused checks, then broader
regression at the actual runtime integration milestone.

Final integration regression:curved_runtime_regression_v2/validation.json PASS,
100selected cases per rank on MPI1/2/4. Its independent_runtime_validation.json
PASS covers all actual curved M=N/M<N outputs. Current source pins match the
worktree; full68-object binary/archive/member pins retained. publication_integrity
checks every original artifact directly or inside its byte-preserving archive.
Earlier failed/controlled-stop attempts keep their original statuses. Controllers
94180 and78676 terminalPASS; no owned heavy job remains live. Next substantial
implementation work is repeated curved conservative/restart/CGNS and real solved
Euler-derived metric cycles, with remaining geometry scope kept intact.
