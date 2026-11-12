# Upstream Euler composition candidate

This specification is new and awaits external approval. The accepted `../Lean` and
`../SourceGate` specifications, definitions, theorem statements and checking files are
unchanged. This candidate is a partial configured-path result, not whole-solver verification.

## Independent mathematical contract

Use the accepted `SU2.State`, `Channel`, `ControlVolume`, `InteriorEdge`, `assemble`,
`scatter` and conservative-update foundations. An edge flux is an already integrated
conservative flux, oriented from its distinct left endpoint to its right endpoint.
Flux computation is an input obligation; the present theorem does not assume Roe is
consistent, positive or entropy stable.

For a list of SIMD lanes, keep exactly the active lanes, retaining order and repeated
edges. For each component, mathematical assembly adds the flux at the left node and
subtracts it at the right node. Inactive padding lanes contribute nothing. Initial
residuals need not be zero. The implementation model first scales a lane flux by its
Boolean mask and performs the two in-place updates in source order.

Let A be this independently defined incidence sum and R0 the initial residual.
Let `finish` denote the still-unverified intervening residual processing, including
far-field contributions and stationary wall projection. The exact-real step contract is

    Unew[i,c] = Uold[i,c] - dt[i]/volume[i] *
                (finish(R0 + A)[i,c] + trunc[i,c]).

`Uold` and `trunc` mean the buffers **at update entry**, after boundary processing.
In production the wall operation also modifies `Solution_Old` and truncation error.
This contract does not claim that the input old buffer equals the state before that
operation, or that `finish` leaves old/truncation storage unchanged. Establishing the
joint boundary-state/residual/truncation relation is an explicit next obligation.
`finish` is a supplied interface, not a premise asserting the theorem's conclusion.
The theorem is valid for every such function; its production correspondence is open.

The source model consumes the selected `UpdateBlocks` method, `Identity::apply`, scale
declarations and explicit Euler switch arm. It connects their interpreted operation
to the regenerated, kernel-parsed scalar `CVariable::AddSolution`. The main statements
are `masked_assembly_correct`, `composed_step_correct` and `composed_source_cell`.
The last gives an actual modeled memory execution, destination value and frame condition.
It proves the scalar callee's address guards from caller bounds and the byte-extent bound.
It is a one-cell result, not a proof of the complete C++ update loop or heap allocation.

## Preconditions and scope

* Finite typed node/component indices; distinct edge endpoints; positive control volumes.
  Unit normals and positive face areas are inherited from accepted geometry definitions.
  No theorem establishes that a mesh loader produces these objects or enumerates each
  edge once. Repeated events are deliberately allowed.
* Boolean masks. The inspected `EdgeFluxResidual` caller constructs masks from its
  in-range test and pads inactive lanes with an existing edge. Arbitrary numeric masks
  are not this specialized caller's domain.
* Fixed mesh, no periodic markers: the selected scale uses physical volume plus zero
  periodic volume. Getter semantics and the zero periodic-volume initialization are
  open C++ refinement obligations.
* An initialized old-state cell; the modeled current/old matrices have N rows and d+2
  columns and `8*(N*(d+2)) <= 2^64`. Index and offset bounds are proved. Live allocations,
  actual pointer separation, all initialized cells and the whole loop's frame invariant
  remain open. The C++ caller's component counter is `unsigned short`; configuration
  here is 2D/four variables. The general mathematical theorem is not a claim that C++
  supports every natural-number dimension.
* Pure, exact real arithmetic. No finiteness/rounding, signed-zero, NaN, exception,
  compiler optimization, SIMD-instruction or fast-math correctness is proved.
* `dt` may be any real in the algebraic theorem. Actual local pseudo-time selection,
  positivity, stability and CFL restrictions have not been verified.

## Conservation and controls

`local_balance` keeps `dt[i]` inside the global sum. Only the specialization with
uniform dt, zero initial/truncation residuals and identity boundary processing uses
accepted interior cancellation to prove conservation of the volume-weighted state.
`local_time_not_conservation` is a concrete two-cell counterexample for unequal dt.
No theorem asserts admissibility after an arbitrary explicit update.

The unchanged composed contract has positive and local-array-renaming controls. Four
successfully parsed semantic mutants change the right scatter sign, mask guard,
truncation sign or Euler-update sign; Lean proves each violates that same contract.
The three accepted scalar mutants and universal scalar refactor equivalence are reused.
Unknown syntax, extra statements and directives are rejected; no fallback AST is used.
