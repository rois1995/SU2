# Frozen full-engine anisotropic MPI matrix

2026-10-10. Defined before execution. Branch codex/native-3d-core, based on
pushed e920aedd2780c7daa8c47ce1efb3bc311ada9267. Goal 1 remains active.

Working repository: /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated.
Five six-tetrahedron faceted domains, twelve original boundary triangles with
individual preserved markers. Full refinement rather than a single split.

| Case | Physical domain | Original nodal sensor |
|---|---|---|
| thin4 | Unit xy box, height 1e-4 | diag(4,4,4/h^2) |
| thin8 | Unit xy box, height 1e-8 | diag(4,4,4/h^2) |
| shear4 | Height 1e-4, x physical = x logical + y | (xx,xy,xz,yy,yz,zz) = (4,-4,0,8,0,4/h^2) |
| spatial | Unit box | xy rotation .37 rad of diag(4+.4x,2+.4y), zz=4+.4z |
| turning | Unit box | Same spatial eigenvalues, nodal xy angle .2+.6x rad |

The remesher's actual immutable target is componentwise original-tetrahedron P1
interpolation of these stored nodal tensors. It is not the nonlinear analytic
turning formula evaluated at new points. Nodal metrics migrate as records;
none are rebuilt from evolved mesh tensors. No BL field is implied by thin
physical geometry; no production BL construction is tested by this matrix.

N=1/2/4, M in {N,max(1,N-1),max(1,N/2)}, sequential. Parent accepted mesh remains
unchanged; mesh and source records migrate to M and final mesh returns to N.
Frozen gates remain q>=.20, J>=.05, original-target integrated edge<=1.8.
Require full completion without weakening these gates or the target. Retain
failed/rejected grids and reasons, even when Catch reports failure.

Independent audits use actual original/migrated/imported records and samples:
exact coordinate/nodal-sensor equality, canonical donor keys, Fraction P1 with
relative-component residual<=2e-14, exact positive volume/no-overlap/convex domain
containment/oriented skin/manifold/facet coverage/markers, exact domain volume
preservation and final metric gates. Import observations cover each worker,
including samples on source ties; three tampering/missing-record controls test
the auditor. Produce self-contained SU2 grids and original metric JSON for
inspection. CGNS, CFD state and repeated coarsening remain required later work.

Profiles identify case and worker rank and retain phases, nested query costs,
private CPU, work and admitted memory. This is a correctness/diagnostic matrix,
not fixed-work MPI scaling: different worker groups may choose different edits.
Run only one low-priority build/case job at a time, one numerical-library thread,
MPI<=4, after real host checks. No cluster launch request to the absent user.

After first passing v1, v2 adds explicit parent-cycle profiling without changing
targets/operators: source+mesh migration, worker communicator creation, worker
phase, return to N, C++ test audit and record export, with residual other closure.
Synthetic nodal metric preparation is recorded once per case. Worker source
verification and external engine time (including final destructors) are nested;
inactive ranks' worker phase is explicitly waiting on the parent completion vote.
External Python audits/SU2 export are separate runner stages. No ParMETIS or
CFD solution/history/rebuild is implemented or included; no sum of rank maxima
is presented as elapsed time. This is a mesh-record fixture lifecycle profile.

V2 geometry/transport checks pass, but postprocessing caught an interleaved long
MPI4 ENGINE stdout record. V3 gathers fixture records using the explicit worker
or parent communicator and prints from one rank, outside engine timings. The
runner rejects malformed/incomplete profile records. Targets/operators stay
unchanged. The mesh-record cycle includes worker test reporting overhead;
profile output collection after the parent timer is excluded explicitly.
Audit JSON now sorts original donor records by cell ID; saved v1/v2 logs were
re-audited in anisotropic_engine_audit_v3 without rerunning native adaptation.
