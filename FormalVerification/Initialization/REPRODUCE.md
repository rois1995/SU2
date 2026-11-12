# Independent evaluator recipe

The candidate and earlier BoundaryMemory identity must remain frozen. The reviewer
independently selects the specification and expected lock, inspects the premises,
and makes the acceptance decision. Worker logs are supporting evidence only.

## Inputs

- This new small handoff ZIP: Git snapshot, complete public formal sources,
  source mappings, mathematical contracts, worker evidence and file hashes.
- Reusable source-only kit `SU2_Lean_evaluator_kit_20261112.tar.zst`, SHA-256
  `4fb743a739308b0b40b02255c2f636a681d311e039d339ab7d1abe04f740ed78`.
- Authorized local Git repository containing accepted private CFDLean commit
  `0a88e36b3810cd5adc3881db526ca8a35f81afbb`. An authorized reviewer can clone
  `https://github.com/rois1995/CFD_Lean.git`. Public assets exclude private sources.

The earlier full BoundaryMemory ZIP contains the reusable kit. Supplying this
exact archive avoids downloading a second copy. Lean 4.19.0 and all mathlib/source
pins remain unchanged. No project or mathematical dependency compiled artifacts
are supplied by the new package/restoration. Pinned ProofWidgets display assets
are not proof artifacts.

## Restore

From the unpacked handoff, choose fresh paths and run:

```sh
python3 review/FormalVerification/Initialization/scripts/restore.py   /absolute/path/to/handoff /absolute/path/to/fresh-evaluation   --historical-kit /absolute/path/to/SU2_Lean_evaluator_kit_20261112.tar.zst   --cfdlean-source /absolute/path/to/authorized/CFD_Lean   --expected-candidate-sha256 REVIEWER_SELECTED_LOCK_SHA256
```

The restorer checks all bundled identities and official Lean fingerprints, restores
only source dependencies, checks out the exact private M1 revision, and prints and
writes commands.txt. Execute those commands sequentially. They run the previous
frozen check chain, new clean candidate build, theorem inventory/type printing,
transitive axiom audit, injected bad-axiom rejection and seven numerical false
postconditions. Then run the private unchanged SU2 build, new source-body controls,
preserved actual boundary-memory/native controls and two full integration runs.

Required host tools: Python 3.12+, Git, tar/zstd, CMake, a C++17 compiler and basic
build utilities. Dependencies and Lean are restored from the kit without network.
Only the authorized CFDLean source repository must already be available locally.
Use an evaluator-controlled offline VM/container for cold acceptance, inspect
restored paths and verify zero supplied .olean files before compiling. LAKE_NO_CACHE
and MATHLIB_NO_CACHE_ON_UPDATE are enabled; source-only mathlib must rebuild.
Worker validation used pinned warm dependency artifacts and is not this cold run.

The scripts print host load before work, limit affinity and Lean to two cores and
native to one process at a time, and wait above load 6. The user's earlier busy-host
permission applies to worker Lean only; evaluators obtain their own host-owner
permission before using --allow-busy-two-threads. Never run native and Lean builds
concurrently. All native tests have timeouts.

## Review targets and integrity

Review the unchanged source-level decision separately from pending kernel/native
acceptance. Check every theorem type and independently audit transitive dependencies
from all logical declarations in SU2Initialization and CFDLean, not merely a fixed
list of top-level theorem names. Only propext, Classical.choice and Quot.sound are
allowed; the injected external bad axiom must fail. Negative proof attempts must
fail with the goal False, and numerical C++ mutants must compile before failing
the numerical contract. Compiler/parser/hash failures are not semantic detection.

Inspect snapshot_interior_boundary_update: its open interior/timestep relation is
an unresolved execution interface. Do not treat the primitive point proof as a
completed preprocessing-memory proof or native mocks as full C++ class validation.
Review the M1 compatibility map against the approved revision; Roe/far-field remain
provisional. Keep all evaluator output outside the candidate and issue a separate
report recording chosen identities, scope, failures and acceptance status. Any
correction needs a new candidate, preserving this one.
