# Native coordinated first-height ownership: connected implementation

Status: implemented, BUILD/VALIDATION PENDING. Full Goal3/general symmetry remain
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
`python3 integration_evidence/native_3d_bl_v1/annulus_coordinated_replay_v1/run_native.py v1 height`
Then independently audit:
`python3 integration_evidence/native_3d_bl_v1/native_height_star_v1/audit.py v1`.
The unchanged native annular lifecycle and affected MPI/transfer/restart controls
remain required even if this operator replay succeeds.

`generic_seeds.log` checks the local metric spacing rule on the existing offline
partition kernel: factors1/1.25 clear frozen shape and size,1.5/1.75 fail geometric
visibility. This does not qualify native geometry or transfer. Build_v1 stopped on
an aggregate initializer compile error; its source pins and log are retained.
Build_v2 is the corrected continuation (live session50386), not yet a passing
implementation. The optional Opus5.5/medium review timed out240s without a final
report (session21708 exit124); it provides no review/validation evidence.
