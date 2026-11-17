# Primary B5 draft corrections

All entries below concern unapplied generated code or its proof/readability helpers. They were found before a B5 production edit, build or numerical test. They are not production result failures. No gate, tolerance or production behavior was changed to obtain a passing check.

| Cause in our draft/helper | Correction |
|---|---|
| Initial draft contained only source slices and reversal; it did not implement the requested members. | Generated concrete Control/Reports methods, declarations and caller routes, then split the large methods at existing logical boundaries. |
| Moving the complete progress observer into Reports would place elections and failure decisions in the reporting unit. | Kept observer/finalization orchestration in Control and moved the print/census portions into Reports. |
| `CaptureDirectoryDonorStatistics` used `result` without its canonical alias. | Bound the original `context.control.result` object explicitly. |
| Started-time parameters were mutable references although Adapt's aliases are const. | Used const references for the original clock/time-point objects. |
| The callback helper was declared void despite returning a Boolean; the observer lacked the phase alias. | Made `NotifyStageObserver` return bool and borrowed the existing phase slot. |
| The quality print call added an outer timings guard around the original inner guard. | Called the print worker unconditionally, retaining the original inner guard and the baseline updates outside it. |
| A quality print worker lacked its `options` binding. | Added the canonical options reference. |
| Draft-only outputs initially omitted old methods/full headers or relied on later declarations. | Retained all nine existing method bodies, generated complete headers, and included Reports/Trial declarations before Control definitions. |
| Early function counts used pre-format text, treated calls as definitions and omitted free print workers. | Counted actual formatted definitions, including free functions. Split the quality details and stage suffix so every actual function is within 80 lines. |
| The new suffix continuation lacked started/counter bindings and the original geometry snapshot/baseline; its definition followed a nondependent caller. | Bound the existing clock/counter, passed the same const snapshot, borrowed the canonical baseline and supplied a forward declaration. No snapshot/query was added. |
| The suffix's edge-census counter parameter used size_t rather than the original uint64_t. | Borrowed the original counter as `const uint64_t&`, avoiding a platform-dependent narrowing. |
| Pre-render body proofs substituted original source chunks for helper calls and could miss a rendered-helper error. | Superseded them with a separate proof that reads final formatted generated method bodies and expands the actual helper calls. This proof must pass before application. |

The primary independently read every new Control, quality and stage worker and both headers. The static review verifies retained old bodies, unchanged optional declarations and actual function lengths. The final recursive body proof is a separate requirement; a pre-render receipt is not used as final acceptance evidence.

Replay clarification: the primary pre-apply static helper correctly read the then-current accepted B4 worktree. A separate post-only replay now binds its baseline reads to immutable c048 git objects, so replay after a later apply cannot accidentally compare a proposal to itself. The original receipt remains unchanged; the base-bound replay passes with no build or numerical repeat.
