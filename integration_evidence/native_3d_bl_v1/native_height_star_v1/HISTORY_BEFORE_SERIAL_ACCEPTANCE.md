# Native coordinated first-height ownership: connected implementation

Status: native complete frozen replay PASS; full lifecycle/coarsening correction pending. Full Goal3/general symmetry remain
open. Production contains no fixture identities, wall coordinates or solver modes.

The unchanged annulus_v7_n1 target has four oversized corner-to-height-apex edges
and a complete36-cell apex star. The independently audited offline355-cell witness
shows that one compound multi-apex replacement can clear the frozen constraints.
The new native operator generates four leaf proposals from the actual finite wall
patch direction and local metric spacing, privately partitions complete stars with
orientation/quality implications and an integer-capacity minimum cut, and validates
only the complete replacement against geometry/height/full composed-field gates.

The engine reuses its existing edge-plus-vertex dependency lookup, bounded donor
import, complete-star reservation, frozen-field identity, collective work admission
and atomic publication. Six new node IDs fit the existing reserved range; imported
cells stay<=64 and private replacements<=128. Four bounded metric-scaled proposals
and three private cuts each replace a shared height apex without changing wall
triangulation. The cut uses uint64 capacities, finite-sum-derived hard costs,
explicit overflow/work checks and post-cut hard-constraint validation. Geometry and
final field checks remain the acceptance authority, not cut cost or heuristic shape.

`patch.dat` contains the actual339-cell terminal state plus the original24-cell
coarse sensor donors. The hidden C++ replay performs the actual-query BL query,
complete native reconstruction, full composed metric/height/overlap checks on the
whole mesh and atomic rejection controls. The existing resumable runner adds a
`height` mode; exact runner/source/binary/config/input pins remain mandatory.

Run after the current build completes:
`python3 integration_evidence/native_3d_bl_v1/annulus_coordinated_replay_v1/run_native.py v3 height`
Then independently audit:
`python3 integration_evidence/native_3d_bl_v1/native_height_star_v1/audit.py v3`.
The unchanged native annular lifecycle and affected MPI/transfer/restart controls
remain required even if this operator replay succeeds.

`generic_seeds.log` checks the local metric spacing rule on the existing offline
partition kernel: factors1/1.25 clear frozen shape and size,1.5/1.75 fail geometric
visibility. This does not qualify native geometry or transfer. Build_v1 stopped on
an aggregate initializer compile error; its source pins and log are retained.
Build_v2 completed (session50386 exit0). Native serial_v1 rejects at the initial
cut in all four proposals, with old placement controls passing. An independent
long-double reproduction finds two wall feet outside by~5.6e-17 for the first
two scales; the larger scales have visibility restrictions. This does not prove
field incompatibility. A consistent binary64 chart-offset correction and bounded
face-specific diagnostics are building in session67346/build_v3. Source, field,
height and geometry tolerances remain unchanged. Preserve serial_v1; use v2 next. The optional Opus5.5/medium review timed out240s without a final
report (session21708 exit124); it provides no review/validation evidence.

## Replay fixture correction before construction inference

Both serial_v1/v2 selected physical skin by ordered tuple equality. One cyclically
reordered triangle54519,8,13647 was omitted, leaving7/8wallfaces and introducing a
false perimeter diagonal. Their negatives do **not** establish a native operator
failure with complete metadata. Production GeometryPatch retains every imported
present physical face. `fixture_admission_error.json` preserves the exact defect.
The shared test OnSkin helper now matches face identity, preserves orientation for
normal geometry validation, and the new replay asserts8faces plus cyclic input
retention. Only its test object is rebuilding (39034/build_v4); use serial_v3 next.
The binary64 shared-offset change and exact-rational reproduction remain separate
from this test defect. No metric, height, geometry or tolerance gate was weakened.

## Complete native frozen replay and first full attempt

serial_v3 passes11cases/81755assertions,3private binary cuts/52replacement cells.
The complete355cell/107node native-produced mesh independently passes exact volume,
finite face/hole coverage, topology,7818overlap pairs, height0 and frozen field
q=.2033345/J=.0512845/Lmax1.7955165. Reference complexity20.00019535;32/64-order
edge agreement4.44e-16 is empirical. Centroid-to-apex tilt3.03/71.74degrees measures
one alignment property, not a complete layer-stack or viscous-accuracy certificate.

The first independent audit finished its checks but failed formatting a relative
provenance path. Its log is preserved. The correction resolves paths and saves a
pinned complete geometry stage before optional reporting, allowing verified reuse.
`audit_v3_corrected.log` and serial_v3/independent_full_mesh.json are complete.

annulus_v8_n1 now completes refinement355cells/861work/91edits but the event rejects
in coarsening at340privatecells. Frozen profile SHA matchesv7 exactly. The shared
collapse prescreen can call strict SurfaceError on a tentative triangle bridging
a finite hole: valid vertices alone do not admit that triangle. The saved working
skin passes independent exact authority/coverage/area checks (136faces/10groups),
and remains a REJECTED private mesh without transferred CFD state.
The generic ContainsFace precheck and a matching hole-bridge control are building
in10356/build_v5; full lifecycle not qualified yet. Existing stage clocks are
preserved in construction_progress.json; different outcomes prove no event speedup.

Upcoming existing harness commands (after the finite-hole unit gate):
`python3 integration_evidence/native_3d_bl_v1/planar_symmetry_reference_v1/run.py annulus 1 v9`
`python3 integration_evidence/native_3d_bl_v1/planar_symmetry_reference_v1/run.py annulus 2 v9`
`python3 integration_evidence/native_3d_bl_v1/planar_symmetry_reference_v1/run.py annulus 4 v9`
`python3 integration_evidence/native_3d_bl_v1/planar_symmetry_reference_v1/run.py annulus 4 v9 all`
The last case explicitly selects N4/M4 and a distinct annulus_all_v9_n4 folder;
ordinary MPI4 remains N4/M2. Worker flags are pinned in provenance. Do not launch
these concurrently or equate their availability with passing runtime evidence.
