# Integrated B8 draft corrections

Status: unapplied candidate; not compiled or numerically tested.

- S11's original standalone draft allowed `PublishRefreshedSurface` to fall off the end of an optional-returning function on success. The integrated generator now emits `return std::nullopt;` after the original success statements. The terminal-return audit covers all 17 bool and optional workers.
- S13's alias-pruning rule selected aliases only when their names appeared directly in moved statements. `stats` uses `result.stats`, so three workers that used `stats` but not `result` directly lacked a necessary declaration. The integrated S13 generator now retains the result alias when retaining stats in `ConstructRegion`, `RunAndFinalizeRegion`, and `EvaluateRegionTarget`. Both aliases borrow the original result; no state or query changes.
- The integrated main renderer now formats only its new setup wrapper and call routes, including the previously overindented construction-pass call. The byte-reversal proof explicitly reverses these whitespace edits before restoring the B7 input.

The final source-level canonical-name declaration scan covers all 31 generated methods, including alias expressions, and the removed-local call scan covers the final main. These lexical checks are not a C++ compiler or a formal scope/type proof. Actual formatted helper bodies are token-matched to the checked sequential stages; the latter include recursive S13 callback-leaf and S15 operation proofs. The full source reversal, original Setup field/special-member check, terminal-return checks and format dry-runs passed. Primary review and the ordered build and gates remain required.
