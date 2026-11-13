# Independent reproduction and acceptance

The evaluator must independently approve the proposed specifications and choose the frozen commit and candidate-lock SHA. Worker scripts/logs do not decide acceptance. Preserve a failed frozen candidate unchanged; propose corrections under a new identity. Independently inspect principal theorem types, dependencies and source correspondence in addition to running this worker-authored recipe.

Inputs:

- This small public interior-path reviewer ZIP: exact SU2 Git snapshot, formal projects, source selections, logs and manifest.
- Reusable source-only offline kit `SU2_Lean_evaluator_kit_20261112.tar.zst`, SHA256 `4fb743a739308b0b40b02255c2f636a681d311e039d339ab7d1abe04f740ed78`, provided by the earlier public Initialization release. No new 397 MB copy is necessary.
- Separately authorized private CFDLean Git source containing both M1 `0a88e36b3810cd5adc3881db526ca8a35f81afbb` and accepted M2b `c92c0f66360f85c0a3339ab9b30fc00c39ee5f93`. The new local private input is deliberately excluded from the public ZIP/release. The user requested keeping private input local. Arrange authorized evaluator access separately; no forwarding or receipt is assumed.

From the unzipped handoff:

```sh
python3 review/FormalVerification/InteriorPath/scripts/restore.py HANDOFF FRESH_OUTPUT \
  --historical-kit /authorized/source-only-kit.tar.zst \
  --cfdlean-source /authorized/CFDLean-M1-M2b.git \
  --expected-candidate-sha256 REVIEWER_SELECTED_LOCK_SHA
```

The restorer verifies source identities and checksums, restores the pinned Lean 4.19.0 distribution and dependency sources, and rejects supplied project/dependency `.olean` files. The official Lean bootstrapping distribution contains standard-library compiled files; pinned proofwidgets display assets are the non-proof exception. Parent projects keep a separate M1 checkout. This new project's root Lake graph explicitly pins M2b, whose relevant M1 definitions and toolchain/manifest bytes are unchanged. Both private inputs must be available; do not silently repin historical projects.

Inspect `restore-result.json` and execute `commands.txt` **sequentially**. The evaluator's initial build must rebuild mathlib, CFDLean and the SU2 projects from source without fetching proof caches, in a fresh environment outside the worker's control. Two Lean threads maximum. The busy-host exception was granted for this worker's Lean runs only; independent hosts choose their own policy. Native builds/runs retain the workspace load gate and run separately from Lean.

`check.py --preserved-checks` first replays the frozen preprocessing and earlier checks in their original dependency environment, then rebuilds this project, regenerates source selections, checks protected files/compiler/package identities and compares elaborated theorem types/axioms against the frozen inventory. It audits the union of all connected logical declaration dependencies, permits only propext/Classical.choice/Quot.sound, rejects an injected external axiom and attempts unchanged false semantic contracts. A compiler error, parser rejection or hash mismatch is not counted as numerical defect detection. Independently repeat principal dependency audits using evaluator-owned tooling.

Native commands rebuild unmodified pinned SU2, run the selected upstream unit tests, new actual timestep/geometry/interior methods, original/refactored and nine semantic source mutants, preserved initialization/preprocessing/boundary/update controls and selected full integration twice. Native actual-method tests retain the baseline release flags including `-ffast-math -fno-finite-math-only`. Source-fragment controls use `-O2`; neither is a floating-point or compiler proof. The stress100 case is expected to reach SU2's divergence guard and is reported separately from positive profile checks.

A warm local worker replay is:

```sh
# Export the pinned Lean 4.19.0 bin directory on PATH, then enter InteriorPath.
python3 scripts/check.py FRESH_RESULTS --allow-busy-two-threads --preserved-checks
```

This is supporting replay, not independent cold verification. The public kit contains no private CFDLean source, native executables or project/dependency proof caches. Source review can begin immediately; final independent kernel/native acceptance requires the separate authorized private source input and substantial cold-build resources.
