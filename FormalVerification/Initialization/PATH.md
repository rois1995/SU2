# Connected coverage and remaining obligations

| Block | Formal result | Remaining connection |
|---|---|---|
| Independent state/physical flux | Pinned accepted M1 + pressure/conversion/admissibility/area-vector compatibility | Extend to literature-approved Roe/boundaries when available |
| Conservative constructor fragment | initialize_refines, seed_compatible, seed_admissible | Actual allocations, freestream inputs, constructor caller and restart variants |
| Primitive conversion/fallback | staged_pressure_correct, staged_temperature_correct, rejection_iff, set_point_refines; tail frame | Execute point loads/stores and fluid/squared-velocity scratch in memory and prove all-point loop |
| Current-to-old snapshot | copy_executes, flat_copy_coverage, snapshot_refines, source_snapshot_execution, general refactor equality | C++ arrays, deep-copy/parallel semantics and physical allocation/address mapping |
| Residual clearing | residual_clear_complete/frame | Caller branch qualification and CSysVector scalar-assignment lowering |
| Truncation | Explicit constructor zero stores | Prove no later selected setup changes; multigrid excluded |
| Geometry, volumes, owned/halo/edge enumeration | Preserved abstract bounds and visit proofs | Load/construct actual geometry and edge coloring; masked SIMD tail lanes |
| Interior SIMD Roe | Exact source dispatch identified | Independent mathematical contract, SIMD arithmetic, conservative scatter, reducer/ownership frame |
| Timestep | Exact caller/getter/storage trace | Eigenvalue reduction, zero-volume, CFL clipping and storage model |
| Boundary memory | Frozen 7cb4ee0 source-level external review passed | Independent kernel/native acceptance and provisional far-field/Roe mathematics |
| Complete explicit update loop | Preserved proof | Concrete C++/IEEE correspondence and caller origins |
| New composition | Snapshot -> OPEN interior/timestep contract -> boundary -> full update | Discharge that open contract with actual executions; current point primitive model is not yet its memory origin |

Dependency chain: approved M1 -> compatibility -> storage/copy -> primitive point
contracts -> snapshot/entry proofs -> protected BoundaryMemory -> protected full
update. Controls and transitive audits exercise these declarations without changing
independent or previously protected specifications.

Proposed next milestone: connect SetPrimVar's complete per-point memory writes,
including Velocity2 and fluid scratch, and the all-nPoint traversal to snapshot
entry. Derive the primitive/cache conventions and residual-clear entry from actual
preprocessing. Then prove geometry/owned/edge-enumeration contracts before the
SIMD first-order Roe memory/scatter refinement. Pin and compare Agent A's approved
Roe revision when available, keeping variants distinct from errors. Only after
those inputs are established should timestep storage and the current OPEN relation
be replaced by an execution theorem.

Other unsupported paths: MPI/OpenMP races and collective semantics, moving meshes,
Navier–Stokes, RANS, MUSCL, low-Mach corrections, entropy variants, implicit/RK/time
accurate stepping, restarts, multigrid/adaptation, alternate equations of state,
NaN/infinity/overflow/underflow and compiler transformations. No complete Euler-path
or whole-solver verification is asserted.
