# B5 member extraction proposal (unapplied)

This is the root-review copy of the provisional B5 extraction. It is bound to the accepted B4 source at commit `c0486d57daf6ac4a1e66733db4a68b3c2dfe133a`, source SHA256 `6149e2cb2de3af25feefefdf056ce1c2b4aa75aa6170029348d4eedbffde5b0a`.

The five generated C++ proposal files and their SHA256s are recorded in `generated/frozen_proposal_sha_map.json`. Full member/header proposals retain the existing methods. No source C++ file was edited or applied; no compile, test, build, or numerical job was run.

`generated/final_output_expansion_proof.json` is the final formatted-file expansion receipt. It extracts the three actual source lambdas from the accepted snapshot, expands the final generated member bodies and report helpers, checks the stage output prefix/suffix/edge rows, compares the resulting token streams, and verifies the full main reversal/restoration byte-for-byte. The proof helper and receipts are reproducible from the generated files and pinned source commit. This is not a compile result or approval to apply the extraction.

The proposed split is:

- `EngineReports`: quality census collection, printing, quality-window updates, and stage rows.
- `EngineControl`: progress observation, synchronous observer callback/election, final proof and result completion.
- `CNativeEngine3D.control.proposed.cpp`: the Adapt body with the three local lambdas removed and all calls routed to explicit member APIs.

State is passed as explicit borrowed references or read from the same `EngineContext` optionals as the source lambdas. `curved` remains an explicit `Finish` argument. Stage output helpers retain a single `std::cout` stream, with the same final `std::endl`; the split introduces no extra flush or geometry snapshot. The details are in `PREPARATION_REPORT.md` and the source/dependency manifests.

Status: **UNAPPLIED; pending root review.**
