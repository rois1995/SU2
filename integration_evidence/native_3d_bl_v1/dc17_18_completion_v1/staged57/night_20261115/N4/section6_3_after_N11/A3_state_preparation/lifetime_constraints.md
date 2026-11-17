# Root implementation constraints — no production edit

The six inventory groups are interleaved in the original declaration order. A3 must not physically regroup objects in a way that changes initialization/destruction, reference identity, allocations, votes, clocks or existing byte accounting. Context is created after geometry resolves; pre-geometry state remains canonical in the setup storage and may be borrowed. Options/composition/surface_target remain the existing by-value inputs; no extra copy/move of callbacks or targets.

An implementation must preserve original-order canonical ownership while exposing the six unit sub-structs. In-place delayed construction is already the A2 mechanism; unit references/views can point to stable canonical storage where physical grouping conflicts with lifetime order. Do not duplicate state, count sizeof(Context), bind a reference to an unengaged optional value, or use heap wrappers merely to defer construction. Exact current field membership/declaration dependencies still require primary review before A3.

Avoid broad textual variable renames: short names such as result/source/phase can be shadowed by inner locals. Canonical references at the old declaration points can preserve existing lambda text during phase A. Later unit methods can retain original names through stable member references or short local aliases, preserving ordinary inner lexical shadowing. This is a mechanical decomposition, not a new execution model.

Scoped exits remain critical in B/C: call the original finish body while original Timer/CavityScope/FieldAccounting objects are still alive. Returning an enum first and then running finish can destroy them too early. Use the scoped continuation/status reading already in source_check item9, with finish invoked exactly once. Preserve short-circuit collective order and the first private-join Allreduce. Legacy wall remains moved verbatim; duplicate differing donor/commit/cavity paths stay separate.

These notes record constraints/possible mechanical bindings, not a new proposal, acceptance, or second source check. No code was changed.
