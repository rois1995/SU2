# Findings and classification

| Item | Verdict | Evidence | Fix branch / test |
|---|---|---|---|
| Earlier boundary-to-memory composition gap | Formal model gap addressed in this candidate; external acceptance pending | Executed boundary theorem derives mb and InputsReady from m0 | `formal/lean-boundary-memory`; Lean composition and controls |
| Upstream numerical changes since d162 | No relevant changes in observed develop 103442f | Complete Git diff: only two CI workflows; identical numerical tree IDs | No production change |
| Discarded writes, wrong buffer, partial-reduction recomputation | Hypothetical semantic mutations rejected | Fixed representation/frame contract, Lean counterexamples, valid compiled fixtures | No claim these defects exist in production |
| Cache stride/marker/vertex/tail errors and dropped repeated visit | Hypothetical semantic mutations rejected | Six-field contract plus frame; closed uniform characteristic/Roe witness; compiled fixtures | No production patch |
| Existing production boundary routines on selected native cases | No discrepancy reproduced | Actual-method complete memory checks and solver integration evidence | Pinned baseline unchanged |
| Far-field/Roe literature compatibility | Unverified, provisional | Agent A request prepared; no relevant approved revision supplied | Compatibility proof/import pending |
| C++ pointers/scratch/allocator/compiler/floating-point bridge | Unresolved proof obligations | Explicit correspondence limits | Native tests are supporting evidence only |

No new production defect is confirmed by these controls. A constructed mutation
is not evidence of an existing SU2 defect. No repair is proposed, no production
code is changed, and no regression reference values are changed. Previously
repaired defects and accepted controls remain in the untouched earlier projects.
