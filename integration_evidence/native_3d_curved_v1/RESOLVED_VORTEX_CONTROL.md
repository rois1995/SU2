# Resolved nonuniform Euler control — qualification in progress

SU2_NativeIntegrated, codex/native-3d-curved-local; Goal 2 ACTIVE, not complete.
The frozen CASE_PLAN.json defines a12x12x3/2592tet/676point initial mesh on
[-2,2]x[-2,2]x[0,1], analytic lower bump amplitude-.001, compressible vortex
center(-.25,0), radius.7, beta3 and at least8 central profile samples. Actual
Euler/Roe, noMUSCL, Mach.5, CFL.3, physical dt1e-4 and8inner iterations supply
pressure Hessians. Metric complexity400, hmin.2/hmax.9/AR2/noise0; strict existing
q.20/J.05/length1.8/Hausd.003 and bounded work16384 are fixed before tests.
No tensor target is prescribed and no limit was relaxed after failure.

The initial condition is the standard positive isentropic compressible vortex:
thermal factor1-(gamma-1)*beta^2*exp(1-r^2)/(8*gamma*pi^2), density/pressure
powers and transverse circulation, added to the solver's nondimensional free
stream. Startup and all three conserved time-level arrays are initialized before
actual CFD runs. The curved finite-domain evolution is not asserted to be the
exact unbounded-vortex solution or a convergence result.

Serial native testcase PASS:31034assertions/1case, actual pressure metric changes,
two remesh/conservative current+BDF transfer/restart/continue events. Meshes:
2592/676 initially,2767/655 afterevent1,2763/656 afterevent2. Initial physical
source/CFD data are preserved; accepted grids and matching current/BDF states are
inspectable. Actual minimum evolved pressure ratio is.74344 and relative state
change.06171. Independent initialization error is3.57e-16, measured profile8points;
all evolved current/BDF states are positive. The mean physical core cell volume
falls from.0061732 to.0035986 (ratio.58294), with246 to422 core cells, showing
refinement of the nonuniform feature rather than only a global count change.
These resolution measurements do not establish grid convergence.

resolved_vortex_v1 receipt is FAIL only because the run wrapper's plural-only
Catch-result regex did not recognize '1 test case'. Native exit was0 and all
assertions passed. Original script/logs/receipt are retained. resolved_vortex_v2
uses a singular/plural self-checked parser, verifies every original serial artifact
and source/binary pin, and reuses that result through the repository-contained
mpi1 link. No successful serial CFD/adaptation job was repeated.

LIVE owned controller60343: MPI2 passes both worker-count variants; MPI4/M1 two events complete,
MPI4/M2/M4 still pending. First MPI4/M1 two events complete. Revalidate this exact
handle before further heavy work; no production source changes while it is live.
One CPU/nice19/threads1/build-j1, MPI<=4, no unrelated-process interruption.
Independent whole-lifecycle geometry/field/conservation/restart audit is prepared
with sweep-box admission before exact pair checks, but NOTRUN until the matrix
terminates. No full MPI or whole-lifecycle certificate is claimed yet.

VTU exports of actual donor and accepted pressure/density/Mach are in
resolved_vortex_audit_v1/mpi1. They match the native geometry/state hashes; native
completion status is always separate. The source grids, frozen sensor tensors,
actual PDE states and reference sidecars remain in resolved_vortex_v1/mpi1.
Next: finish the outstanding matrix, run independent full audits, update the
explicit requirement map and push qualification. Preserve full Goal 2 scope;
no shock/fullphysical-unsteady/forces/production3DBL/generalCAD/performance or
arbitrary-feasibility claim.
