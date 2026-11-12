# Local supplemental validation — 2026-11-12

These checks were run by the working agent. They are reproducible evidence for the user's
independent evaluator, not an independent acceptance decision.

| Check | Observed result |
|---|---|
| Frozen handoff identity | All 50 tracked files equal original commit `1c022ca8307517585a443b2609bddd873f31c46c`, including original whitespace and logs |
| Source provenance | Private shallow Git repositories pass `git fsck`; repaired tree, parent, 11 mapped blobs, before-header and full repair patch agree |
| Fresh production build | Pinned source, GCC 13.3.0, pinned Meson/Ninja/Eigen/MEL; serial double release; two CPUs and `ninja -j2` |
| Repaired `[HLLC-wall]` test | **47,460 assertions passed in one test case** |
| Parent production with identical repaired test fixture | **Failed as expected**: first failure at `moving_wall_tests.cpp:136`, momentum residual `0.0` versus expected `6.0`; `--abort`, 66 assertions passed and 1 failed |
| Source restoration | Disposable production header restored byte-for-byte; no tracked production changes remain |
| Negative transport controls | Safe extraction passes; parent-directory traversal and escaping symlink rejected |
| Environment control | Inherited compiler/search-path/repository overrides removed; Lake cache disabled |
| Synthetic wrong evaluator lock | Rejected before output creation; actual evaluator lock left unchanged |
| Cold Lean rebuild | **Passed from an empty output directory**, all 940 build targets, Lean 4.19.0; no precompiled dependency Lean objects supplied |
| Axiom and theorem audit | **231 logical declarations / 113 theorem declarations**, only `propext`, `Classical.choice`, `Quot.sound`; all 52 explicit theorem types exported |
| Negative axiom fixture | **Rejected** outside the accepted project |
| Extracted-source C++ experiment | **Passed**, 1,000 deterministic cases; inherited defect and both refactors checked as supplementary evidence |
| Final protected-file checks | **Passed**, including external evaluator specification lock; dependency tracked sources unchanged |

Both build/run recipes execute inside `unshare --user --map-root-user --net`. No network
interface with outside connectivity is available during those runs. The Lean compiler
comes from a new download of the official release, with the recorded hash; dependencies
come from pinned Git objects with no precompiled Lean caches. ProofWidgets display-only
JavaScript assets and its original publisher build trace are supplied separately. The dependency/source rebuild
and all supplementary C++ checks run sequentially, never alongside another build here.
The machine owner's explicit two-thread exception is used only for the Lean load wait.

The successful cold run and follow-up checks are retained under `evidence/lean-*`. The
initial missing-display-assets failure and successful packaging confirmation are also
retained separately. The negative axiom fixture is not an accepted project dependency.
The recipe read the evaluator-owned JSON from the supplied ZIP through a pipe and did
not edit or publish that record. Independent acceptance is still pending.

The fresh C++ evidence is under `evidence/wall-*`: complete build commands, compiler
options, compile command database, fixed/parent run logs and binary SHA-256 values.
The parent control stops on its first failure; it does not measure all old defects.
The repaired test covers its full existing loops. Serial assertion totals differ from
historical logs in the frozen handoff; those older binaries were not reclassified as
fresh evidence.

The first packaging attempt omitted MEL, producing a missing `mel.hpp` build error. Its
log is retained as `wall-initial-missing-mel.log`. After including the exact existing MEL
revision, a new checkout and build completed successfully. Production files, test
assertions, numerical flags and the Lean contract were not changed to fix that packaging
omission.

The first cold Lean attempt exposed an additional offline packaging requirement:
Mathlib's ProofWidgets dependency requires its JavaScript display assets. The official
v0.0.57 release was downloaded and only its 19 JavaScript files plus original display
trace were packaged. No `.olean` or other compiled Lean files were selected. The initial
failure log is retained. The corrected recipe passed again from a fresh directory;
no mathematical definitions, proof statements, source dependencies or audit rules were
changed to resolve this build-system requirement.

The broad Python-spacing check against `origin/develop` reports pre-existing lines in
the historical SU2 base. The supplemental commit changes only `FormalVerification/`;
its new Python lines pass the spacing check. Likewise, `git diff --check` identifies
existing whitespace in the byte-preserved handoff and patch snapshot. These protected
historical files were not reformatted. The newly written supplemental code and documentation pass their own check. Captured
compiler logs also retain their original trailing spaces and are not reformatted.

No whole-suite CFD regression, MPI, OpenMP, AD, production floating-point proof, verified
C++ translation or external independent certification is claimed. The current maintained
SU2 branch is not the pinned historical baseline. Existing model/refactor and source-slice
experiments retain the evidence level stated in the frozen correspondence report.
