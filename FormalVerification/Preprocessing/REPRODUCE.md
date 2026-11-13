# Independent reproduction

The evaluator must first approve the proposed contracts and choose the frozen commit and `candidate.json` SHA independently. Do not accept worker output as external acceptance. Preserve a failed candidate unchanged and propose a separate correction. `scripts/check.py` is a convenience; use independently controlled theorem/dependency checks as well.

Required inputs:

- The small public preprocessing reviewer ZIP, including exact Git source snapshot, formal projects, evidence and manifest.
- The reusable **source-only** offline dependency kit `SU2_Lean_evaluator_kit_20261112.tar.zst`, SHA256 `4fb743a739308b0b40b02255c2f636a681d311e039d339ab7d1abe04f740ed78` (the earlier public Initialization release provides it).
- Separately authorized private CFDLean Git source containing M1 `0a88e36b3810cd5adc3881db526ca8a35f81afbb`. The optional local 36 KB bare-source input is kept private/local, not in this release. If forwarding is authorized later, its SHA256 is `29b2448e1061b0058e4e26665e30ac42afedce5dbd62b5f5100f506279c81a1a`. Extract `CFDLean-M1.git.tar.gz` locally and pass the resulting bare repository. No receipt or sharing is implied.

After unzipping, run the packaged script with independently chosen paths/identity:

```sh
python3 review/FormalVerification/Preprocessing/scripts/restore.py HANDOFF FRESH_OUTPUT \
  --historical-kit /authorized/source-only-kit.tar.zst \
  --cfdlean-source /authorized/CFDLean-M1.git \
  --expected-candidate-sha256 REVIEWER_SELECTED_LOCK_SHA
```

The restorer verifies hashes and Git identities, restores the pinned Lean distribution and dependency source trees, and rejects supplied project/dependency `.olean` files. Lean's official bootstrapping distribution includes its own standard-library compiled files. Proofwidgets display assets are the separately pinned non-proof exception. Inspect `restore-result.json`, then execute `commands.txt` sequentially. The cold build rebuilds mathlib, CFDLean and all SU2 projects from source. This is resource intensive; do not fetch Lean proof caches. Set max two threads; native tests retain the workspace load gate. Use `--allow-busy-two-threads` for Lean only if the host owner grants that exception (the user granted it for this worker).

`check.py --preserved-checks` also replays the frozen InitializationReplay and BoundaryMemory checks. It rebuilds the new project, regenerates source selections, checks pinned compiler/dependency/source identities, inventories elaborated theorem types, collects transitive axioms across all connected logical declarations, rejects an injected external axiom and runs semantic theorem-negation probes. Only propext, Classical.choice and Quot.sound are allowed. Independently inspect every principal theorem's type and dependencies; a successful regex scan is not an axiom audit.

The supplied native commands build unchanged pinned SU2, run selected upstream tests, actual preprocessing and existing boundary-memory/update method tests, preserved numerical controls and selected integration twice. Source-body controls run original/refactored bodies and require each mutant to compile before its numerical contract fails. Verify failure reasons; unsupported syntax, missing source, hashes and compiler errors are not semantic detections. Run native and Lean phases sequentially with the prescribed load/CPU limits.

For a warm worker replay with existing authorized dependencies, export the pinned toolchain bin directory on PATH, enter FormalVerification/Preprocessing and use `python3 scripts/check.py FRESH_RESULTS --allow-busy-two-threads --preserved-checks`. Its result explicitly records that local caches/dependencies do not establish independent cold acceptance.

The public archive includes no private CFDLean sources, no dependency `.olean` files and no native executable evidence in place of source/logs. It is suitable for source review immediately; full independent proof acceptance requires the separate authorized private input. Keep that limitation in the evaluation decision.
