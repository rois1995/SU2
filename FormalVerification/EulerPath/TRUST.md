# What is and is not verified

| Evidence | Exact meaning |
|---|---|
| Accepted Phase 1 proofs | Exact-real state/thermodynamics, geometry, moving-wall correction, oriented assembly and update specifications; unchanged historical acceptance. |
| New Lean proofs | Kernel-checked restricted scatter/caller semantics refine the independent local-step contract, using accepted assembly and scalar-update results. |
| Raw source consumption | Lean computes parsing of fresh upstream bytes; parsed guards/operators determine execution. Complete scatter/apply methods and complete selected scale/arm fragments are consumed. Other enclosing branches are mapped and excluded by the selected configuration, not silently erased from a whole-function claim. |
| Actual C++ evidence | Git membership and spans, complete-file identities, manual dispatch/accessor/layout inspection, preprocessing check of scalar method, native inherited-method tests and full solver integration. These are not a certified C++ semantics refinement. |
| Floating point | Native tests only. Exact-real identities are not IEEE-754 or fast-math theorems. |
| Acceptance | Prior acceptance attributed to supplied external reports. New candidate and scripts are worker-authored; a separate evaluator must approve this specification and run its own checks. |

The parser is deliberately specific, not a C++ frontend. It validates fixed surrounding
syntax and reads the semantic operator/guard slots. It accepts two array binder names
consistently. The source selector is Python and unverified; complete source Git objects,
byte spans and regenerated literals make its result independently checkable. The parser's
own semantics-to-C++ correspondence is still manual. Hashes establish identity, not meaning.

`UnpackBlock` is included as raw source and in native execution, but its loop's translation
is manual: `scatterLane` interprets `out[k][i] = mask[k] * in[i][k]`. C++ macro expansion,
member/accessor resolution, static dimension assertion, scalar and SIMD lane storage,
row-major pointer arithmetic, aliasing and compiler correctness remain obligations.
The selected no-OpenMP/no-AD configuration removes the pragma/concurrency branches;
C++ unsigned overflow and memory safety are not inferred from real arithmetic proofs.

The scale parser recognizes the two actual declarations. A successful result certifies
recognition, not the getters' semantics or division safety. Positive volumes are part of
the mathematical input. No parser proof establishes runtime dispatch by itself.
The selected Euler arm is reached via the freshly inspected explicit call chain; the
old implicit call site is not used as evidence for this choice.

The `finish` boundary interface is deliberately open. The present result composes the
interior-assembly and update components across that interface; it does **not** prove an
uninterrupted end-to-end C++ trace through the intervening boundary routines. Roe flux,
boundary construction, geometry loading, edge enumeration, initialization and the whole
point/component loop must be connected before claiming a complete Euler residual/update.

`EulerAudit.lean` reuses the accepted audit design without modifying the accepted checker.
It inspects transitive dependencies of all logical declarations in the new namespace;
only `propext`, `Classical.choice`, `Quot.sound` are allowed. Internal unsafe compiler
runtime artifacts are counted separately; no logical dependency is exempted. A separately
injected axiom must be rejected. Full theorem types and axiom lists are exported.
No compiled Lean caches are included in the review handoff. Development reuses pinned
mathlib dependency builds; the cold recipe restores source dependencies and builds anew.

A checked extraction route would need a semantics-preserving mapping from a typed C++
frontend (including class layout, overload resolution, macros and the memory model) to
this IR, or a separately checked translation certificate. Extending this narrow parser
alone cannot discharge that obligation. For this milestone it earns its cost by exposing
the actual SIMD guard/signs and Euler caller arithmetic to kernel-checked mutation controls.
The next useful extension is the stationary boundary triple (old state, residual,
truncation), then first-order Roe flux; broad C++ syntax support is deferred.
