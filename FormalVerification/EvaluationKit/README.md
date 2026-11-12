# Phase 1 independent-evaluation supplement

This directory supplies the environment and source provenance missing from the evaluator's
2026-11-12 review. It does not make an independent acceptance decision. The adjacent
`../Lean/` is a byte-for-byte copy of the frozen project commit
`1c022ca8307517585a443b2609bddd873f31c46c`; no protected definition, theorem or acceptance
criterion has changed. Its original documents and evidence remain historical records.

The evaluator provisionally accepted the mathematical scope but could not run Lean in
its environment. Keep its JSON lock outside this checkout and pass it read-only to the
recipe below. The external lock and evaluator-owned report are not authored or modified
by these scripts. A fresh worker-run build remains local evidence.

## What is pinned

| Item | Pin |
|---|---|
| SU2 repaired commit | `e0d3ee8d72598767227e62a9be5834a4554532b6` |
| Immediate parent | `371fcae6f2d408f881f9db130f9feedd6a791e9d` |
| Repaired tree | `86084d611eaebd65c0d9216cf838eac56525e732` |
| Frozen archive SHA-256 | `30369d2f0eb53ae63227d8a4523bcba6949d692077b7ee072673c49f0542b480` |
| Lean release | Linux x86-64, Lean 4.19.0, compiler `6caaee842e94` |
| Official release archive SHA-256 observed here | `6fe3ce97a58f44e2b3567d455b994eacec5bfe9ae7774f2a573444480ba813fe` |
| Mathlib | `c44e0c8ee63ca166450922a373c7409c5d26b00b` |

`payload-manifest.json` pins all nine Lean dependency Git snapshots and the four production
build dependencies: Meson, Ninja, Eigen and MEL. Source is packed from Git objects, not
working directories. Dependency Git snapshots contain no worker-built `.olean` files or build caches.
The official Lean distribution necessarily includes its standard-library compiler objects;
this recipe does not bootstrap Lean itself.

Mathlib also requires ProofWidgets' display assets. `proofwidgets-v0.0.57-js.tar.gz`
contains only 19 JavaScript files and the original display build trace selected from the
[official pinned release](https://github.com/leanprover-community/ProofWidgets4/releases/tag/v0.0.57).
The upstream archive SHA-256 is
`fc6fa33a1272b7e2ec38a2b6427ea7645557e79a9338828ace91bf7403449f8e`.
Its compiled Lean objects are discarded; **all imported ProofWidgets Lean code is rebuilt**.
These display assets are not rebuilt from TypeScript in this recipe. Their publisher
trace avoids the otherwise mandatory offline JavaScript rebuild; it does not exempt any
Lean proof from checking. This separate non-proof asset dependency is recorded explicitly
in the payload manifest.

The [repair on the user's fork](https://github.com/rois1995/SU2/commit/e0d3ee8d72598767227e62a9be5834a4554532b6)
was also checked through GitHub's API. Its commit is unsigned. The
[official Lean release](https://github.com/leanprover/lean4/releases/tag/v4.19.0) exposes
no publisher digest for this asset in its API metadata. `provenance-metadata.json` records
those observations. Hashes establish content identity, not publisher signatures or trust
in the compiler. The evaluator must approve compiler provenance independently.

## Offline evaluation

Requirements: Linux x86-64 with compatible glibc, Python 3.12+, Git, GNU tar/coreutils,
zstd, a C++17 compiler, make and CMake. Reserve at least 10 GB of free space. The portable
payload is about 379 MiB and is distributed as a separate archive, not a large Git blob.
The release includes scripts, payload and checksums under `SU2_Lean_evaluator_kit/`.
Check the externally supplied archive digest first, then extract it in an evaluator-owned
VM or directory. From the extracted directory:

```sh
sha256sum -c SHA256SUMS
python3 EvaluationKit/self_test.py
unshare --user --map-root-user --net \
  python3 EvaluationKit/lean_check.py payload /absolute/path/new-lean-run \
  --lock /external/evaluator/SU2_Lean_phase1_evaluator_lock_20261112.json
```

The output directory must not exist. The Linux network namespace blocks downloads during
the check. Where user namespaces are disabled, use a network-disabled VM/container and
run the same Python command inside it. No special container image is required. Preserve
`payload-manifest.json` and the script checksums under evaluator control before running.

The Lean recipe checks the external archive/specification lock, every original checksum,
toolchain fingerprints, dependency and display-asset pins, SU2 tree membership and the parent repair
patch. It clears inherited Lean/Lake search paths and disables Lake build caches. It
restores clean sources, checks absence of compiled Lean dependency objects, installs only
the pinned display assets, and rebuilds
all imported dependencies and project modules with at most two CPUs/Lean threads, runs
the frozen transitive audit, exports all 52 explicit theorem types and axiom reports,
and confirms rejection of an injected axiom outside the accepted project. It finally
runs the extracted C++ statement experiment as supplementary evidence.

`--allow-busy-two-threads` bypasses only the load wait when the machine owner has authorized
it. The user authorized that option for Lean validation in this workspace. C++ builds and
runs retain the load-below-6 wait. Do not run the Lean and C++ recipes concurrently here.

Inspect `cold-build.log`, `axioms.log`, `theorem-types.log`, `negative-audit.log`,
`source-slice.log` and `result.json`. The script's audit is worker-authored: independently
inspect the exact theorem types and transitive dependencies using the pinned Lean APIs
or the evaluator's own checker. For example, after the build:

```sh
export PATH=/absolute/path/new-lean-run/lean-4.19.0-linux/bin:$PATH
cd /absolute/path/new-lean-run/SU2_Lean
lake env lean /external/evaluator/OwnAudit.lean
```

Only the external evaluator records acceptance. Neither exit zero nor the existence of
`result.json` constitutes that decision. The required criteria remain those in the
unchanged `../Lean/docs/EVALUATION.md`.

## Production-source evidence

Run separately in the same network-disabled environment:

```sh
unshare --user --map-root-user --net \
  python3 EvaluationKit/wall_test.py payload /absolute/path/new-wall-run
```

This builds the full production unit-test driver from a new standalone Git checkout with
private metadata. It never calls submodule initialization in a shared worktree. Exact
Meson/Ninja/Eigen/MEL sources are installed first; SU2's preconfigure is run with dependency
acquisition disabled. CMake builds pinned Ninja, and pinned Meson builds the test driver
with `ninja -j2`. Production code and its default fast-math flags remain unchanged.

The selected existing test is `[HLLC-wall]`: 2D/3D ideal-gas and Peng–Robinson cases,
old state, residual, diagonal/neighbor Jacobians, truncation error and multigrid state
versus correction. This is supplementary C++ coverage, broader than the ideal-gas
Lean proofs. It does not extend their formal coverage to real gas, implicit integration
or multigrid execution paths.

The negative control substitutes the exact parent production header in this disposable
checkout while retaining the repaired test and registration. The script verifies that
these are the only differences from the parent, then rebuilds and runs the test with
`--abort` to stop on the first failed assertion. It restores the repaired source in a
`finally` block. The retained binary after this control is the **parent** binary; both
binary digests and this fact are recorded. Do not mistake it for the repaired executable.
Build commands, compiler version, complete compile command database, options and both
run logs are retained. No numeric reference values or test assertions are edited.

The offline SU2 repository includes the repaired commit, its immediate parent, and both
complete trees/blobs. It is deliberately shallow at the parent: older ancestry and unused
submodule contents are omitted. All 11 mapped source blobs, the before-header, tree,
parent and full repair patch are checked against these objects with `git fsck` and Git
object identifiers. Each dependency repository is shallow at its selected commit. The
fork branch supplies a separate online route to the source history.

## Recreating the payload

`prepare.py` reads existing Git object stores without changing them. Obtain the frozen
archive, the official Lean release and the pinned dependency source repositories, then:

```sh
python3 EvaluationKit/prepare.py \
  --project /path/to/frozen/SU2_Lean \
  --su2 /path/to/SU2 \
  --externals /path/to/SU2/externals \
  --archive /path/to/SU2_Lean_phase1_20261112.tar.gz \
  --lean-release /path/to/lean-4.19.0-linux.tar.zst \
  --proofwidgets-release /path/to/ProofWidgets4-v0.0.57.tar.gz \
  --output /path/to/new-payload
```

The project argument supplies the exact clones at `.lake/packages/` from its manifest;
compiled files in those clones are never packed. Git object identity is reproducible;
compressed transport timestamps may differ. Repacking therefore creates new transport
hashes: review the resulting manifest before explicitly adopting it as
`EvaluationKit/payload-manifest.json`. Do not replace an evaluator's accepted lock or
original frozen archive. The check recipes intentionally reject unreviewed manifests.

## Remaining obligations

| Item | Status |
|---|---|
| Frozen mathematical definitions and 52 explicit theorems | Preserved unchanged; original local proofs recorded; see local rerun evidence |
| External mathematical-scope decision | Provisional acceptance in evaluator-owned record |
| Independent Lean rebuild and transitive audit | Still requires evaluator execution and assessment |
| Source commit/tree/parent membership | Checked against content-addressed Git objects; no authorship signature claim |
| Fresh production moving-wall test | See `LOCAL_RESULTS.md`; finite numerical evidence |
| Formal source-to-model correspondence | Open; manual models and source-slice tests are not a semantics proof |
| Floating-point correctness / compiler correctness | Open; exact-real proofs do not establish these |
| Semantic source mutations reaching the Lean model | Open; digest rejection alone is not semantic rejection |
| Production refactor correspondence | Existing source-slice experiment and exact-real model proof only |
| Complete Euler residual/update path | Future milestone; see `../Lean/docs/DEPENDENCIES.md` |

The maintained local `p1fix_bc` branch has advanced since the historical pin (observed
at `c512963931` on 2026-11-12). Later changes include positivity, moving-mesh P1 boundary
closures and wall-force work. This branch intentionally remains based on `e0d3ee8d72`;
porting the source/model mapping to the maintained branch requires a separate review.
No claim is made about its current paths or a whole solver.

The evaluator report's Markdown contains a transcription error in the specification
hash. Its JSON lock agrees with the actual frozen file: `docs/SPECIFICATION.md` is
`9843d9f62e2a08a4f40fa543d32616faa51773ef3a7a3b6d29bfd7451ce36953`.
The external report has not been edited.

Next: evaluator acceptance of this unchanged contract/environment; then select one
bounded source routine and establish source-connected sequential semantics, semantic
mutation rejection and refactor correspondence. Add an explicit Galilean-conjugacy
lemma under the approved wall contract if the evaluator requests it. Continue with the
flux, mesh traversal, boundary budget and explicit update obligations already sequenced
in `../Lean/docs/DEPENDENCIES.md`.
