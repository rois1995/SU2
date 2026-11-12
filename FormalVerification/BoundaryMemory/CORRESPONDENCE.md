# Source correspondence and limits

Source baseline: `d162016382f0c122433000c114e88d808b48f7a7`, no production
patches. On 2026-11-12 the observed upstream develop revision was
`103442f30ca2fba1ff48ef12aa1ae93480256a30`. The complete Git comparison changed
only two CI workflows; SU2_CFD, Common and externals trees are identical. The
numerical baseline remains d162. `baseline.json` records the comparison and tree
identities; the evaluator kit includes both revisions. This qualifies the observed
revision, not arbitrary future develop changes.

The frozen BoundaryPath commit `73a322b640f45505e8d8fb70fbce46e6ccffd206` and
investigation `5a6d3a578a4a348b91ef6ce8dcd259e142f60d37` are retained byte for byte.
The new directory is the only source change. `source-manifest.json` fixes complete
file hashes, selected byte spans and fixture variants. Regeneration reads pinned
Git objects, never the mutable working production source.

| Production routine/storage | Memory model connection | Strength of evidence |
|---|---|---|
| `CEulerSolver` constructor `nPrimVar=nDim+9` | 11 primitive/cache columns for nDim=2 | Complete assignment parsed in Lean; constructor dispatch/manual specialization |
| `CFlowVariable::Primitive.resize` and `GetPrimitive` | Block 8, row offset point×11 | Complete selected statements/getter token-checked; object allocation/layout bridge manual |
| `AllocVectorOfMatrices(nVertex,nPrimVar,CharacPrimVar)` and `GetCharacPrimVar` | Block 9+marker, vertex×11 | Allocation statement/getter parsed; allocator and lifetimes manual |
| `C2DContainer` row-major const row pointer return | Row×column-count addressing | Exact return fragment parsed; macro/storage branch and mutable forwarding inspected, not certified |
| `CFVMFlowSolverBase::BC_Sym_Plane`, reached by `BC_Euler_Wall` | Old, residual, truncation load/reduce/store sequence | Complete selected three projection fragments plus old pointer binding parsed; normals, wrapper and selected branch manual |
| `CEulerSolver::BC_Far_Field` | Cache-row lookup, ownership, primitive reads, characteristic reconstruction, six writes, scalar Roe call, residual additions | Six-write fragment and cache getter parsed; remaining arithmetic/call order manually represented and native-tested |
| `CSysVector::AddBlock` → `SetBlock<VectorType,false>` | Four ordered load/add/store operations | Complete AddBlock and SetBlock bodies parsed, specializing nVar=4 and alpha=1 |
| `CIntegration::Space_Integration` | Weak far-field pass precedes final Euler-wall pass | Manual pinned caller trace; actual marker/vertex list generation remains supplied |
| `ExplicitEuler_Iteration` → selected `Explicit_Iteration_impl` | Complete owned point/component update, actual abstract getter loads | Preserved restricted source gate and loop theorem, applied with derived readiness |

## Restricted language and execution semantics

`Parser.lean` reuses the preserved lexer. Only complete declared fragments are
accepted; extra tokens, unknown characters and unsupported shapes return `none`.
Block comments and whitespace are lexical trivia. No whole C++ function containing
unhandled branches is claimed to have been parsed. Selected return/assignment
fragments are explicitly identified as fragments, with surrounding file identity
available for manual review. Canonical token grammars are fixed in the parser,
not generated from the same input used for the acceptance check.

The new supported semantic alternatives include old/current binding, primitive
extra columns 9/4, actual/zero marker or vertex lookup, six cache writes with or without an
explicit tail-erasure loop, and completed/recomputed reductions with forward or
reverse correction order. Combined refresh+reverse and unmatched three-buffer
schedules are unsupported and fail. `wallFrom`/`farFrom` expose those restrictions.
`source_wall_connection` and `source_far_connection` connect the parsed original
codes to the executor used by the principal theorem. The previous source controls
remain unchanged. All new negative numerical controls compile and run; fail-closed
parser tests are a separate result.

## Required distinctions

- **Mathematical properties:** preserved conservative/geometry/assembly/wall
  identities; provisional far-field and boundary Roe definitions; independent
  memory representation and frame requirements.
- **Implementation-model properties:** sequential loads/stores, bounds and address
  separation, six cache writes, all untouched cells, owned/unowned branches,
  repeated visit folds, derived readiness and complete explicit update composition.
- **Actual source established:** byte identity, token acceptance for the declared
  restricted fragments, and their stated interpreter results. These are not a
  certified C++ semantics for the surrounding functions or translation units.
- **Manual or unverified correspondence:** Python byte selection, configuration and
  dispatch, geometry enumeration, primitive/scratch bindings, characteristic/Roe
  translation, row-major specialization and allocation identities.
- **Native evidence:** actual pinned methods and selected full solver runs, plus
  fragment fixtures and numerical mutations. These support the manual bridges;
  they cannot turn a manually translated model theorem into a C++ proof.

## C++ memory, arithmetic and compiler obligations

Actual primitive/normal setters borrow pointers; the scalar Roe residual object
views internal scratch storage. Valid lifetimes through ComputeResidual/AddBlock,
nonaliasing of allocations, scratch ownership, lack of concurrent interference,
object-layout correctness, initialized scalar reads and sufficiently sized buffers
remain assumptions. The model proves bounds in its allocation scheme and models
all numerical writes in the selected branches; it does not model every diagnostic,
profiling counter or scratch write in the full C++ object graph.

The selected execution is serial passive-double, 2D, fixed-grid, inviscid ideal-gas
Euler, first-order Roe, explicit Euler, no periodic volume or turbulence. The
boundary flux is the **scalar boundary Roe routine**, distinct from the outstanding
interior SIMD Roe computation. Dynamic-grid, implicit, viscous and turbulence
branches are excluded by this disclosed configuration, not silently erased from a
general theorem. Full binary linkage includes other routines but does not verify them.

Exact reals do not justify IEEE rounding, NaN/infinity behavior, sqrt/pow library
accuracy, reassociation under release fast-math, integer machine semantics outside
the proved bounds, or compiler correctness. Native release flags are recorded in
the evidence. Fragment controls use C++17 -O2 without fast-math; unused parameters
are permitted so a valid marker-zero mutant is tested semantically. No compilation
failure, hash mismatch or unsupported syntax is counted as defect detection.

Agent A coordination is in `coordination/SPEC_REQUEST_A.md`. The request is prepared
for user forwarding, with no receipt or approval implied. CFD_Lean M1 at
`0a88e36b3810cd5adc3881db526ca8a35f81afbb` is pending independent acceptance and
does not cover Roe/boundaries. It is not imported. Upon an approved revision, pin
it and prove compatibility of state/enthalpy conventions, wall normals,
characteristic branch equalities/sonic cases and Roe matrix/dissipation conventions;
report incompatibilities rather than altering the approved specification.
