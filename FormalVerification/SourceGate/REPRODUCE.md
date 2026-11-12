# Reproducing and independently evaluating the candidate

Use an evaluator-controlled environment and preserve an external copy of `candidate.json`
and the handoff archive digest before running anything. Review/approve the new candidate
specification separately; the recorded Phase 1 acceptance does not approve these additions.
Do not replace the original evaluator's locks, reports or approved files.

Requirements: the pinned Linux x86-64 Lean 4.19.0 environment, Python 3.12+, Git, g++ with
C++17 support, and the source Git object `e0d3ee8d72598767227e62a9be5834a4554532b6`.
The native test intentionally uses serial passive double, LP64, `-O2 -DNDEBUG` and no
fast-math option. It is a scoped source build, not the production solver build configuration.

## Reuse the independently rebuilt environment

The existing `SU2_Lean_evaluator_kit_20261112.tar.zst` has SHA-256
`4fb743a739308b0b40b02255c2f636a681d311e039d339ab7d1abe04f740ed78`.
It already supplies the pinned compiler, dependency Git objects and source baseline.
Follow its unchanged [cold-build recipe](../EvaluationKit/README.md) in a network-disabled
VM/namespace if a fresh independently checked environment is not already available.
That rebuild installs only the pinned ProofWidgets JavaScript display assets and compiles
all imported Lean dependency sources; supplied dependency `.olean` files are not used.
Keep the external evaluator's original lock read-only.

The cold-build output contains `SU2_Lean`, `SU2-source` and `lean-4.19.0-linux`.
Extract the new source-gate handoff archive into that `SU2-source` checkout. The archive
contains `FormalVerification/` with unchanged `Lean/` and `EvaluationKit/`, the acceptance
attribution, and this new candidate. It contains no `.lake` directories or compiled proofs.
Alternatively, check out the user's `formal/lean-source-update` fork branch; its Git history
contains the same pinned source commit.

Point this candidate's dependency directories at the already independently rebuilt
sources (or restore the exact same pinned Git snapshots into those directories):

```sh
export SU2_GATE_REPO=/absolute/path/new-lean-run/SU2-source
export SU2_FOUNDATION=/absolute/path/new-lean-run/SU2_Lean
export PATH=/absolute/path/new-lean-run/lean-4.19.0-linux/bin:$PATH
python3 - <<'PYSETUP'
import json, os
from pathlib import Path
foundation = Path(os.environ['SU2_FOUNDATION']).resolve()
gate = Path(os.environ['SU2_GATE_REPO']).resolve() / 'FormalVerification/SourceGate'
packages = gate / '.lake/packages'
packages.mkdir(parents=True, exist_ok=True)
for package in json.loads((foundation / 'lake-manifest.json').read_text())['packages']:
    (packages / package['name']).symlink_to(foundation / '.lake/packages' / package['name'], target_is_directory=True)
PYSETUP
cd "$SU2_GATE_REPO/FormalVerification/SourceGate"
unshare --user --map-root-user --net \
  python3 scripts/check.py /absolute/path/new-gate-run
```

The output directory must not exist. If user namespaces are unavailable, run the same
Python command in a network-disabled VM. Do not run multiple recipes concurrently on a
shared machine. The script limits affinity and Lean to two CPUs/threads, checks load before
each build/run, and waits below load 6. The machine owner may authorize
`--allow-busy-two-threads` for Lean only; native compilation/execution still waits. Native
commands have timeouts. There is no network dependency acquisition in this script.

The script checks compiler fingerprints, exact dependency revisions and clean tracked
sources, all frozen Phase 1 checksums and candidate protected-file hashes. It regenerates
source fixtures and Lean raw-text definitions from the Git-pinned header and compares
bytes to the frozen candidate. It deletes only this gate's `.lake/build`, rebuilds its
modules, audits the entire `SU2Source` logical declaration dependency closure, exports
the explicit theorem types, and confirms rejection of an injected axiom outside the
accepted library. Existing dependency artifacts are reused in this step; cold rebuilding
them is the preceding Phase 1 environment step, not a claim made by this script.

The same unchanged `WitnessContract` and proof attempt are then submitted for all five
source fixtures. The three expected failures happen in the execution equality, after
successful parsing. Separately accepted counterexample theorems establish that no correct
proof of those mutant contracts exists. The parser's unsupported-input tests also compile.

Finally, the native script archives the actual source from Git into the output directory.
For each source fixture it changes only the selected method in that disposable header,
checks the full header hash, verifies preprocessed-method token agreement, compiles the
real class/container, and runs the fixed native test. Original and refactor must each
pass 20,412 bitwise checks; the three mutants must fail their value/frame contract.
The disposable source is restored to original on successful completion; each binary is
named by its variant. No user's production checkout is modified.

## What the evaluator should inspect

- `candidate.json`, `SPECIFICATION.md`, `Semantics.lean` and `Parser.lean`: meaning and
  scope, particularly valid-memory assumptions and the distinction between an arbitrary
  scalar addition function, real addition and actual floating-point arithmetic.
- `source-manifest.json`, regenerated fixtures, preprocessed methods and the pinned
  container/accessor/type source: confirm the boundary is selected and interpreted correctly.
- `build.log`, `audit.log`, `types.log` and `TheoremTypes.lean`: actual theorem types and
  transitive axioms. The inventory is 30 explicit theorems and 318 logical declarations,
  including 105 theorem declarations generated or written by Lean. Only `propext`,
  `Classical.choice` and `Quot.sound` are allowed. Internal unsafe compiler-runtime
  artifacts are reported separately and exempt no logical dependency.
- `negative-audit.log`, five `*-contract.lean`/logs and the three counterexample theorems:
  inspect failure causes rather than accepting nonzero exit codes alone.
- `native-*-build.log`, `preprocess-*.log`, `native-*.log` and `result.json`: native source,
  compiler options and the sampled values. These are supplementary evidence only.

Run an independently authored axiom/type audit with `lake env lean` and assess the open
correspondence obligations in `CORRESPONDENCE.md`. Keep the new decision outside the
worker's repository. `result.json` records worker-script checks, not independent acceptance.

## Frozen results

`evidence/` records one worker run and its logs; these are reproducibility aids, not
substitutes for evaluator execution. The old Phase 1 archive and its evaluator kit retain
their original hashes. The new handoff has its own SHA-256 sidecar and candidate lock.
