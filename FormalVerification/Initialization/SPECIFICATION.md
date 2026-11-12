# Specifications and premises

## Independent mathematics

The independently accepted CFDLean mathematical revision is
`0a88e36b3810cd5adc3881db526ca8a35f81afbb`. Acceptance provenance is recorded in
coordination/status.json, with approval limited to ideal-gas state algebra and
oriented physical flux. Its definitions are imported unchanged, not copied into
SU2 or altered. `toCFD` maps tagged SU2 channels to density, momentum per volume
and total energy per volume. The compatibility proofs cover both conversion
maps, pressure, admissibility and area-vector physical flux. Normal vectors pass
directly without an implicit area multiplier. This is not Roe compatibility.

The SU2 constructor receives density rho, velocity (vx,vy) and specific TOTAL energy
e: its conservative row is (rho,rho*vx,rho*vy,rho*e). `seed_compatible` identifies
that row with the independent primitive-to-conservative map when
`e = p/((gamma-1)*rho) + |v|^2/2`, with rho nonzero. `seed_admissible` proves its
admissibility from an admissible primitive state. The actual freestream setup and
nondimensionalization supplying these inputs are not proved.

## Formal implementation models

Memory is the preserved exact-real partial map `(block,offset) -> Option Real`.
Blocks 0,1,5,6 denote current, old, residual and truncation. Existing primitive/cache
blocks and their eleven-column stride remain unchanged. Allocation, pointer
separation, lifetimes and concrete C++ object layout remain external correspondence
premises. No whole-memory expected result is an executor argument.

`copyCells` reads each source component from evolving memory before storing it to
the destination. `copy_executes` requires distinct logical source/destination blocks
and initialized source cells. `store_complete`, `flat_copy_coverage`, the preserved
visit-order/bounds/injectivity lemmas and `snapshot_refactor` cover every allocated
point and component, including empty domains and halo rows. Reverse traversal
preserves the complete logical memory. The snapshot wrapper assertion requires equal current/old extents of M*4; this
is an explicit concrete-layout precondition of the selected model. Assertion
failure is outside its successful-call domain. Hardware address bounds use the preserved
8-byte/64-bit allocation premises; these do not certify C++ pointer arithmetic.

`constructMemory` explicitly clears truncation storage, writes conservative rows
and copies current into old. Other CFlowVariable constructor allocations, primitive
and velocity-scratch initialization, restart, dual-time, RK4, 3D and alternate fluid
branches are outside that selected constructor fragment. `clearResidual` writes
zero to every residual component and frames other blocks. Production selects it
only when preprocessing is not Output and the reducer strategy is disabled.

`setPoint` follows the primitive computation and retry order over exact reals:
velocity and squared velocity; static energy; ideal-gas fluid state; density,
pressure, sound and temperature checks; optional old-to-current fallback and
recomputation; enthalpy after pressure. Density, pressure and temperature reject
nonpositive values; sound rejects strictly negative squared speed. A negative
sound value leaves that primitive destination unchanged. The point model retains
all 11 fields and frames columns 7..10. It accounts for the squared-velocity result;
its actual storage and fluid-model scratch are not connected to memory yet.

`rejection_iff` proves that the four checks equal inadmissibility under declared
ideal-gas assumptions gamma>1, R>0. `set_point_refines` covers either current-state
branch and requires an admissible backup for an admissible selected result. No
such guarantee is made for an invalid backup; source retries without checking its
second return flags. Total real division at zero is a mathematical totalization,
not an assertion about NaN, infinity, trapping or finite-precision behavior.

## Composition and open interfaces

The new principal theorem takes initialized pre-snapshot current memory only for
the values it derives for old storage. It executes the complete snapshot and
quantifies over later interior/timestep memory with explicitly required old-buffer
preservation, a frame relation, residual/truncation/primitive/cache representations,
geometry/timestep values and the preserved FarReady relation. Those are unverified
stage-output contracts, not theorems that the stages ran. The proof derives the
boundary representation's old component and then update readiness using the
unchanged BoundaryMemory theorem. The final frame is relative to pre-snapshot
memory and includes the explicitly declared intervening changes.

Primitive-model-to-memory execution, constructor-to-preprocessing linkage,
residual assembly from computed SIMD flux, geometry/edge enumeration, actual
volume/timestep computation, ownership provenance, initialization of caches,
C++ layout/aliasing, scratch, compiler and IEEE arithmetic remain obligations.
No theorem assumes a completed post-boundary representation as its principal premise.
