# Preprocessing memory candidate — external evaluation pending

This separate candidate extends frozen Initialization/InitializationReplay without changing their source, proofs, specifications or checker identities. It executes the selected primitive preprocessing and subsequent snapshot in a partial-address, exact-real memory model. It does not verify the complete Euler path or production C++ semantics.

Start with [REPORT.md](REPORT.md), [SPECIFICATION.md](SPECIFICATION.md), [SOURCE_CONNECTION.md](SOURCE_CONNECTION.md), [COVERAGE.md](COVERAGE.md) and [REPRODUCE.md](REPRODUCE.md). The parsed-caller counterpart is `SU2Preprocessing.source_preprocessing_snapshot_refines`. The principal declaration is `SU2Preprocessing.preprocessing_snapshot_refines` in `SU2Preprocessing/Composition.lean`. All of its memory premises concern the input **before** preprocessing; there is no supplied expected output memory or flux callback.

`candidate.json` freezes this proposed candidate and every historical file, theorem type and axiom inventory. This worker-authored freeze is an identity control, not approval of new specifications. Independent reviewers control specification approval and final acceptance. The previous Initialization review was SOURCE_LEVEL_PASS; its independent cold Lean/kernel and full native acceptance remain pending. This worker's checks do not close that external gate.

The pinned upstream numerical source is `d162016382f0c122433000c114e88d808b48f7a7`; observed develop is `103442f30ca2fba1ff48ef12aa1ae93480256a30`. Only CI files differ. No production patches are included. Lean is 4.19.0, mathlib is `c44e0c8ee63ca166450922a373c7409c5d26b00b`, and independently approved private CFDLean M1 remains pinned to `0a88e36b3810cd5adc3881db526ca8a35f81afbb`.

The public reviewer package contains SU2/formalization sources, logs and recipes. It **does not contain private CFDLean source**. The user has chosen to keep the separately prepared private source input local for now; an evaluator must obtain authorized access separately. `coordination/SPEC_REQUEST_A.md` is available for user forwarding. Receipt and approval of that request remain unknown.
