# Upstream baseline and Euler composition milestone

This is a **candidate for independent review**, based on upstream develop
`d162016382f0c122433000c114e88d808b48f7a7`, with no production patches.
It records the externally accepted restricted-source gate and advances a real selected
Roe/explicit-Euler path. It does not verify the complete C++ solver.

Read [BASELINE.md](BASELINE.md), [SPECIFICATION.md](SPECIFICATION.md),
[PATH.md](PATH.md), [TRUST.md](TRUST.md) and [RESULTS.md](RESULTS.md).
The main results are `SU2Euler.masked_assembly_correct`,
`SU2Euler.composed_step_correct` and `SU2Euler.composed_source_cell`.
They reuse the frozen foundations and scalar source gate. The new project has 49 explicit
theorems, including parsing facts, migrated results, composition and semantic controls;
the number is not a count of independently verified C++ routines.

`../Lean`, `../SourceGate`, `../EvaluationKit` and prior acceptance files are preserved
byte-for-byte. The new attribution record is `../Acceptance/source-gate-20261112.json`.
The supplied external report is authoritative for that historical acceptance only.
The new worker-owned `candidate.json` freezes review inputs; it is not an approval.

## Reproduction

The portable handoff contains this review tree, a shallow Git snapshot with both upstream
and candidate commits, the original offline evaluator kit (official Lean release, exact
mathlib/dependency sources and native build dependencies), the historical source-gate
archive, and the supplied independent report ZIP. There are no worker-generated compiled
Lean caches or C++ binaries. The publisher's Lean compiler distribution is necessary;
its content fingerprint is checked, not asserted to be a signed publisher attestation.

On Linux x86_64, install Python 3.12+, Git, zstd, CMake and GCC/G++ (worker used 13.3.0).
The evaluator should independently select the expected candidate hash from the handoff
identity record and inspect/approve the specification before final acceptance.
From the extracted handoff root:

```sh
python3 review/FormalVerification/EulerPath/scripts/restore.py \
  . /absolute/new/review-directory \
  --expected-candidate-sha256 THE_REVIEWED_CANDIDATE_JSON_SHA256
```

This validates identities and restores source-only inputs. It writes `commands.txt`
with the exact sequential commands to build all three Lean projects cold, audit types
and dependencies, build upstream SU2, run the selected integration and all eleven native
source variants. Run those commands in order. Native builds/runs wait above load 6 and
use two build threads. The Lean checker accepts `--allow-busy-two-threads` only when the
host owner authorizes that exception; it still uses at most two threads.

For an already restored workspace with the pinned compiler on PATH:

```sh
python3 scripts/check.py /absolute/new/lean-results --allow-busy-two-threads
python3 scripts/build_baseline.py /absolute/SU2 /absolute/externals /absolute/new/native-build
python3 scripts/integration.py /absolute/native-build/SU2/build/SU2_CFD/src/SU2_CFD /absolute/new/integration
python3 scripts/native.py /absolute/new/native-controls
```

Only `check.py` is allowed the user-authorized load exception. Each command refuses an
existing output directory. Source identities regenerate from Git objects, not old fixture
copies. Positive/refactor controls must pass; mutants must fail for their documented
semantic reason. The axiom probe must be rejected transitively. Never count a timeout,
missing dependency or unsupported parse as success.

Final evaluation must use an evaluator-owned checker and approved specification outside
worker control. Reusing these scripts is a convenience, not independent acceptance.
