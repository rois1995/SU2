# Boundary-to-memory composition: frozen-candidate specification

This candidate adds an executed memory transition to the preserved BoundaryPath
specification. It does not accept or modify that earlier candidate. Its independent
cold Lean, dependency and full native evaluation remains outstanding. The new
candidate also requires external evaluation. The specification and proof files are
locked together only after development checks; the evaluator chooses approval.

## Independent requirements and their status

The conservative state, channel order, wall projection and explicit update are
reused unchanged from `../Lean`, `../EulerPath` and `../BoundaryPath`. The wall
requirement removes normal momentum from old solution, residual and truncation,
with the complete scalar product computed before either component changes.
The far-field requirement is the existing closed characteristic reconstruction,
cache update and scalar boundary Roe flux accumulation in `SU2Boundary.Spec`.
**Its CFD literature approval remains provisional.** It is independently stated
from the memory executor, but it is not an approved CFDLean literature dependency.
The source-derived arithmetic in `SU2Boundary.Boundary` is a separate model.

The new memory requirements are:

1. Execute every supplied far-field visit in order, then every supplied stationary
   wall visit in order. Preserve repetitions. Honor each ownership flag.
2. Use row strides 4 for conservative buffers and 11 for primitives and each
   marker's characteristic matrix. Cache identity is the pair (marker, vertex).
3. Far-field writes exactly temperature, two velocities, pressure, density and
   enthalpy at cache columns 0–5, then adds four flux components to residual.
   All other cache cells and every primitive cell remain unchanged.
4. Wall execution reads and projects old solution, then residual, then truncation.
   It changes only the two momentum cells in each selected row.
5. Derive the update input relation from the memory produced by those operations.
   Execute every component of every owned point with the preserved explicit Euler
   loop. Its result is old minus dt/volume times (residual plus truncation).
6. Preserve every memory cell outside the union of the executed boundary writes
   and owned current-solution destinations, relative to the original memory.

## Memory, domains and representation

`Mem` is a partial map from (block, scalar offset) to exact real numbers. Missing
loads fail with `none`. Blocks 0/1 are current/old; 2/3/4 are volume, periodic
volume and dt; 5/6 are residual/truncation; 7 is local CFL; 8 is primitive storage;
9+marker is that marker's cache matrix. This is an abstract allocation identity,
not a proof about C++ addresses, object lifetimes or allocator separation.

`Bounds` proves row bounds, 64-bit byte-offset bounds and the 16-bit marker bound.
`CacheSite` uses dependent finite indices, including each marker's own vertex
count. `cache_key_injective` and address-injectivity lemmas prevent index aliasing
inside the model. Bounds are supplied by the caller; actual allocation and geometry
construction are not yet refined. All cache addresses outside columns 0–5 are
covered by the frame theorem, including existing unknown values or absent cells.

`BoundaryRep vertices m0 s0 P` describes **pre-boundary** memory only:
old/residual/truncation contain s0; all eleven primitive cells in each represented
point row contain P. Known logical cache entries constrain six fields; a `none`
logical entry leaves actual cache storage unconstrained. The current buffer is
unconstrained because these boundary operations do not read it. Boundary execution
frames it and the update initializes every owned current destination.

In the principal theorem M is the total allocated point count and N≤M the owned
prefix updated by `nPointDomain`. Boundary visits can refer to any point in M,
including halo points with a false ownership flag. There is no distinct-visit,
nonempty-list, nonempty-point-domain or unique-cache-key assumption. The relation
between source `GetDomain` flags and the owned prefix is a geometry/caller
correspondence obligation, not silently inferred by the memory model.

`GeometryRep m0 vol (fun _ => 0) dt` initializes the actual getter blocks before
boundaries. Its preservation is proved. Zero periodic volume is the selected
nonperiodic configuration, not a general periodic result. Positive volume, finite
valid arithmetic and timestep calculation remain necessary for physical/machine
interpretation. Exact-real division is total; the theorem alone makes no claim
that a zero denominator is a valid C++ numerical execution.

`FarReady` is reused unchanged: for owned far visits, inside and reconstructed
ghost densities are nonzero and stored inside enthalpy matches the stated ideal-gas
enthalpy. It constrains initial primitive data and reconstruction, not the desired
memory transition. Admissibility/initialization, the conservative-to-primitive
relationship and the stronger finite machine domains remain caller obligations.
The previous admissibility lemma can discharge ghost density from positive input
states and nonzero reconstructed acoustic speed; that does not prove its callers.

## Executable model and postcondition

`executeSourceBoundary` consumes only memory, gas/free-stream data and the visit
metadata. It does not receive s0, P, a required post-state or a boundary callback.
The executor reads its operands from memory. Far-field cache-row bounds are checked
before ownership, reflecting the source pointer lookup; the wall ownership check
precedes wall loads. Reads after cache stores use the updated memory.

`source_boundary_to_complete_update` quantifies only m0 in its memory premises.
It produces mb and mu and proves:

- source-selected boundary execution from m0 returns mb;
- mb represents the boundary specification computed from s0 and P;
- mb satisfies `InputsReady` for the owned restriction of that state;
- the existing complete update loop returns mu with its full numerical contract;
- mu has the combined frame relative to m0.

The update's own frame is correctly relative to mb. Replacing mb with m0 there
would prohibit the legitimate boundary writes. Proof helpers such as
`projectionWrites` and `residualWrites` are used only after proving equality to
actual load/store execution. They are not an executor that installs a required
result. All equations are about exact reals. No floating-point preservation,
compiler correctness, full C++ semantics or complete Euler path is asserted.

## Controls and protected requirements

`Controls.lean` uses the same representation and frame requirements for positive,
negative and refactoring cases. Eight numerical mutations are rejected: discarding
writes, old-to-current buffer confusion, recomputing a projection scalar after the
first correction, six-column stride, marker or vertex zero substitution, erasing untouched
cache columns, and omitting a repeated far visit. The repeated-visit witness uses
the closed characteristic/Roe computation on equal admissible primitive states;
it does not supply an unconstrained flux callback. Parser rejection tests are
reported separately. Reversing the independent correction stores is proved to
preserve arbitrary model memory, including failure behavior.

No protected prior definition, statement, lock or acceptance criterion is changed.
