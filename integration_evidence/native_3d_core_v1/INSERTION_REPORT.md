# Native 3D bounded cavity insertion — 2026-10-11

Repository `/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated`, branch
`codex/native-3d-core`, predecessor `16029e00cba59c54a05c71eb320c4833845762a2`.
Implementation/evidence publication pending. Full Goal 1 remains active.

## Change

Exact edge bisection can be impossible for stored binary64 endpoints. Refine
now tries bounded complete-edge-star insertion if ordinary geometric splitting
rejects. The representable midpoint need not lie exactly on the old edge.
Existing positive coning and fixed oriented skin certification preserve interior
cavities. On a surface seam, exactly two triangles must share one canonical
planar facet: replace their diagonal fan together with the adjacent volume.
Every new oriented planar child must lie in the finite original facet, the new
point must see every retained cavity face, and the complete replacement must
pass the existing prescribed-interface/topology checks. Positive planar children
keep the original oriented outer border; exact coplanarity closes the volume.

Immutable facet/marker junctions and incomplete stars reject. New point/cell
identities, 64-cell input/128-cell replacement bounds, dependency admission,
private/remote original-P1 sampling, metric validation and atomic publication
use existing infrastructure. Arbitrary replacement routing follows the existing
reconstruction path instead of assuming two children per parent. The inserted
counter distinguishes successful insertion from ordinary splits/moves/swaps.
No geometry tolerance, frozen target, quality/sliver/length guard, default
512-round limit or 2D/MMG/BL/fade/noise policy was changed.

## Evidence

`INSERTION_CHECKPOINT_VALIDATION.json` checks receipt/artifact integrity,
identical production archive/thin-member identities, configured regression
coverage and 884 closed exclusive/nested phase records in
`insertion_rank_phases.csv`. The latest full configured 68-object test driver
passes MPI1/2/4 insertion, automatic repair, budget, default rejection,
native2D and MMG controls. Direct insertion tests cover unit and 1e-8-thick
cavities, off-edge interior and coupled planar proposals, incomplete dependencies,
marker junctions, off-plane/outside points, ID failure and unchanged rejected
output. These are C++ checks, with independent lifecycle audits below.

`insertion_auto_audit_v1` independently validates 324 automatic repair/refine/
coarsen frames and nine missing/tampered-source/final-metric controls. Those
repair fixtures do not exercise accepted insertion; their scope is preservation
of the existing automatic repair paths. Actual insertion is exercised by the
larger replay and aligned n3 lifecycle cases.

Nine actual manufactured driver cases independently qualify three complete
events each, including worker return, ReplaceMesh, current/BDF2 state transfer,
original sensor transport, immutable geometry/marker/embedding/volume checks,
frozen residuals and actual SU2/CGNS output/readback. No CFD iterations were run.

| Folder | N/M | Accepted tetrahedra at three events |
|---|---|---|
| insertion_driver_n1_v1 | 1/1 | 88 / 36 / 94 |
| insertion_driver_n2_m1_v1 | 2/1 | 88 / 36 / 95 |
| insertion_driver_n2_m2_v1 | 2/2 | 88 / 36 / 95 |
| insertion_driver_n4_m1_v1 | 4/1 | 88 / 8 / 70 |
| insertion_driver_n4_m2_v1 | 4/2 | 88 / 9 / 69 |
| insertion_driver_n4_m4_v1 | 4/4 | 77 / 12 / 70 |
| insertion_aligned_n3_n1_r1024_v1 | 1/1 | 288 / 50 / 312 |
| insertion_aligned_n3_n4_m4_v1 | 4/4 | 288 / 61 / 308 |
| insertion_aligned_n3_n4_m2_v1 | 4/2 | 288 / 47 / 281 |

The first six original n2 controls have all 66 geometry/source/state files
byte-identical to the coordination predecessor. The n3 fixture uses a separate
grid-aligned marker line, retaining the historical incompatible-junction input
as an adversary. Aligned n3 passes with the default factory on N4/M4 and N4/M2.
The serial n3 row uses an explicit test-only 1024-round adapter; it does not
qualify the production default. Across qualified cases qmin is 0.2142575612,
Jmin 0.06217637445, Lmax 1.799787275, state error <=5.80681e-16 and transported
sensor tensor error zero. Frozen thresholds remain q>=0.20/J>=0.05/L<=1.8.

Inspectable accepted grids: `insertion_aligned_n3_n4_m2_v1/envelope_s0.vtu`
through `envelope_s2.vtu`, with actual SU2/event reference sidecars and final
CGNS alongside. Manufactured affine histories are included in the VTUs.

## Preserved failures and limits

`insertion_regression_v1` retains an initial test-only Catch2 assertion syntax
compile failure; corrected v2 and latest v3 pass. No numerical guard was changed.

`insertion_aligned_n3_n1_v1` accepts 162->288 tetrahedra at the first event,
then rejects the second coarsening at 512 rounds (59 accepted/453 rejected).
Accepted mesh/state/reference preservation checks pass before the completion
assertion fails. `insertion_aligned_n3_n1_r1024_v1` reaches that same coarsening
target after 619 rounds (66 accepted/553 rejected). All seven common input,
first accepted mesh/state/sidecar and second-input files are byte-identical;
the first default event is thereby independently qualified. The default
three-event serial case still fails. Raising the production limit is not a fix.

Original n3 `insertion_replay_original_n3_v1` is an independently qualified
rejection: 175 rounds, 57 private accepted edits (42 insertions/15 splits),
118 rejected, 420 tetrahedra/121 points, volume 1, qmin 0.5546890195 and
Jmin 0.334496804. Six oversized edges touch twelve cells; Lmax 2.229036246.
Original immutable geometry and the demonstrated incompatible marker junction
remain. Accepted CFD mesh/state/reference/source stay unchanged. Inspect
`replay_rejected.su2` and `independent_audit.json` for residual locations.
Private progress does not establish a completed adaptation.

## Performance and next work

Per-event parent/remesh maxima, max/mean, useful accepted edits per parent second,
requested native bounds and lifetime RSS are in the checkpoint manifest. Parent
includes manufactured metric preparation, observation, remeshing and replacement;
requested output and independent Python audits are separate. Requested bounds
are not allocator/RSS proofs. Maximum retained native bound is 8 MiB; qualified
process lifetime RSS reaches 32,309,248 bytes.

Aligned serial diagnostic parent times are 1.92336/5.12843/1.80597 s; useful
rates 51.47/12.87/70.88 edits/s. Default N4/M2 parents are
1.69834/4.59551/2.12345 s; N4/M4 2.29380/7.09842/4.09791 s. These are single
contended pilots with different resulting meshes, not fixed-work speedups or
scaling evidence. Original rejected replay engine wall is 1.25382 s with metric
0.47209 and reconstruction 0.24478 s; nested edge tracing 0.87680 s overlaps
exclusive phases and must not be added. Rejected events deliver no accepted CFD
work, regardless of private progress or rejection speed.

The serial coarsening rejection count motivates pruning provably impossible
boundary collapses before full-star reconstruction, preserving invalid-field
repair fallbacks and strict completion. A successful collective vote also
currently clears local geometric rejection text; this separate diagnostic bug
explains the blank last-edge reason and should be fixed with rejection checks.
Neither change is implemented in this checkpoint.

Full MPI wait/traffic, allocator/ParMETIS/CFD memory calibration, instrumentation
overhead, repeated timings/scaling, stronger/direction-changing metrics,
thin/wedge/restart/larger size limits and closed SGE campaigns remain open.
Raw geometry reference admission currently caps 256 input triangles. No curved,
robust BL generation, full unsteady 3D or CFD affordability claim is made.
Foreign work, AdapNoExt, submodules and ClusterResults remain untouched.
