# B7 unapplied C++ proposal

The generator extracts the six B7 lambdas from the frozen B3 source at commit
`ce25219f04cb01285bfeb8b33dba1e6a3496782a` and writes proposed main, header,
and implementation files under `generated/`. This source remains the immutable
806e13 snapshot even though the working tree has since advanced to candidate
B4. The generator verifies input hashes before generation, direct removed-body
token streams, the inlined `enter_phase` body, and exact byte reversal of main
call routing plus lambda reinsertion. No production file was edited; no build,
test, or numerical job was run.

Primary review caught one stale Adapt-local call in the extracted
`CompletePhaseEntry`: `begin_length_generation(false)`. It now binds to
`BeginLengthGeneration(context, false)`. The inlined body proof reverses that
member call to the original lambda call. The B7 audit also reads the immutable
`ce252` snapshot rather than the advanced working tree.

## Bindings

- Main source base (`ce252`): `806e13eccc41df3a6e19602b38e1297dc8b9852fd4a6a8804af7a8b67d4d80e5`
- `EngineControl.hpp` base: `4d9ba9cacce2c4103745e9711211091bcb9ae56642f67ddb5ebd27ac172fbd7c`
- `EngineControl.cpp` base: `8a49a7de40d846ad4e7f6164ed2c8b1d345e3939b2aa972408eb9db87a3a4b11`
- `binding_inventory.md` / `binding_audit.json` are the read-only binding and
  canonical-slot audit immediately preceding this generator.

Call routing from the current main candidate: `begin_length_generation` 3,
`enter_phase` 7, `capture_checkpoint` 1, `rollback_checkpoint` 2, and
`discard_checkpoint` 1. These are recomputed from this source snapshot; older
inventory counts/positions are not reused.

## Proposed files

- `CNativeEngine3D.cpp.proposed` SHA256
  `d4c39ffe293f5abd8d48af246cf400f7a1b95386cd613d3e3374723e48b176a2`
- `EngineControl.hpp.proposed` SHA256
  `74f63846fa9bd278fd8c02068238e1ba45b3538912493a7c869b99e7ded9717f`
- `EngineControl.cpp.proposed` SHA256
  `47d2bf15dd9897e57ff6bdce661807ec7fbc1bad54a056d3a651c2c445affad8`
- `binding_manifest.json` SHA256
  `cfc0f957b3dd9b35d9a996286a64b94ae53737784b06381a5140f662dd2b0597`
- `generate_b7_proposal.py` SHA256
  `a3f759ea947589feed632217d6280594389c8a521e20e0c682a5556a1821b8a4`
- `audit_b7_bindings.py` SHA256
  `c450f4bc1a195679cd2a927896bc791bf1c3a71f0dbd171ac124bdc2237abcd3`

All nine new member definitions are at most 68 clang-formatted lines including
their borrowed aliases. The 61-line `enter_phase` lambda is divided at its
existing surface-repair branch, bidirectional direction setup, and normal-entry
tail; each submethod remains under the 80-line limit.

## Future binding notes

The current candidate still has the synchronous Adapt-local
`observe_progress` lambda. `EnterPhase`, `EnterSurfaceRepair`,
`CompletePhaseEntry`, and `RollbackCheckpoint` take it by template reference;
they allocate/store/copy no callback. After B5, root may rebind these calls to
`EngineControl::ObserveProgress(context, curved, stage)`.

`CompletePhaseEntry` uses the provisional B6 method target
`context.ownership.PhaseOwnership(context, true, curved)`. Rebind this spelling
to the reviewed B6 API. The proposed methods take the original `curved` value
explicitly where required. These are integration dependencies, not hidden
claims that the pending B5/B6 APIs are already present in the base headers.

The token and reversal proofs are not compile evidence. This proposal is bound
only to the stated candidate B3 snapshot and must be regenerated/reviewed if
that source changes before integration.
