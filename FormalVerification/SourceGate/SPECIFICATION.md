# Candidate specification: scalar conservative update

Status: worker-frozen candidate, **not yet independently approved**. The approved Phase 1
specification in `../Lean/` is unchanged. This gate adds a source-connected model of one
scalar write; it does not certify C++ or IEEE-754 execution.

## Operation and contract

At source commit `e0d3ee8d72598767227e62a9be5834a4554532b6`, the scalar overload of
`CVariable::AddSolution(unsigned long, unsigned long, su2double)` in
`SU2_CFD/include/variables/CVariable.hpp:343` sets the current cell to the **old** cell plus
the supplied increment. It does not add to the current cell. The neighboring pointer
overload has a different operation and is excluded.

Let `before` be the memory before this call, `s` the old-state cell address, `d` the
current-state cell address, `x` the value read from `before(s)`, and `delta` the supplied
increment. `UpdateContract add before after d s x delta` requires:

1. `before(s) = some x`.
2. `after(d) = some (add x delta)`.
3. Every address other than `d` has exactly its previous contents.

This contract is independent of the parsed program. `source_update_correct` establishes
existence of a completed modeled execution and this contract, assuming only two valid
address calculations and the initialized old cell. No postcondition is assumed.
Current and old addresses may coincide: the old value is read before the write. The
frame condition does not claim the old cell remains unchanged when it is the destination.

## Data and execution semantics

`Semantics.lean` defines allocation identities and cell offsets as `Nat × Nat`. A memory
maps each address to `Option scalar`; `none` means an unavailable/uninitialized cell.
A matrix view supplies an allocation identity, rows and columns. An access computes
`row * columns + column` after checking both bounds and `8 * rows * columns ≤ 2^64`.
The row-major and byte-offset theorems prove these arithmetic bounds. They do not prove
that a C++ allocation of the supplied size exists.

The interpreter reads the right-hand side before assigning the destination, uses one
scalar addition/subtraction per AST node, and sequences statements in source order.
Local declarations bind a scalar without changing memory. Every load, assignment and
local binding is interpreted. Invalid addresses or unavailable reads return `none` in
the model; this is **not** a specification of how undefined C++ accesses behave.

Arithmetic is a parameter `Operations scalar`, with total pure `add` and `sub` functions.
The generic storage and refactor theorems need no algebraic laws for these functions.
`Binary64Bits` supplies a possible carrier only; no binary64 addition implementation is
provided or proved. `realOperations` is an explicit, separate exact-real interpretation.
`source_real_update` composes it with the unchanged `SU2.conservativeUpdate` definition.
It does not identify machine addition or the caller's multiplication with real arithmetic.
`finiteAdditionDeviation` defines a potential finite-decoding error; no error bound is proved.

## Recognized source language

`Parser.lean` consumes the entire selected method, including the scalar signature and
closing brace. Accepted syntax is:

- the exact return type/name and parameter types, with three distinct identifier names;
- assignments to `Solution(row,column)` or `Solution_Old(row,column)`;
- `const su2double name = expression;` with fresh local names;
- expressions formed from the increment parameter, previously declared locals, matrix
  accesses, and left-associated binary `+` or `-`;
- indices naming either unsigned-long parameter; ASCII identifiers and whitespace.

There is no recovery or default AST. Comments, macros within the selected body, branches,
loops, other calls/operators/types, nested scopes, casts, and trailing tokens are rejected.
This narrow grammar deliberately accepts the actual scalar operation and the recorded
refactor/mutants. It is not a general-purpose C++ frontend or a proved C++ grammar.
Kernel-checked `by decide` proofs establish parsing and the concrete ASTs of all five
fixtures. No native evaluator is used to justify a logical proposition.

## C++ interpretation obligations and domain

The intended configuration is C++17, LP64, passive `su2double = double`, eight-byte scalar,
serial access, row-major dynamically sized containers, and `NDEBUG`. The selected method
has no source branches. The container's compile-time row-major branch is selected by the
actual member type; bounds assertions are absent with `NDEBUG`.

To apply the restricted model to C++, an evaluator must additionally establish:

- correct method/overload selection, preprocessing, member and typedef resolution;
- live, aligned allocations and objects; initialized source cells and accessible destination;
- accurate layout-to-heap interpretation and in-range unsigned indices, without arithmetic
  wraparound or partial-byte aliasing; exact same-cell aliasing is modeled;
- that accessor calls have the modeled reads/address calculation and no extra observable
  effects; that no concurrent write changes either matrix during the call;
- an arithmetic interpretation matching the target, including rounding, exceptional values,
  signed zeros, overflow/underflow and relevant floating-point environment effects;
- correspondence from these source semantics through the compiler to executed machine code.

These are **open correspondence obligations**, not premises secretly asserted as theorems.
The proofs establish the restricted language's behavior. Memory safety, actual allocations,
full C++ standard semantics, compiler correctness, floating-point flags/traps and rounding
are not proved. Physical admissibility is not needed for the scalar storage contract and
is not claimed preserved by an arbitrary increment.

## Fixed experiments and acceptance questions

Original input uses old value 2, current value 11 and increment 3, in separate initialized
cells. The required result is current 5 with every other cell unchanged. Source mutants
replace addition by subtraction (current becomes -1), read current instead of old (14),
or write old instead of current (current stays 11, old becomes 5). All mutants parse;
Lean proves each violates the same `WitnessContract`. A common proof attempt for that
unchanged contract must pass for original/refactor and fail for each mutant. Parsing or
checksum failure alone does not satisfy this negative control.

The refactor renames parameters and introduces one `const su2double previous` local for
the old-state load. `refactor_equivalent` proves identical modeled outcomes for every
memory, layout, input and pure scalar arithmetic, including modeled failures and exact
aliasing. `refactored_update_correct` reuses the original contract. No production refactor
is applied and no arithmetic reassociation is permitted.

The new independent evaluation must inspect this specification, source selection,
semantics, theorem types and axiom dependencies; reproduce positive/negative controls;
and explicitly decide which correspondence obligations, if any, have been discharged.
Passing the worker's script does not constitute external acceptance or authorize a claim
that production C++ has been formally verified.
