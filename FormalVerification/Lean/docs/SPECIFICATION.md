# Phase 1 mathematical specification — candidate for independent approval

Status: authored specification; not yet approved by the independent evaluator. The final
handoff hashes freeze this text together with `SU2/Spec.lean` and the theorem statements.
Changes after approval require a new specification version and evaluator approval.

## Scope and units

This is a finite-dimensional, exact-real, vertex-centred finite-volume foundation.
`d` is an arbitrary natural dimension; the selected C++ layouts have d=2 or d=3 and
nVar=d+2. Dimensional or consistently nondimensional variables may be used; mixing
scales is outside the specification. A state contains density rho, momentum density m,
and total energy density E. A residual or conservative increment uses the same component
tags but need not be a physically admissible state. No turbulent kinetic energy, species,
chemical energy, incompressible layout, or real-gas equation of state is included.

## Thermodynamics

q(U)=E−(m·m)/(2 rho) is internal energy per volume. Admissibility means rho>0 and q>0.
For constant gamma>1 and gas constant R>0:

- u=m/rho; p=(gamma−1)q; T=p/(rho R); H=(E+p)/rho.
- The inverse map is (rho,u,p) ↦ (rho,rho u,p/(gamma−1)+rho(u·u)/2).
- Both round trips must be identity when density is nonzero. Positive density and pressure
  identify the physical primitive domain. The stronger physical assumptions must never
  be implicitly inferred from merely nonzero density.
- Conversion must agree with the staged specific-energy pressure formula in the selected
  ideal-gas source branch. The source's invalid-state fallback and sound-speed checks
  are separate obligations, not part of the round-trip result.

Division is total in Lean; every physical claim requiring nonzero denominators must carry
an explicit hypothesis or obtain it from positive density, volume, gamma−1 or R.

## Geometry and residuals

A control volume has a center and strictly positive measure. An interior edge identifies
separate nodes, a positive face area, and a normal n with n·n=1. Its flux F has already
been integrated over that face. Positive F leaves the left volume and enters the right.
Its residual contribution is +F at left and −F at right. Reversing the oriented face and
negating F must preserve each node's contribution. Summing over all represented nodes
must cancel an interior contribution component by component.

Assembly is independently defined as a sum of signed incidence contributions over a
finite list of events. Events may repeat; the implementation must accumulate rather than
overwrite them. The implementation model is sequential left-add/right-subtract scatter
and a left fold. For arbitrary initial residual R0 it must equal R0 plus the specified
assembly, and exact-real results must be invariant under event permutation.

The explicit update is U'_i=U_i−dt R_i/V_i. For a shared dt, volume-weighted totals must
change by −dt sum_i R_i, hence remain unchanged for only interior events. This is an
algebraic statement, not a stability or positivity theorem. Local time stepping with
unequal dt_i does not satisfy the stated global identity. Boundary fluxes, source terms,
partition ownership, duplicate halo events and MPI reductions require additional contracts.
The normal and volume fields are inputs: correctness of mesh geometry construction is
not proved by storing positive measures and unit normals in records.

## Moving-wall operation

Take a fixed unit normal n and prescribed wall velocity w for this operation; g=w·n.
Impermeability for a conservative state is m·n−rho g=0. With nonzero density this is
equivalent to (u−w)·n=0. For an increment D the required homogeneous constraint is
D_m·n=D_rho g. In particular a zero normal momentum increment is generally wrong for
a moving wall if the density changes.

The independent relational `WallContract` requires:

1. rho'=rho;
2. m'·n=rho' g;
3. some scalar a satisfies m'=m−a n and E'=E−g a.

These conditions express a normal-only momentum correction and its pressure work.
They do not name an algorithm or assume that its output is correct. The selected
correction is a=m·n−rho g, P(U)=(rho,m−a n,E−g a). Required results are satisfaction
and uniqueness of that contract, idempotence, equivalence of conservative and velocity
impermeability, and preservation by U+alpha D when both U and D satisfy the constraint.
Here alpha is one scalar for every component, and n,w must be the same for U and D.
Spatially changing normals, different stage wall velocities, or component-wise relaxation
need new results.

The exact identity q(P(U))=q(U)+a²/(2 rho) must hold for rho≠0. Consequently the correction
preserves admissibility for rho>0. This does not make arbitrary solution updates positive.
It also does not assert that P preserves total energy: the specified energy change is −g a.

The operation is the classical compressible projection already present in repair
`e0d3ee8d72598767227e62a9be5834a4554532b6`. The original source comment and repair test
establish its intended use in a translating wall frame. It is not a new wall algorithm.
Multiple simultaneous wall normals, their construction and their ordering are excluded
explicitly; this project does not treat a single merged normal as a valid sharp-corner
wall model.

## Proof acceptance requirements

Every named theorem must compile in the pinned environment and its transitive axioms
must be a subset of propext, Classical.choice and Quot.sound. These are standard logical
foundations, not CFD assumptions. No project axiom, proof placeholder, compiler-trust
axiom, native_decide result, circular contract or assumed desired conclusion is accepted.
The evaluator must inspect statement meaning as well as axiom dependencies. Proofs about
exact reals must never be described as floating-point proofs. The separate implementation
model and mathematical specification must remain identifiable. Source hashes establish
identity of artifacts, not their semantics or source/model equivalence.
