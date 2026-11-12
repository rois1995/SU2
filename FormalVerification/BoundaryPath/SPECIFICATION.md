# Boundary and complete numerical update-loop specification

This is a new candidate. The evaluator has not accepted it. The pending EulerPath
candidate at `15ee28adf032a866e292926502a5f022e38199c8` remains byte-for-byte intact.
Production is upstream `d162016382f0c122433000c114e88d808b48f7a7`; no patches.

## Selected domain and independent definitions

`SU2Boundary.Spec` imports the accepted state, thermodynamics, geometry and wall
specification. This extension explicitly selects two dimensions, four conservative
components, standard ideal gas, stationary mesh, serial direct Euler, first-order
Roe, kappa 1/2, zero entropy fix, no low-Mach correction, viscosity, turbulence,
periodic boundaries, multigrid correction or implicit integration. The exact
configuration and mesh remain `../EulerPath/tests/integration/`.

All quantities in the proofs are real numbers. A `Fin N` point and `Fin 4`
component are bounded mathematical indices, not evidence about a C++ allocation.
For this serial boundary composition N is the selected owned point count (all
mesh points in the pinned case). The update theorem separately permits an allocated
row count M >= N and frames the remaining current rows. This does not model MPI
boundary vertices outside the owned prefix; representing their traversal is a
future caller obligation. The wall and far-field visit lists are ordered, may
repeat in-range points, and carry an ownership flag. The two boundary kinds may share corner points.

`BoundaryState` contains current conservative solution, old conservative solution,
residual, truncation error, and the characteristic primitive cache. The primitive
input at a far-field visit is an explicit snapshot supplied by preprocessing.
Its density/pressure/velocity consistency with current conservative data and the
stored enthalpy's agreement with the ideal-gas formula are outstanding caller
obligations. `FarVisit.insideEnthalpy` explicitly retains the stored source input;
the implementation flux reads it, while the independent specification uses the
thermodynamic enthalpy.

`farPrimitive` independently specifies the selected Riemann-invariant boundary:
q is the freestream normal velocity, c∞ the freestream sound speed. R+ comes from
the inside exactly when q > -c∞; R− comes from the inside exactly when q > c∞.
The advected tangential velocity/entropy comes from inside exactly when q > 0.
The reconstructed normal velocity is (R+ + R−)/2; sound speed is
(R+ − R−)(gamma−1)/4. Density is the stated real power of entropy*c²/gamma,
pressure is density*c²/gamma. All equality cases take the production else branch.
Temperature and enthalpy are cached as well.

`boundaryRoe` is a closed scalar Roe formula with explicit 4x4 eigenvector entries,
a closed left-matrix formula, physical Euler flux and spectral dissipation. It
retains the nonpositive Roe sound-speed-squared zero-flux branch. No unknown
boundary flux function occurs. The left matrix is not assumed to be an inverse;
proving the general eigensystem identity is a separate obligation. This scalar
boundary formula is distinct from the interior SIMD Roe computation.

`farSpec` adds the boundary flux and writes its cache entry. It preserves current,
old and truncation buffers. `wallSpec` applies the accepted stationary normal
projection to momentum in **old, residual and truncation** at an owned point.
Density and total energy in all three are preserved. Current and cache are framed.
Stationary wall processing does not add a pressure or energy flux in this selected
routine. `boundarySpec` folds all far-field visits first, then all final wall visits,
matching the selected `CIntegration::Space_Integration` order.

`updateSpec` is U_new[p,c] = U_old_after_boundary[p,c] − dt[p]/volume[p] *
(residual_after_boundary[p,c] + truncation_after_boundary[p,c]). It does not read
initial current as the base. Periodic volume is modeled as an explicit getter
load in the implementation, with zero periodic volume in the selected contract.

## Contracts and assumptions

- The accepted `WallContract` supplies unchanged density, normal-only momentum
  correction, impermeability and energy-work behavior. With stationary walls,
  mass and energy are unchanged. Unit normals are required for impermeability.
- `far_normal_velocity` and `far_tangential_velocity` specify independent geometric
  properties, while `riemann_reconstruction` proves inversion of characteristic
  combinations for gamma > 1.
- `far_primitive_admissible` derives positive ghost density and pressure from
  admissible inside/free primitive states and nonzero reconstructed acoustic speed.
  It does not assert every admissible pair satisfies that last condition.
- `FarReady` records nonzero inside and reconstructed densities and agreement of
  the stored interior enthalpy with the ideal-gas formula, needed for the
  exact-real enthalpy-to-conservative representation bridge. The sufficient
  admissibility result explains how a caller can establish those facts. Finite
  C++ evaluation of all divisions/square roots/powers is a stronger open obligation.
- Wall projection preserves old-state admissibility, but explicit Euler positivity
  is not proved. Residual and truncation vectors need not be admissible states.
- The update preserves impermeability after a final owned wall visit at that point.
  Two incompatible normal projections need not commute. There is no theorem
  asserting simultaneous impermeability for arbitrary intersecting walls.
- `InputsReady` requires initialized old/residual/truncation, volume, periodic
  volume and timestep cells representing the actual post-boundary data. It does
  not prescribe output cells or assume the update conclusion. Current may initially
  be arbitrary. Production allocation, initial residual zeroing, old-state copy,
  truncation initialization and primitive preprocessing are not proved.
- Logical storage blocks are disjoint. Row-major offsets have four components;
  owned count N <= allocated row count M; 8*(M*4) <= 2^64. The iteration-bound
  theorem also proves point/component counter increments fit LP64/unsigned short.
  Byte aliasing, lifetime, alignment and C++ object representation remain unproved.
- Positive physical volume and meaningful timestep are required when applying the
  mathematical model to CFD. Algebraic refinement itself also holds under Lean's
  total real division; it is not a license to execute C++ with zero volume.

`LoopContract` requires every owned point/component to equal `updateSpec` and
frames every other address, including halo current, old, residual, truncation,
geometry and timestep. The model covers the entire **numerical solution-update
loop**, not residual-monitoring side effects, MPI communication or verification
metrics in the enclosing method. In the selected serial configuration communication
is inspected/native-tested, not formally refined.

## Separation from implementation representations

`Boundary.lean` defines a two-iteration reduction followed by two stores for each
wall buffer, a separately written characteristic arithmetic representation, and
a serial j/k Roe dissipation loop. Its nontrivial proofs connect these to the
independent definitions, including the density-dependent conservative bridge.

`Loop.lean` reads getters from logical memory on every iteration, uses the accepted
parsed Identity.apply, explicit Euler arm and AddSolution source IR, and executes
all point/component visits sequentially. It proves traversal coverage, increasing
row-major offset order, no duplicates, bounds, read-buffer stability, output values
and memory frame. There is no implicit-update substitution.

`assembled_boundary_complete_update` composes the prior masked assembly theorem,
explicit boundary model and complete numerical update. Inputs still include fluxes,
edge visits, boundary geometry and initialized memory. The theorem does not prove
the configured solver produces those inputs.
