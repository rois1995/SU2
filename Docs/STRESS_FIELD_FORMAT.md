# Saved stress fields, version 1

The `.suf` file is the authoritative, immutable scenario. The adjacent JSON file
records generation provenance and a readable audit. SU2 reads the `.suf`, checks
its payload hash, recomputes its norms, and hashes the whole file for restart and
MPI identity. The whole-file hash includes geometry, units and support. Changing
only the JSON does not change the solver field.

Coordinates, wall offsets and lengths are in metres. The frame is fixed Cartesian;
any rigid transformation must be applied to coordinates, tensors and geometry
before export. `U_ref` is in m/s. Stored coefficients are dimensionless. The
result is kinematic stress in m²/s². SU2 converts coordinates using its length
reference and divides the result by its velocity reference squared, once.

## Header and payload

The UTF-8 header has this exact order. Words in angle brackets below are values,
not literal input. Lines end with LF.

```text
SU2_STRESS_FIELD 1
SCENARIO <one token, at most 128 characters>
PURPOSE <SCENARIO or COUPLING_CONTROL>
FRAME CARTESIAN_METERS
DIM <2 or 3>
COUNTS <node count> <cell count>
SCALES <U_ref> <L_ref> <epsilon> <ell> <ell_w>
NORMS <max nodal coefficient norm> <normalized H1 norm> <area or volume> <maximum cell diameter>
GEOMETRY <see below>
SUPPORT <see below>
ENCODING <TEXT or BINARY_LE>
PAYLOAD_SHA256 <64 lowercase hexadecimal characters>
DATA
<payload>
```

`L_ref` records the field's physical reference length; it does not rescale already
physical coordinates. `ell` and `ell_w` are physical smoothing and wall lengths.
The norm domain is exactly the supplied simplex mesh; its measure is recomputed.

Each node has eight float64 values: `x y z c0 c1 c2 c3 c4`. Nodes are followed by
all cells, each containing three (2D) or four (3D) zero-based node indices.
`TEXT` uses whitespace-separated decimal values. `BINARY_LE` uses little-endian
IEEE float64 values and uint32 indices, without padding. Exact payload length is
checked. Triangles/tetrahedra must have positive orientation. Duplicate simplices,
nonfinite numbers, bad indices and invalid certificates fail. Overlapping cells
with different connectivity, nonmanifold geometry and solid intersections require
an offline mesh audit; this reader is not a general mesh validator.

The five orthonormal tensor basis members are:

- `T0 = diag(1,-1,0)/sqrt(2)`;
- `T1 = diag(1,1,-2)/sqrt(6)`;
- `T2`, `T3`, `T4` have the symmetric xy, xz, yz entries `1/sqrt(2)`.

In 2D, `z=c3=c4=0`, but the zz stress remains present. Norms use exact consistent
P1 mass and stiffness integrals. A `SCENARIO` must satisfy both bounds by epsilon.
`COUPLING_CONTROL` relaxes only these bounds; it never relaxes integrity or geometry
checks. The exporter never silently renormalizes supplied coefficients.

## Geometry and support

`GEOMETRY NONE` explicitly means no walls. `GEOMETRY PLANES N` is followed by N
lines `nx ny nz offset weight`. The fluid is on `n·x >= offset`; normals are unit
vectors. These are physical wall sheets, with constant nonnegative weights.
Two charts representing one sheet must divide its weight, not each contribute
one. Up to 32 planes are allowed. Local collars and CAD patches are unsupported.

`GEOMETRY CIRCLE cx cy 0 radius` is a separate 2D **offline-only** oracle provider.
SU2 rejects it in a coupled run. It does not qualify general curved geometry.

Support is `NONE`, `BOX lower_xyz upper_xyz width_xyz`, or (circle only)
`RADIAL start end`. Box/radial tapers use the C2 polynomial
`6t^5-15t^4+10t^3`, clamped to [0,1]. Planar box z entries are unused.
Solid-side queries fail. Exact wall points return zero within a fixed `1e-12 m`
tolerance. Outside the mesh, only explicitly zero support/wall queries are allowed;
a nonzero query fails rather than borrowing the nearest value.

At each physical query, locate a triangle/tetrahedron and interpolate the **raw**
coefficients. Then compute `b_j=weight_j*ell_w/d_j`,
`L=(I+sum(b_j*n_j*n_j^T))^-1`, `q=1/(1+sum(b_j))`, and
`E=U_ref²*chi*q²*dev(L*B*L)`. The implementation uses rank-one inverse updates.
It never interpolates pre-damped final stresses. Shared element boundaries use
the lowest containing cell index deterministically. There is no query cache.

The replicated prototype permits at most 100,000 nodes, 300,000 cells and 64 MiB
on disk. Its startup summary reports persistent memory per rank; parsing has
additional temporary storage. Large distributed fields need a different reader.

## Offline commands

Requires Python 3, NumPy and SciPy. No command here starts SU2 or accesses a cluster.

```bash
python3 SU2_PY/stress_field/generate_field.py --mesh mesh.npz --definition field.json --output field.suf
python3 SU2_PY/stress_field/export_field.py --input raw.npz --definition field.json --output field.suf
```

`mesh.npz` contains `nodes (N,2|3)` and integer `cells (M,3|4)`. Export input also
contains `raw_coefficients (N,5)`. The definition has `scenario`, `U_ref`, `L_ref`,
`epsilon`, `ell`, `ell_w`, `geometry`, `support`, and optional `purpose` (default
`SCENARIO`). For planes, `geometry.walls` contains `normal`, `offset`, and optional
`weight`. For box support use `lower`, `upper`, `width`, each with three values.

Generation also requires `seed: {"type":"SINE_PHYSICAL_V1",
"wave_vectors":[five vectors], "phases":[five values]}`. These specify continuous
physical-coordinate sine functions. The sparse P1 solve is `(M+ell²K)z=Mf` with
homogeneous Neumann conditions. The generator scales all components together to
meet both bounds and saves the result. It rejects periodic pairs; it does not
mesh the geometry. The user supplies a conforming fluid mesh and is responsible
for boundary classification, sheet identities and geometric error. The JSON
report labels `H>ell/3` as `NOT_RESOLVED`; tiny algebra fixtures intentionally
need not satisfy this study-resolution gate.

The supplied circular oracle is imported with `export_field.py --input
curved_field.npz --v07-circle-manifest curved_field_manifest.json --output
circle.suf`. Its NPZ hash must match its manifest.

Build the dependency-free C++ query probe and run the cross-language checks:

```bash
c++ -std=c++17 -O2 UnitTests/SU2_CFD/numerics/stress_field_probe.cpp -o /tmp/stress_field_probe
python3 SU2_PY/stress_field/test_field.py --probe /tmp/stress_field_probe --v07-root /path/to/SA_stress_uncertainty_v07
```

The probe reads physical `x y z` triples from stdin and writes
`E_xx E_yy E_zz E_xy E_xz E_yz`. Omitting the optional oracle path reports that
one test as skipped. It must not be described as a passed circular replay.
