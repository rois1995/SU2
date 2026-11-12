# SU2 Lean: boundary-to-memory composition

This candidate executes selected stationary Euler-wall and far-field memory
operations and derives readiness for the preserved complete explicit Euler update
loop from **pre-boundary** memory. It preserves all earlier candidate identities.
Independent acceptance and CFDLean far-field/Roe literature approval remain pending.

The principal theorem is
`SU2BoundaryMemory.source_boundary_to_complete_update`. Its memory executor has no
expected post-state argument or unconstrained boundary callback. It covers the
three wall buffers, actual 11-column primitive/cache layout, six selected cache
writes, ownership branches, repeated boundary visits, all owned update cells and
the combined memory frame relative to the original state.

- [Specification](SPECIFICATION.md): mathematical requirements and assumptions.
- [Source correspondence](CORRESPONDENCE.md): exact routines and evidence limits.
- [Path coverage](PATH.md): proof composition and unresolved Euler stages.
- [Reproduction](REPRODUCE.md): pinned, offline, source-only evaluator process.
- [Results](RESULTS.md): checked worker evidence and acceptance status.
- [Agent A request](coordination/SPEC_REQUEST_A.md): local file for user forwarding.

No complete Euler path, production C++ semantics, IEEE floating-point or compiler
correctness is claimed. Interior SIMD Roe, geometry/edge enumeration, preprocessing,
initialization and timestep computation remain outside the connected proof.
