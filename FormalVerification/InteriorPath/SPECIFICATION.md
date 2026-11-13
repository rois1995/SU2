# Proposed specifications and assumptions

## Domain and independent foundations

The selected profile is the existing `EulerPath/tests/integration/euler.cfg`: 2D node-centered Euler, standard air, fixed mesh, steady local pseudo-time, first-order ideal-gas SIMD Roe, kappa 1/2, entropy coefficient zero, explicit Euler, CFL 0.1 without adaptation, mesh zero, serial passive double, one rank/thread, Euler walls and far field. The connected theorem selects all allocated points as owned. Generic timestep and scatter contracts also state halo/frame behavior; they do not establish MPI execution.

The preserved foundations specify conservative variables `[rho, rho*u, rho*v, rho*E]`, positive density and internal-energy density, thermodynamics, orientation, assembly and explicit update. CFDLean M1 and M2-J/A/R/L are independently accepted. The new timestep estimator and geometric contracts below are proposed mathematical contracts; their approval belongs to the evaluator. Full Roe dissipation and far-field characteristic mathematics remain provisional SU2-derived representations.

## Geometry

`Geometry M E` contains typed endpoint indices, source ordering `left < right`, area normals, and each point's ordered `(opposite endpoint, edge)` neighbor list. `IncidenceValid` separately specifies that neighbors enumerate precisely the incident edges. This is a geometric input obligation; it is not silently inferred from an interior proof. The selected source constructs median-dual volumes from primal elements. For a 2D triangular piece, volume is one half the absolute cross product of `(centroid-point)` and `(midpoint-point)`; an oriented dual-normal increment is `(dy,-dx)`. Theorems connect the literal helper operations to these definitions, prove reversal/nonnegative volume, and describe the exact-real EPS-squared repair of zero normals.

`GeometryReady` requires positive edge-normal area. Nonempty adjacency, noncollinearity, complete mesh parsing, connectivity construction, ownership flags and dual-volume assembly are separate source obligations. Native checks of the small fixture do not prove these universally.

## Timestep

For each owned point i, execute the spectral reset, then accumulate its actual neighbor list in order. With area normal n, use

`edgeSpeed(i,j) = abs((v_i dot n + v_j dot n)/2) + (a_i+a_j)/2 * norm(n)`.

Then visit boundary markers/vertices in source order. Owned far-field vertices contribute `abs(v_i dot n) + a_i*norm(n)`; Euler walls contribute nothing to this estimator. Repeated visits are retained. These arithmetic means of nodal acoustic speeds are distinct from Roe-averaged sound speed.

For nonzero volume V, raw local pseudo-step is `CFL*V/lambda`. Store `min(raw, MAX_DELTA_TIME)`. Raw min/max diagnostics are updated before the cap. Zero volume stores zero without updating those diagnostics. The selected one-rank reductions are identities; global pseudo-step becomes the raw minimum. The solver's unsteady scratch is initialized separately. Configured physical timestep storage is not written by this steady branch. Time-accurate and dual-time limiters, viscous restrictions and grid-velocity adjustments are unsupported branches, not dropped from a claim covering those modes.

The memory theorem starts from primitive, volume and CFL inputs, deriving the spectral buffer and timestep stores. CFL is itself derived by executed all-point initialization in the composed prefix. Positivity follows from admissible backups processed by the preserved conversion, nonempty incidence, positive edge areas/volumes, positive CFL and cap. These are initial physical/geometric requirements, not hypotheses asserting a positive computed result.

## Interior implementation model

The selected serial coloring contains every edge in natural index order. Packets have positive width `w+1`. Active lanes enumerate this list once; inactive tail lanes gather the packet base edge and compute it, but do not write residuals. The edge reconstruction diagnostic nevertheless counts every gathered lane. First-order initialized zero edge counters therefore give zero diagnostics.

Each lane reads six primitive fields 0..5 through the actual eleven-column primitive layout. The model computes the literal selected SIMD arithmetic: conservative energy `rho*H-p`, ratio `sqrt(rho_R/rho_L)`, velocity/enthalpy ratio averages, sound `sqrt((gamma-1)*(H-|v|²/2))`, source P/inverse-P tensors, characteristic absolute speeds, central flux and the nested matrix dissipation. It does not insert the scalar boundary implementation's absolute density ratio or nonpositive-sound-square shortcut. These are disclosed implementation variants, not confirmed defects. The full expression is provisional, not an independently approved Roe specification.

Every packet gathers/computes all lanes before writes. Each active lane then traverses all four components, reading and adding the left row before reading and subtracting the right row. This refines independent oriented-edge assembly. Conservation holds over all allocated points, including halo rows, independently of the flux formula. An owned-only balance would need interface exchange. The frame preserves conservative current/old, primitives including untouched fields, truncation, characteristic caches, volumes, CFL and timestep storage.

## Composition and trust boundaries

The first-iteration composition executes CFL initialization before preprocessing/snapshot, timestep, interior assembly, selected inactive source/interface stages, far-field processing, final wall projections, and complete explicit Euler update. It derives boundary input memory and subsequently derives `InputsReady`; it never assumes already-correct post-operation memory or installs a supplied assembled output. Intermediate write-fold expressions are proof witnesses obtained from sequential load/compute/store execution.

Truncation and initial cache/geometry representations, allocated storage, valid marker/vertex identities and the fluid object's configuration remain explicit initial premises. Freestream nondimensionalization, C++ geometry construction, pointer-table loads, numerical scratch allocation/lifetimes, SIMD loads, integer widths/overflow, aliasing, compiler, floating-point/libm and parallel execution remain correspondence obligations. Literal scratch allocation/free in the fluid-interface caller is outside the abstract data memory, even when no interface marker executes.

All operational arithmetic is Lean exact-real arithmetic, including total real division and square root. C++ finite-value correspondence requires additional nonzero denominator, valid radicand, range and rounding obligations. A theorem quantified over arbitrary real rows is not a theorem about C++ division by zero or NaN propagation. The old-buffer invariant permits inadmissible trial currents, conditional on admissible initial backups; it does not prove positivity of the explicit update or floating-point reachability.
