Current status supersedes historical live-job notes below: reference stages are
terminal; follow [uncertainty experiment](GOAL3_UNCERTAINTY_COMPLEXITY.md) for the
current policy, construction failure and remaining requirements.

# Goal3 — complete M6 frozen field and integration reassessment

Full Goal3 ACTIVE/incomplete. Working branch codex/native-3d-bl. All physical
metric functions remain byte-identical to977691bc4e/412230. No accepted M6
adapted grid, remesher entry, transfer or affordability proof.

## Complete actual first-trial checkpoint

`integration_evidence/native_3d_bl_v1/oneram6_frozen_field_v1` contains a passive
4-rank checkpoint BEFORE the first IntegrationRule3D call. One initial CFD step
from the existing120-step flow; config/mesh/reference/restart byte-identical to
oneram6_frozen_v2. Producer restored and fresh CFD/test binaries built. Capture
finishes in47s including debuggers; this is not a performance measurement.
Read-only GDB memory reads, no inferior calls, all-rank ready barrier then quit.
No complexity quadrature or remeshing. Build/capture terminal exit0.

Decoder proves complete unique owner coverage of61330 points/303359 cells,
original coordinates bitwise, trial1/core.056676963474793819/scale-.47810758149935007.
All3 earlier actual failing-cell raw9 tensors/core/scale match exactly. Source
cell host is the owner of its smallest global node, matching OwnedSimplices.
Every hosted tetrahedron has all4 nodes in that rank's captured field.

Halo/owner matrices differ up to2.0231e-16 relative because metric exchange packs
upper6 then reconstructs symmetric halo values; owned spectral recomposition can
have roundoff asymmetry. Preserve BOTH raw forms. frozen_trial.npz contains each
rank's IDs/tensors plus node/cell owners and the owner field; never substitute
owner tensors for a hosted cell's halo if claiming exact replay. No field repair.
Raw buffers/metadata/hashes remain for audit. Decoder excludes irrelevant test
binary/source context pins, since capture executed only SU2_CFD. Repeated packing
checks arrays and reuses immutable NPZ bytes. Creation-time producer provenance
is not proof of a future changed runtime producer.

Small frozen_cell88|157|329_sensor_input.vtu files expose original neighborhoods,
raw owner tensors, size eigenvalues and sensor-only mass estimates. These are
INPUT sensor states, not composed BL/final normalized targets or adapted grids.

## Sensor-only whole-mesh scalar audit

Independent NumPy P1 integration uses the exact hosting-rank raw tensors. For
sensor-only eigensolver density, tred2 reads the lower triangle; diagnostic copies
use that symmetric matrix without changing archived inputs. Positive18-point
Duffy quadrature checks all barycentric moments through degree3, integrating the
P1 determinant polynomial exactly in exact arithmetic. Concavity of det^(1/3)
and of sqrt yields analytical lower/upper mass bounds; floating evaluations are
not outward-rounded machine certificates and exclude BL composition.

- Volume2088461.81061555; sensor lower142421.03244, Q18 estimate152994.20094,
  upper154272.45539. Q4 differs4.0379e-4 relative, so Q4 is not sufficient proof.
- Sensor mass by rank134518.81/7762.85/6040.97/4671.56. This is mass, not work.
- Whole-wing AABB plus thickness safely excludes90253 tetrahedra from this faceted
  BL;213106 remain possible, not proven actual support. Their total volume.2241754.
- No raw constant-sensor tetrahedra: a byte-equality constant fast path would not
  help this actual first-trial field.

This first-trial sensor mass already exceeds requested100000. It is not the final
normalized metric, an irreducible BL floor, nor proof that100000 is infeasible.
Files sensor_mass_audit.json/npz retain per-cell values/support and derivation.

## Independent full-density reference facility

The existing hidden NativeFacetedVolumeReplay3D test has a bounded point-query
mode via SU2_NATIVE_BL_QUERY_POINTS (test-only, no production config). Input is
4 binary64 requested barycentrics per row; output point_values.f64 has6 columns:
actual original-source barycentrics4, target density, active-wall distance (-1
outside). Position uses long-double affine evaluation; exact3D Barycentric
checks containment, then unchanged original-P1/ApplySample/eigensolver evaluates
the same physical law. Maximum1e6 rows; invalid size/weights/nonfinite rejected.

`oneram6_density_reference_v1/reference.py` uses independent positive tensor-Gauss
Duffy grids, no native EdgeBreakpoints/adaptive controller. It retains complete
per4-slab scalar/moment/provenance receipts and deletes regenerable query grids.
Cache checks pin source/build/binary/fixture/NumPy/parameters/helper identities;
flock prevents duplicate stage jobs. This is independent INTEGRATION, not an
independent physical tensor/geometry implementation or rigorous error enclosure.

Interface checks pass64 saved full-unit22 samples (max9.508e-13 relative),64
normal13 samples (exact), and invalid byte size/sum/NaN. Initial18 stages complete:
orders24/48/72 with apex0/3 on each actual88/157/329. Cell157 order changes
8.77e-7/2.55e-7 and chart gap4.99e-7: useful numerical agreement.329 one chart
still changes5.91e-4;88 one changes1.23e-4. Reuse157; refine only329/88 to96/128.
Current owned reference batch session11887; one core/nice19, no CFD/MPI/model job.

## Next structural decision

No further local queue/chart/budget fallback variants. Full frozen data enables
whole-complexity allocation experiments, but scale-solver1e-6 and quadrature1e-4
indicators must be treated explicitly; tiny unsampled BL mass needs conservative
handling, not permission to ignore it. No new stopping policy is implemented.

Before production changes, diagnose the actual geometric branches behind saved
normal jumps. A candidate shared faceted representation is the lower envelope of
valid face/edge/vertex squared-distance quadratics along a chord, including GLOBAL
feature competition. It could resolve real transitions and avoid full metric
Evaluate calls merely to admit distance-band roots. Per-triangle Voronoi planes
alone are insufficient. This is a design to test offline, not an implemented fix.
Shared actual-query/finer/fade/noise0/geometry/height/shape/full-length constraints
and every full Goal3 gate remain. Current accepted geometry classes are still
limited; resolving integration alone does not establish general wing construction.
