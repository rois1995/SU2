# Findings

| Item | Verdict | Evidence | Production fix |
|---|---|---|---|
| Frozen BoundaryMemory | External SOURCE-LEVEL PASS, kernel/native pending | User decision recorded separately | None |
| Upstream numerical baseline | UNCHANGED | Same develop revision and pinned numerical tree diff | None |
| CFDLean M1 | ACCEPTED within state/physical-flux scope | Pinned candidate, acceptance record identity and compatibility proofs | None |
| Invalid backup state | NUMERICAL-METHOD LIMIT / caller obligation | Source retry ignores second physical flags; point theorem explicitly requires admissible backup | No confirmed reachable production defect claimed |
| Wrong/incomplete copies, kinetic/density omission, stale retry, tail overwrite, wrong residual clear | HYPOTHETICAL MUTATIONS | Lean negations and compiled numerical-contract failures | No production defect claimed |
| SIMD Roe, geometry, preprocessing-memory and dt | OPEN VERIFICATION OBLIGATIONS | Coverage/source trace | None |
