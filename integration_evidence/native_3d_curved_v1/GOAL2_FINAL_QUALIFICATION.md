# Goal 2 local curved-3D qualification

The declared local Goal 2 implementation and validation requirements pass. The final checkpoint must be pushed before updating the goal status. This continues published Goal1 checkpoint 23197eba1715f44d61c9ad3a070f61441dfb0560 on codex/native-3d-curved-local in SU2_NativeIntegrated.

Analytic sphere, arbitrary-axis capped cylinder and finite bump authority now support coupled surface/volume edits with preserved facets, markers, features and components. Existing split, collapse, movement, reconnection and automatic joint reconstruction use complete versioned dependencies, actual frozen-target validation and nonlocal clearance. Optional coordinated-strip placement remains a kernel experiment; it is not presented as an integrated runtime operator.

Finite geometry-aware source extension is generated before freezing, including the conforming cylinder/cap collar that resolved the saved rim obstruction. Original physical P1 interpolation has priority. Sensor-only caching and actual-query composition semantics remain intact; no after-failure padding, coarse-cell wall-tensor spreading or target reset was introduced. Composed 3D edge acceptance explicitly rejects until its geometry-resolved integrator exists; production3DBL is outside this qualification.

Current production is qualified by the 107-case-per-rank MPI1/2/4 regression and independent primitive/manufactured/solved-bump audits. Direct curved MPI transaction tests cover two-owner dependencies, M=N/M<N, stale versions, rank/storage vetoes, byte-identical rejection and successful publication. Primitive two-event runs cover sphere and both cylinder axes, all tested worker subsets, conservative current+BDF transfer, SU2/CGNS output and real restart.

The actual resolved compressible Euler vortex starts with 2592 tetrahedra/676 points, evolves through the real solver, computes a pressure-derived anisotropic metric before each of two native remesh events, conservatively transfers current/BDF fields, restarts the actual driver and continues solving. All six N/M variants pass natively and independently:

| CFD ranks N | Remeshing workers M | Tetrahedra after event1 | After event2 |
| --- | --- | --- | --- |
| 1 | 1 | 2767 | 2763 |
| 2 | 1 | 2767 | 2755 |
| 2 | 2 | 2778 | 2744 |
| 4 | 1 | 2767 | 2738 |
| 4 | 2 | 2767 | 2696 |
| 4 | 4 | 2764 | 2718 |

The twelve independent certificates check exact positive volumes, manifold conformity and no interior overlap, complete original-P1-or-declared-extension edge coverage, shape/sliver/length, actual transported tensors, actual conservative current+BDF integrals and positivity, new/lost-domain accounting, immutable reference/output signatures, refreshed nonuniform metrics and real restart states. Worst resolved-event bounds: q_min=0.2377832642 (limit0.20), J_min=0.0774960868 (limit0.05), L_max=1.7999493230 (limit1.8), transported-metric residual2.1146e-16 (limit2e-10). Maximum sampled bump error4.9672e-6 is below the pre-frozen0.003 geometry bound. Native analytic face bounds, rather than the samples alone, establish the surface contract.

Physics validation verifies the analytic initial seed, eight profile samples, actual evolved pressure contrast/state change and positive current/BDF states in all six variants. The serial first-event mean physical core-cell volume decreases by41.7%; this demonstrates local feature refinement, not grid convergence. Short solved-bump and manufactured grow/shrink/refine/coarsen controls remain separate evidence.

final_source_and_evidence_validation.json confirms847 current source files,763 unchanged production files,68 actual test objects,10 libraries,774 archive members, the actual executed binary, immutable input/helper hashes and repository-contained reuse links. Primitive source differs only in two reviewed error-message expressions; successful geometry/source algorithms are unchanged. The broad regression covers current diagnostics. Original rejected experiments, parser failure and whole-batch timeouts are preserved. The independent audit was rescheduled per event after its whole-batch timeout; its mathematical predicates and acceptance bounds did not change.

Inspect serial input/accepted meshes, states, frozen source and sidecars under resolved_vortex_v3/mpi1 (a closed link to the preserved native run). The matching actual density/pressure/Mach VTUs are under resolved_vortex_audit_v1/mpi1. MPI datasets are under resolved_vortex_v3/mpi2 and mpi4. The final exact certificates and aggregate are in resolved_vortex_audit_v2. GOAL2_REQUIREMENT_REVIEW.md and GOAL2_REQUIREMENT_MAP.json connect every original requirement to implementation and evidence.

No owned solver/build/audit job remains live. AdapNoExt, foreign ClusterResults and dependencies are untouched. Performance/scaling/cluster, production3DBL, general CAD, arbitrary-mesh feasibility, exact evolved-vortex accuracy, shock resolution and full physical unsteady/force convergence are not claimed.
