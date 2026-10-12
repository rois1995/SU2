# Curved conservative lifecycle and solved Euler adaptation

Repository: /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated.
Branch: codex/native-3d-curved-local. Goal2 ACTIVE; full completion still unproven.

This milestone extends the actual curved driver path through repeated conservative
transfer, new/lost domains, CGNS restart, and actual solved three-dimensional Euler
pressure-Hessian targets. No production numerical/geometry/metric/transfer policy
changed. Source change is one existing lifecycle test file plus evidence/docs.
Production libraries and all archive members remain hash-identical to the previous
100-selected-case MPI1/2/4 integration regression; only its one test object changed.

Manufactured control: initial cube with two analytic bump marker components,
amplitudes -0.01 (growth) and +0.01 (shrink), ADAP_HAUSD0.003, scalar sensor targets
0.5I ->4I ->0.5I. Affine nonuniform conservative variables seed current/time_n,
and time_n1 is scaled0.96875. The driver loads initial authority from its real
sidecar, conservatively replaces the mesh three times and writes actual CGNS.
After the first event it writes both restart histories, tears down the driver,
reloads the CGNS mesh and restart, verifies all15current/history fields by physical
coordinates, then continues adaptation. Final actual SU2 andCGNS readback agrees
with the accepted mesh/markers. AllM=1..N on localMPI1/2/4 are exercised.
Serial trajectories: growth8->48->8cells; shrink8->48->10cells. Both new and lost
volume are nonzero in their respective first events (~0.0002083333333333).

Conservation is checked against the existing CLOSED/open-marker rule, not identical
faceted-domain volumes. Independent exact element volumes give median-dual nodal
volumes; the accepted nodal states are integrated against them and checked against
all10projected current/old-history targets. The ledger donor+fill_open-lost_open
agrees with those targets. Density and internal energy remain positive for all
three stored states. Exterior boundary-trace filling is not affine extrapolation;
no outside-domain affine-equality claim is made.

Solved control:48tetrahedra/27points, z=-0.01*x(1-x)*y(1-y) slip wall, other faces
farfield, Mach0.5, Roe withoutMUSCL, BDF2/time step0.001, fiveinner iterations per
step. Actual Preprocess/Run/Postprocess/Update advances the PDE, then the standard
ComputeMetric forms PRESSURE Hessians and a spatially varying anisotropic target
(complexity6, hmin0.3/hmax2, ARmax2, NOISE0). Two real native remesh/transfer events
are separated by actual CGNS-independent SU2/BDF restart and solver continuation;
another actual PDE step follows the last event. LocalN1/2/4 and availableM1/2/4
exercise M=N/M<N. Serial event meshes have37cells, with21then22points; the two
pressure-derived tensor fields differ, including nonzero off-diagonal components.
This is a solved-flow target rather than a prescribed sensor or saturated fixed
hmin=hmax control. It is a small subsonic lifecycle, not a converged/resolved shock,
force-accuracy or full physical unsteady qualification.

curved_lifecycle_v1/validation.json PASS:2focused cases per rank MPI1/2/4, complete
configured68-object driver/source/archive/member/binary pins. Independent audit
PASS: exact positive volume/manifold/conformity/no-interior-overlap; original-P1
priority and declared exterior edge coverage; arbitrary sensor tensor interpolation,
transported-metric residuals and exact-interval metric edge integration; q>=0.20,
J>=0.05, length<=1.8; sampled wall geometry; actual accepted-state conservative
integrals/new-lost accounting, positivity, real current+BDF restart, actual SU2/CGNS
mesh signatures and unchanged analytic reference sidecars. In serial, minimumq is
~0.438688 for solved events and maximum edge~1.792384; transported metric residual
is zero in the saved serial examples. All other partitions receive their own audits.

Inspect actual solved grids:
curved_lifecycle_v1/mpi1/solved_bump_m1_mesh_00001.su2 and_mesh_00002.su2;
actual manufactured CGNS grids:
curved_lifecycle_v1/mpi1/curved_lifecycle_grow_m1_mesh_00001.cgns and subsequent
steps, plus correspondingMPI2/4/M<N folders. Each event retains input/accepted mesh
and source JSON, actual donor/accepted states, projection ledger, cfg, reference,
restart files and logs. Corrected executable independent auditor is
curved_lifecycle_v1/audit_lifecycle_final.py; the initially prepared script with
one unmatched parenthesis remains immutable because the controller captured it.
No CFD/source/binary/acceptance gate changed to correct that auditor.

All four full Goal2 requirements remain pending. Runtime sphere/cylinder, fuller
mixed curved/planar and feature coverage and a suitably resolved nonuniform/shock
case are still missing. Curved commits remain globally serialized and rebuild their
collision index; no efficiency/scalability claim. GeneralCAD/production3DBL/full
physical unsteady/forces convergence and allcluster work remain outside this goal.
Next substantial work should address remaining geometry and resolved flow controls,
not repeat the unchanged broad regression or micro-optimize this checkpoint.
