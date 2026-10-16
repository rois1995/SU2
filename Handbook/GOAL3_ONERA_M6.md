# ONERA M6 coarse RANS adaptation

User-requested practical native3D wing test, 2026-10-16. Full Goal3 stays open.

Case root: `/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated/integration_evidence/native_3d_bl_v1/oneram6_coarse_v1`.
Original mesh: `/media/rausa/4TB/Verification/OneraM6/Meshes/Coarse.su2`; local copied input has SHA256 `8c839170eb5e359f7de6a41434aa6a9c2d620053f37de6d9360bd04d2a63088b`.
303359 positive tetrahedra,61330 points; WING3D38800, SYMMETRY8094, FARFIELD2060 triangular faces.
Median wall-face tetrahedron altitude0.0049863; minimum6.67196e-5. This is a coarse
non-layered starting mesh. Original coordinates and physical symmetry retained.

RANS/SA, Mach0.8395, AOA3.06, Re11.72e6/reference length0.64607;120 initial
iterations and one requested native adaptation, with10 follow-up iterations.
MACH/PRESSURE quadratic sensors, noise0, target complexity100000, bounds1e-6..100,
AR1e6, HGRAD1.3, original faceted authority and surface adaptation. Requested
BL first altitude2e-6/growth1.2/thickness0.01. Four CFD/working ranks, conservative
transfer, automatic size-scaled work bound and300s native-entry deadline,1200s
whole-process ceiling. One owned heavy job, nice19/threads1. No scaling or converged
viscous accuracy claim follows; current qualified prism/cylinder construction is
not a qualified exterior-wing constructor. No MMG substitution or CAD claim.

## First attempt: Hessian workspace failure

All120 initial flow iterations completed (last density residual log10 -2.038186,
CL0.157317/CD0.053009; unconverged). The subsequent metric computation aborted
in `computeGradientsLeastSquares.hpp:300`, reached from `CSolver::SetHessian_Adapt`.
Exit134, no native remesh entered and no adapted mesh generated.

The QLS recovery always computes a WLS fallback first. CFlowVariable allocated its
Rmatrix for primal least squares or Hessian WLS, omitting Hessian QLS. With both
primal gradients GREEN_GAUSS this is an empty array. The shared allocation now also
includes QLS. The existing signed-curvature/fallback regression explicitly uses
both Green–Gauss primal gradients and checks that primal LS was not requested.
This changes storage only; no sensor, fit, geometry, height or acceptance policy changes.

The first attempt's configuration, source/build/binary pins, dirty diff, mesh audit,
history, full log and symbolized diagnosis are preserved. Because metric computation
precedes final solution output, this crash produced no saved flow/restart. Do not
label the copied input an adapted or rejected proposal. The serial rebuild and focused check pass1 case/32260 assertions.

## Matching retry: thin BL complexity failure

`retry_workspace_fix` uses bit-identical input/configuration and completes120 flow
iterations. All122660 owned point/sensor QLS fits finish with zero fallbacks,
Hessian maximum rank time1.42196s; large residuals are retained with noise0.
The subsequent geometric complexity integration exhausts the65536-point bound
on original global cell13 (exit1), before any remesher entry or candidate.
This is a real unresolved thin-volume integration gap; it establishes neither
mesh-construction failure nor successful adaptation. No limits/targets were relaxed.

Original cell13 (14th input element) has points23981,23978,4352,31725,
centroid(1.1049647955,1.1981531812,.00539510694), upper outer wing near the tip.
One WING3D face4352/23981/23978 has current altitude.00224586883.
`failure_cell_13.vtu` saves its46-cell input neighborhood with original IDs and a
FailureCell flag; it is not a remeshing proposal.

Because metric errors prevented final flow output, one matching flow-only export
was saved in `flow_checkpoint`. All120 iterations of every shared printed history
column exactly match both failed attempts. Float64 restart coordinates exactly
match the mesh; all fields finite, density/ideal-gas pressure positive. Open
`flow_checkpoint/flow.vtu` and `surface_flow.vtu`; restart_flow.dat preserves the
starting solution. This is not a converged viscous or aerodynamic accuracy result.

All jobs terminal. No further M6 adaptation or profiling task is queued. The next
connected issue is thin-BL volume integration on actual wing facets; exterior-wing
mesh construction, acceptance and transfer remain untested. Full Goal3 stays open.
See `oneram6_coarse_v1/README.md`, result.json/artifacts.json and source/binary pins.
