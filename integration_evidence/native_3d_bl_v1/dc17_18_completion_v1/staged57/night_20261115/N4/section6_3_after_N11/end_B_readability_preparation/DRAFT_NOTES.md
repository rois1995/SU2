# End-of-phase-B readability inventory preparation

This is a **draft synthetic review**, not a claim that B8 is in production or that phase B has passed. `inventory.py --draft-overlay` reads the unapplied B8 main/Setup, B7 Control and B6 Ownership candidates, plus the other live private units. It formats each complete file in memory with `clang-format-18`; no production file is changed. `draft_synthetic_inventory.json` records the exact path and SHA-256 of each input and formatted output. Re-run without `--draft-overlay` after the accepted files are in place; reassess against the actual phase-B head and any changed hashes.

The current draft detects 328 definition candidates and 150 lambda candidates across the main and all private `.cpp`/`.hpp` units, including defaulted/deleted special members. Of those, 153 definition candidates and 23 lambdas are in the new private units: none are over 80 or 15 formatted lines respectively. It independently finds all 12 named B6, nine B7 and 31 B8 candidate methods; each is at most 80 formatted lines. The new main `setup_finish` lambda is three formatted lines. These are lexical results, not an exhaustive compiler-backed proof; the report lists the detection limitations and any initializer-list constructor candidates requiring inspection. Review the inventory and written exceptions before treating it as an end-of-phase report.

Draft ten longest detected definitions, all in `CNativeEngine3D.cpp` (line counts include the formatted signature through the closing brace):

| Function | Lines |
|---|---:|
| Adapt | 4998 |
| MeshOptimizer::SmoothBoundary | 419 |
| MeshOptimizer::Smooth | 320 |
| MeshOptimizer::CoupledBoundary | 232 |
| Selection::Add | 169 |
| MeshOptimizer::BoundaryFlip | 156 |
| MeshOptimizer::EdgeRemoval | 137 |
| OptimizedRepairPatch | 108 |
| MeshOptimizer::ConeReplace | 108 |
| MeshOptimizer::Run | 102 |

`Selection::Add` is the lexical scanner's unqualified `Add` entry; its enclosing class requires manual interpretation. The report provides each formatted line number and signature. The scanner may miss or misclassify templates, macros, unusual operators, raw strings, conditional compilation, function-try-blocks or complex constructor initializers. Its candidate lists are evidence for review, not a guarantee that no other definition exists.

CutPlan sections 3, 5 and 6 and `source_check.md` give the written exception: `EngineLegacyWall` may exceed 80 lines, and its historical rationale must not be read as a claim that the code has already been retired. The 80-line `Adapt` requirement is due at phase D, not the end of B. `Selection` and `MeshOptimizer` remain outside this cut and their pre-existing long bodies are baseline, not new-file exceptions. The existing “found during the cut” list remains in `source_check.md` and must be reconciled in the actual phase report. No phase C work, build, or numerical job is authorized by this preparation.
