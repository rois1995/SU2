# External evaluator recipe

The evaluator must choose and approve the specification/identity independently.
The working agent's successful runs are supporting evidence only. Approval of
prior BoundaryPath cold Lean, dependencies and full native checks is still pending;
the new result does not retroactively accept that candidate.

## Frozen inputs

The delivery manifest gives the exact new commit, tree, candidate-lock SHA-256 and
all handoff file hashes. Select the expected lock from a trusted copy of the
handoff identity before restoring. The Git snapshot contains the pinned numerical
baseline, preserved EulerPath/BoundaryPath/investigation chain, new candidate and
observed upstream develop comparison. Only `FormalVerification/BoundaryMemory`
changes in the new commit. No production patch is included.

Lean is 4.19.0, with mathlib at
`c44e0c8ee63ca166450922a373c7409c5d26b00b`; all dependency revisions and official
binary fingerprints are inherited unchanged. The historical offline payload
contains source-only mathematical/native dependencies and the official Lean
release. It supplies no project or dependency `.olean` cache. Proofwidgets display
assets are separately hash-pinned and are not Lean proofs. No CFD_Lean dependency
is imported while its approval and relevant specifications remain pending.

## Restore and execute

From the unpacked delivery root, using a fresh output directory:

```sh
python3 review/FormalVerification/BoundaryMemory/scripts/restore.py \
  /absolute/path/to/handoff /absolute/path/to/fresh-evaluation \
  --expected-candidate-sha256 REVIEWER_SELECTED_LOCK_SHA256
```

The restorer checks the bundle, Git identities, compiler fingerprints and source
pins, then writes `commands.txt`. Run those commands **sequentially**. They perform:

1. All preserved Lean/source/integrity checks, then the new project, theorem-type
   inventory, transitive axiom audit and eight semantic negative proof attempts.
2. A private standalone build of the unchanged upstream source and selected unit
   tests; it does not initialize submodules in a shared worktree.
3. Actual-method boundary-to-memory/update tests and new compiled mutations;
   preserved boundary, scalar, assembly and update controls; two full configured
   twenty-iteration integration runs.

Before every build/run the scripts print host load and wait above 6. All use at
most two CPU cores, `LEAN_NUM_THREADS=2`, `OMP_NUM_THREADS=1` and native `-j2`.
Only when authorized by the host owner, pass `--allow-busy-two-threads` to the Lean
checker. This does not override native load gating. Runs have timeouts. Ensure
Python 3.12+, Git, zstd/tar, CMake and a C++17 compiler are available. No source
or mathematical dependency download is required after restoration.

For a stricter cold evaluation, use an evaluator-controlled container/VM with
network disabled and independent compiler/kernel validation. Verify the restore
reports zero supplied `.olean` files and inspect the allowed path dependencies.
The checker clears inherited Lean/Lake paths, disables cache fetching, limits Git
protocols, and rebuilds each candidate's project artifacts. Source-only restored
mathlib dependencies are built by Lake. Worker development used pinned dependency
caches; that is explicitly not an independent cold run.

## Independent review targets

Read `SPECIFICATION.md`, `CORRESPONDENCE.md`, `PATH.md`, `coordination/status.json`
and the printed theorem types, not merely the checker success message. Inspect
`source_boundary_to_complete_update` for pre-boundary-only memory premises and
verify that `InputsReady` is derived. Review current/old separation, the 11-column
layout, all untouched cells, repeated visits, ownership, ordering and halo frames.

Run an independent axiom/dependency audit. The worker audit permits only `propext`,
`Classical.choice` and `Quot.sound`; it checks all logical declarations in the new
namespace transitively. An injected external bad axiom must be rejected. No
unfinished proof, new axiom or native decision shortcut is accepted. Parser/hash
failures must not substitute for numerical negative controls: all eight contract
trials must reduce to `False`, and all eight C++ mutants must compile successfully
before returning the specified numerical-contract failure.

Review the scalar boundary Roe/characteristic translation against Agent A's
literature-approved requirements when available. Neither the old comment wording
nor a successful model proof establishes literature approval. Do not treat source
hashes, native tests or the manual translation as certified C++ semantics.

Keep evaluator output outside the candidate. `worker-validation/` contains only worker results, generated
source and intentionally failing negative probes; it is not an acceptance report
and those probes are not imported proof dependencies. Record approval, findings, chosen
identity and scope in a separate signed report; never modify the frozen files to
make a test pass. A changed specification or implementation requires a new candidate
and explicit review, preserving this one.
