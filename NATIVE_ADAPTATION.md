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
| Static 2D time-domain Euler/RANS, `WINDOW_AVERAGE`, BDF1/2 | Admitted on `codex/native-unsteady-performance`; existing current/history transfers reused | Euler actual MPI1/2/4, SA/SST coarse-to-BL controls, independent saved-field/mesh audits and window-boundary SU2/CGNS restart; see NATIVE_UNSTEADY_PERFORMANCE.md for exact coverage and cost limits |
| Native predicted/fixed-point time windows | Rejected until validated | No implementation or support claim |
| SU2 and CGNS input/output | Existing readers and writers reused; CGNS requires its build dependency | Controlled mesh and viscous solution/reference round trips at MPI1/2/4 |
| Barycentric and conservative transfer | Existing implementations reused | Both exercised on repeated controlled BL adaptation; conservative transfer and resumed CFD pass all three realistic-airfoil cycles at MPI1/2/4 |
| Fixed physical sampling | `ADAP_SURFACE= NO` retains physical vertices/edges | Paired MPI controls demonstrate both a repairable fixed case and an incompatible retained-edge target |
| Adaptive physical sampling | `ADAP_SURFACE= YES` permits reference-constrained split/remove/redistribution | Controlled MPI runs retain markers, components, feature vertices and first altitude |
| 3D, mixed cells, periodic/paired interfaces, moving/deforming meshes, multizone, FEM, adjoint/derivative builds | Explicitly rejected | Guard implementation; selected collective failure subprocess controls. This is not exhaustive runtime coverage of every rejection. |
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

For `NATIVE_CAVITY`, only the CFD sensor tensor is frozen on the original
volume connectivity. Every query first interpolates that sensor, then intersects
it with wall constraints evaluated against the retained original geometry.
This applies equally to new private points and imported MPI cavities. Finer
sensor demands and coupling are preserved. All active walls are intersected;
there is no experimental hard-normal reset or native tangential coarsening floor.
The original outer fade interpolates wall eigenvalues in log space toward the
frozen sensor core eigenvalue. Composed wall tensors are never interpolated
through coarse donor cells.

Each native complexity trial grades only the bounded sensor field, synchronizing
owner/halo values in global-ID neighbor order. On 2D BL cases, positive cell
quadrature integrates the same geometric composition at actual sample locations.
Geometric distance bands resolve the first height inside a coarse donor cell;
geometry samples are cached across scale trials. Final donor tensors remain
sensor-only. Targets below the attainable geometric BL complexity are reported
without weakening wall or sensor constraints.

Sensor transport and the composed field are audited separately. The report
counts directed-edge violations and distinguishes fixed-point changes from
actual transported-metric residuals. A graded sensor is not a certificate of
combined-field gradation, particularly inside coarse cells and through the
original outer fade. No composed nodal correction is spread into the donor.
Original polyline tangential lengths/corner treatment and native geometry gates
remain authoritative. The incoming reference-chord helper is retained for
experiments; its different sizing policy is not enabled by this integration.

Quadratic Hessian recovery reports weighted fit residuals and QR pivot ratios.
Stencils with fewer than two residual degrees of freedom, failed fits or relative
residuals above 0.05 seek a second ring. An overdetermined first-ring fit is kept
when it has the smaller residual; resolved curvature is retained and persistent
large residuals are reported. Second-ring owner neighborhoods are requested only
where needed. Standalone nodal BL geometry exchange uses bounded occupied query boxes with
incident-face support. Production native composition uses the complete retained
original reference, including private/remote queries; it remains replicated.

WLS reuses the geometric normal matrices and weights across sensors within one
adaptation call, where geometry and weighting match. Periodic, symmetry and AD
paths retain the established kernels. No persistent mesh cache is introduced.
`ADAP_HESSIAN_NOISE=0` remains the initial integration setting; a positive value
is experimental and failed the incoming frozen RAE MPI/gradation integration
check. Manufactured filtering tests do not supersede that failure.

The shared symmetric eigensolver normalizes subnormal Householder scales with
powers of two, avoiding overflowing reciprocals in fast-math builds.

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

## Unsteady checkpoints and performance

On `codex/native-unsteady-performance`, static2D triangular `WINDOW_AVERAGE`
uses the existing dual-time adaptation driver. Set `TIME_DOMAIN=YES`, positive
`TIME_STEP`, `TIME_MARCHING=DUAL_TIME_STEPPING-1ST_ORDER` or `-2ND_ORDER`,
`ADAP_FREQ`, and `ADAP_UNSTEADY_METRIC=WINDOW_AVERAGE`. Existing solver settings,
BL contracts and transfer choices still apply. Native3D remains unsupported.

Mesh `mesh_<first-step>.su2` or `.cgns` and its `.native_ref` pair with the
rewritten restart of step `first-step-1` (and `first-step-2` for BDF2). The donor
VTU of the preceding step stays on the previous mesh. When `WRT_ADAP_MESH=YES`,
requested result formats are also written at native window ends, even with sparse
ordinary output. Use `VOLUME_OUTPUT_PRECISION=DOUBLE` for independent strict
frozen-metric audits. Restart equivalence is demonstrated at complete adaptation
window boundaries; mid-window Hessian-accumulator persistence is not implemented.
A restart before the first adaptation needs its original reference checkpoint;
that checkpoint lifecycle is not validated here.

The measured cache improvement reduces one RAE coarse-to-BL native phase cost
by35.2% with byte-identical meshes and solutions. It does not establish affordable
adaptation cadence for arbitrary unsteady flows. See NATIVE_UNSTEADY_PERFORMANCE.md.
