# Experimental native 2D adaptation

`ADAP_REMESHER= NATIVE_CAVITY` selects the MMG-free cavity backend. MMG remains
the default when this option is omitted. Native generation uses the actual frozen
SU2 nodal metric, adapts physical boundary sampling and its adjacent BL triangles
together, then reuses the existing distributed mesh replacement and solution
transfer. It builds with `-Denable-mmg=false`.

The backend is experimental. Controlled MPI integration checks and three-cycle
realistic-airfoil runtime checks pass at1/2/4 ranks, with all nine independent
airfoil audits passing. Eight-cycle opposing-wall checks with both transfers and
all48 independent saved-mesh audits also pass. Default-MMG production adaptation,
the final38-case native matrix, scoped allocation checks and native NS/SA/SST plus
CGNS controls in an MMG-enabled build pass at1/2/4 ranks. The ordinary four-rank
airfoil executable completes all three adaptations and exits successfully.
All168 final output audits pass across the no-MMG and MMG-enabled native matrices,
including configured SU2/CGNS geometry comparisons and frozen-target checks.
These checks do not establish general production robustness or a
performance advantage.

## Supported scope and verification

| Input or operation | Current behavior | Runtime evidence |
| --- | --- | --- |
| Static single-zone 2D triangular compressible Euler | Admitted in a primal double build | Controlled adaptation/transfer/output at MPI1/2/4 |
| Static single-zone 2D triangular compressible Navier–Stokes | Admitted; configured wall height reconstructed | Eight changing-height one-/opposing-wall cycles with both transfers, actual sensor/BL targets and three-cycle realistic-airfoil runtime at MPI1/2/4; nine airfoil and48 opposing-wall independent audits pass |
| Steady compressible RANS | Admitted by the driver and native checks | Actual sensor/BL targets, conservative transfer, resumed SA/SST updates and full SU2/CGNS solution/reference restart pass controlled MPI1/2/4 checks |
| SU2 and CGNS input/output | Existing readers and writers reused; CGNS requires its build dependency | Controlled mesh and viscous solution/reference round trips at MPI1/2/4 |
| Barycentric and conservative transfer | Existing implementations reused | Both exercised on repeated controlled BL adaptation; conservative transfer and resumed CFD pass all three realistic-airfoil cycles at MPI1/2/4 |
| Fixed physical sampling | `ADAP_SURFACE= NO` retains physical vertices/edges | Paired MPI controls demonstrate both a repairable fixed case and an incompatible retained-edge target |
| Adaptive physical sampling | `ADAP_SURFACE= YES` permits reference-constrained split/remove/redistribution | Controlled MPI runs retain markers, components, feature vertices and first altitude |
| 3D, mixed cells, periodic/paired interfaces, moving/deforming meshes, multizone, FEM, time-domain, adjoint/derivative builds | Explicitly rejected | Guard implementation; selected collective failure subprocess controls. This is not exhaustive runtime coverage of every rejection. |
| Smooth CAD/reference projection | No native fitted/CAD reference input implemented | Original polyline is the production reference; analytic smooth-reference research controls do not establish native CAD integration |

The native path retains the **original mesh polyline**, component association,
marker junctions and detected features across adaptation calls. It cannot recover
unknown smooth geometry from a faceted input. Changing the mesh boundary samples
does not replace that original reference.

## Configuration example

Add these options to an otherwise valid supported solver configuration; use
the actual wall marker names and length scales for that case:

```ini
COMPUTE_METRIC= YES
ADAP_REMESHER= NATIVE_CAVITY
ADAP_SURFACE= YES
ADAP_LOOP= YES
ADAP_SIZES= (20, 35, 12)
ADAP_SUBITER= (1)
ADAP_FLOW_ITER= (2)
ADAP_TRANSFER= CONSERVATIVE
ADAP_HMIN= 0.004
ADAP_HMAX= 0.04
ADAP_HAUSD= 1e-8
ADAP_BL_MARKER= (lower_a, lower_b)
ADAP_BL_FIRST_HEIGHT= (0.004)
ADAP_BL_GROWTH= (1.2)
ADAP_BL_THICKNESS= (0.016)
WRT_ADAP_MESH= YES
MESH_OUT_FORMAT= CGNS
MESH_OUT_FILENAME= native_adapted
```

`MESH_OUT_FORMAT= SU2` selects SU2 output instead; CGNS needs an enabled CGNS
build. `WRT_ADAP_MESH= NO` disables configured mesh output while allowing
adaptation. The parallel writer already in this branch handles CGNS output.

Adapted mesh output includes a `.native_ref` sidecar containing the original
geometry and accepted associations. Retain it alongside the mesh for native
adapted restart, together with the ordinary solution restart files. A native
restart missing this sidecar fails explicitly instead of silently rebasing its
geometry reference.

## Acceptance and limits

Each native call reports its fixed contracts: metric quality at least0.18,
Simpson metric edge length at most1.8, configured reference tolerance and
requested wall-apex altitude relative error at most1e-8. The metric remains
frozen for that call. The backend does not relax these limits or silently switch
to MMG when construction fails. An incomplete or invalid candidate is rejected
before replacing the accepted CFD mesh and solution.

First altitude means the adjacent triangle's apex distance from its actual wall
base. It does not guarantee y+, cell-centre spacing, exact multilayer growth or
prisms/quads. The full BL metric still guides the surrounding volume triangles.

For `NATIVE_CAVITY`, metric construction prescribes the configured wall-normal
size throughout the full BL band, removes normal-tangent coupling there, and
blends back to the sensor metric through the outer fade. Adapted surfaces retain
tangential sensor refinement; fixed surfaces retain the near-wall tangential
floor. Overlapping bands use the nearest active wall. Complexity scaling includes
these constraints: a target below the attainable minimum is reported without
relaxing the wall heights. Wall resolution can therefore impose a substantial
minimum complexity even when the outer metric reaches `ADAP_HMAX`.

Adapted-wall tangential sizes now use symmetric chords of the retained native
polyline reference, capped by the same `ADAP_HAUSD` deviation check used by the
backend and stopped at reference features. Existing wall-edge lengths do not
set this cap. A conflict with the tangential minimum (`max(ADAP_HMIN, 2 h0)`)
is reported; the backend's geometry acceptance remains authoritative.

Native metrics also undergo full tensor gradation with `ADAP_HGRAD`, using
synchronous halo exchanges and neighbors ordered by global point ID. Each
complexity trial includes gradation, bounds and prescribed full-band BL normals.
The outer BL fade is applied once before gradation. The report distinguishes a
fixed point from the sweep limit and reports transported-metric violations;
hard BL/bound constraints can prevent unrestricted gradation.

Quadratic Hessian recovery reports weighted fit residuals and QR pivot ratios.
Stencils with fewer than two residual degrees of freedom, failed fits or relative
residuals above 0.05 seek a second ring. An overdetermined first-ring fit is kept
when it has the smaller residual; resolved curvature is retained and persistent
large residuals are reported. Second-ring owner neighborhoods are requested only
where needed. Active native BL faces are exchanged by padded rank bounding boxes,
including support for vertex normals; the backend's immutable original reference
is still replicated.

WLS inverse-distance-squared weights and its geometry matrix are recomputed on
each gradient-kernel call. `Rmatrix` is a work array, not a persistent cache.
Variables supplied together share that work, but the WLS Hessian path invokes
the kernel again for each sensor. A persistent cache would need invalidation for
mesh motion/adaptation and correct periodic/AD dependencies; it is not added here.

Transactions import complete dependencies, reserve affected nodes, validate
versions/interfaces/geometry, and prepare publication storage before the final
collective vote. The default2MiB per-rank dependency admission applies to bounded
transaction work, not the accepted mesh, original donor, MPI implementation or
whole-process RSS. Controlled allocation probes are measurements, not a
universal memory proof. Original boundary storage and existing reader master
boundary rows are replicated; volume reconstruction avoids a complete gather.
Frozen-field discovery uses at most128 padded triangle regions per caller and
imports at most256 intersecting immutable donor cells. Discovery metadata is
counted separately from the admitted target payload; these caps do not bound
whole-process RSS. Candidate cache storage is fixed at64 local entries; parity
and allocation controls passMPI1/2/4. Realistic-airfoil performance remains
unverified for this cache.
Current candidate scans and all-rank metadata impose scaling limits.

## Reproduction

With MPI, CGNS and the usual SU2 build dependencies available, configure a
separate primal build from the repository root:

```sh
python3 externals/meson/meson.py setup build-native -Dwith-mpi=enabled -Denable-cgns=true -Denable-tecio=false -Denable-tests=true -Denable-mmg=false -Dbuildtype=debugoptimized -Db_ndebug=false
ninja -C build-native -j2 SU2_CFD/src/SU2_CFD UnitTests/test_driver UnitTests/test_memory
```

The portable example [QuickStart/native_NACA0012.cfg](QuickStart/native_NACA0012.cfg)
uses the same physical/metric request as the passing airfoil fixture. Run from
`QuickStart` so its relative mesh path resolves:

```sh
cd QuickStart
env OMP_NUM_THREADS=1 OPENBLAS_NUM_THREADS=1 mpiexec -n 4 ../build-native/SU2_CFD/src/SU2_CFD native_NACA0012.cfg
```

This ordinary executable run passes at MPI4 in the recorded no-MMG build,
including all three accepted remeshes, replacements, configured mesh/reference
outputs and final successful exit. Separate MPI1/2/4 driver fixtures verify
conservative transfer, finite admissible owned/halo fields and resumed CFD updates;
all nine independently audited airfoil meshes meet the declared contracts.

For repeated BL, viscous/turbulence and output/restart controls, run sequentially
from the repository root:

```sh
for ranks in 1 2 4; do
  env OMP_NUM_THREADS=1 OPENBLAS_NUM_THREADS=1 mpiexec -n "$ranks" build-native/UnitTests/test_driver '[NativeRemesher],[NativeBL2D],[NativeProducedBL2D],[NativeCGNS2D]'
  env OMP_NUM_THREADS=1 OPENBLAS_NUM_THREADS=1 mpiexec -n "$ranks" build-native/UnitTests/test_memory '[NativeMemory]'
done
```

Focused native Catch tags include `[NativeReader]`, `[NativeImport2D]`,
`[NativeEngine2D]`, `[NativeField2D]`, `[NativeMesh2D]`, `[NativeBL2D]`,
`[NativeProducedBL2D]`, `[NativeFixedAdaptive2D]`, `[NativeCGNS2D]` and
`[NativeMemory]` (the last uses `test_memory`). Run each executable sequentially
with `mpiexec -n 1`, then2, then4. Hidden `[NativeAirfoil2D]` needs
`SU2_NATIVE_AIRFOIL_CONFIG`; hidden `[MMGDefault2D]` needs both MMG and CGNS.
The actual MMG-default production control passes at1/2/4 ranks, including two
mesh replacements, resumed uniform flow and configured CGNS output.
For an MMG-enabled comparison build, use `-Denable-mmg=true` and the existing
`mmg_root`/`mmg_scotch_root` options where needed. The local installed static MMG
archive needs `-Dmmg_scotch_root=/home/rausa/Software/scotch`; this is an environment
dependency, not a new requirement of the native backend.

In the research checkout, `BL_NATIVE_INTEGRATION_WORK/` holds runners,
independent target/topology/reference audits, exact executable archives and
per-run commands, source hashes, configurations and exit status.
`HANDOFF_Codex.md` records the current sole job and remaining goal gates.
