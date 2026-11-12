# Initialization replay correction — separate candidate

The frozen mathematical/source candidate remains d3aea3de43f6aefbc4dce7a09fc399c367230b9d,
with original lock SHA-256 1257f65d1c1ce63ef98c9f4c29d89581efb5ff82e9a9cc6a301e092cd1a909bf.
Every Initialization and earlier file remains unchanged. This follow-up adds only
separately pinned replay tools; it proposes no mathematical requirement change.

Worker replay found an inventory-tool defect: the unanchored regular expression
read the phrase "theorem does" in a comment as a declaration. All Lean source built
and the transitive kernel audit passed; theorem-type printing then correctly failed
on the nonexistent SU2Initialization.does. The failed run is retained in the handoff.
The original README/RESULTS/lock's count of 63 explicit declarations is therefore
incorrect: there are 62 actual explicit declarations. No genuine declaration was
removed. The audit still reaches 299 initialization/CFDLean logical declarations,
including 159 theorem declarations (with generated auxiliaries).

verify.py anchors theorem discovery to declaration lines, checks the exact corrected
inventory and explicitly checks that its only discrepancy from the original inventory
is the nonexistent comment token. The complete kernel namespace audit remains
unchanged and covers all logical declarations transitively, including CFDLean.
Specifications, Lean definitions/proofs, numerical contracts and mutation controls
are untouched. This replay correction is itself subject to external review.

## Reproduce this handoff

Read ../Initialization/README.md, SPECIFICATION.md, SOURCE.md, PATH.md and REPRODUCE.md
for scope and private/reusable dependencies. The principal result has an explicitly
OPEN interior/timestep contract and is not complete Euler-path verification.

Use this directory's restore.py, adding BOTH independently selected locks:

```sh
python3 review/FormalVerification/InitializationReplay/restore.py \
  /absolute/handoff /absolute/fresh-evaluation \
  --historical-kit /absolute/SU2_Lean_evaluator_kit_20261112.tar.zst \
  --cfdlean-source /absolute/authorized/CFD_Lean \
  --expected-candidate-sha256 1257f65d1c1ce63ef98c9f4c29d89581efb5ff82e9a9cc6a301e092cd1a909bf \
  --expected-replay-sha256 REVIEWER_SELECTED_REPLAY_LOCK
```

Run the FINAL commands.txt printed by this wrapper. It changes only the checker
entry point to InitializationReplay/verify.py; all source/native commands and cold
restoration controls remain in place. Do not execute the original failing checker
and treat its failure as a proof failure. A direct prepared-project replay is:

```sh
cd SU2/FormalVerification/Initialization
python3 ../InitializationReplay/verify.py /fresh/results --preserved-checks
```

Worker validation of the correction reuses the already successful preserved check
chain from the failed worker attempt. It reruns the new clean build, type inventory,
axiom and bad-axiom checks and every negative proof attempt. Independent evaluation
must execute the full corrected cold recipe. Final kernel/full-native acceptance,
preprocessing-memory, geometry, SIMD Roe and actual timestep proofs remain pending.
CFDLean M1 is accepted and pinned; Roe/far-field literature approval remains pending.

The new native tests compile unchanged method bodies in mock storage: original
and refactor each pass 500 checks and seven numerical mutants compile before failing.
They supplement the 62 actual Lean declarations and do not prove production C++.
