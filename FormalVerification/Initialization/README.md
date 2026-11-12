# SU2 Lean: initialization and snapshot contracts

This new candidate preserves BoundaryMemory at `7cb4ee0b2a5e62bdaadcaab1a630881971403de4`.
The user reports that its external source-level review passed. Independent cold
Lean/kernel, transitive dependency and full native acceptance remain pending;
[review-decision.json](review-decision.json) records that exact scope.

New checked blocks are conservative constructor storage, complete current-to-old
copying, flat-loop traversal coverage, staged ideal-gas primitive conversion,
invalid-state fallback, residual clearing and conditional snapshot/boundary/update
composition. Approved CFDLean M1 is now a pinned dependency with compatibility
proofs for state conversion, admissibility and oriented physical flux conventions.

The principal new composition is `snapshot_interior_boundary_update`. It executes
the snapshot first and derives the old-buffer values used by the protected boundary
and complete update theorem. Its quantified interior/timestep gap remains OPEN:
no execution of those stages is represented or proved by that quantifier. The
current, old, primitive and geometry interfaces are stated explicitly. The primitive
proof is a point model; connecting its writes and scratch to logical memory remains
an additional obligation. This handoff advances reusable contracts and does not
complete initialization-to-boundary memory or the configured Euler path.

Read [SPECIFICATION.md](SPECIFICATION.md), [SOURCE.md](SOURCE.md), [PATH.md](PATH.md)
and [REPRODUCE.md](REPRODUCE.md). [RESULTS.md](RESULTS.md) records worker evidence.
The unchanged [request to Agent A](coordination/SPEC_REQUEST_A.md) remains available
for user forwarding. Its receipt is not assumed.

CFDLean is private. Its source is deliberately excluded from public handoff assets;
an authorized evaluator must supply the exact accepted checkout. Roe and far-field
formulations remain provisional; importing M1 does not approve them.
