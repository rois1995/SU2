# Worker validation results — 2026-11-12

**Local checks passed. New independent acceptance is pending.**
Baseline: upstream `d162016382f0c122433000c114e88d808b48f7a7`, zero production patches.
Frozen candidate SHA256:
`0a5781dad650c8de5bf24ef57804c6a422dbef3aa2be3f373b7ebc21371ccac6`.
The portable handoff's manifest identifies the exact publication commit.

| Check | Outcome |
|---|---|
| Historical preservation | All 144 tracked FormalVerification files from accepted commit d156abf38 are byte-identical; prior archives/pins preserved. |
| Lean environment | Lean 4.19.0 / compiler 6caaee842e94, binary fingerprints checked; mathlib c44e0c8 and all nine package pins verified. |
| Builds | All three Lake projects build; EulerPath rebuilt from a clean project build directory. Existing pinned dependency artifacts were reused for worker validation. No claim of a fresh worker cold mathlib rebuild this milestone. |
| Accepted foundation audit | All 52 explicit theorem types exported; 231 logical declarations / 113 theorem declarations checked. |
| Historical source-gate audit | All 30 explicit theorem types exported; 318 logical declarations / 105 theorem declarations checked. |
| New composed project audit | 49 explicit theorem types exported; 192 logical declarations / 95 theorem declarations checked. Only propext, Classical.choice and Quot.sound in transitive proof dependencies. |
| Negative audit | An external unapproved axiom used by a theorem in the audited namespace was rejected transitively. Probe is outside accepted dependencies. |
| Source regeneration | Fresh Git extraction matches frozen manifests/raw Lean text/fixtures exactly; scalar AddSolution body remains unchanged. |
| Migrated scalar semantic trials | Original and refactor pass the same contract; subtraction/current-read/wrong-target variants parse and fail at the modeled execution equality. |
| New semantic controls | Positive/refactor contract proved; four parsed mutants formally disproved under the same composed contract. |
| Upstream C++ build/unit tests | GCC 13.3.0, serial release, selected flags recorded; 545 assertions across six vectorization/linear-algebra cases pass. |
| Native scalar gate | Actual inherited original and refactor each pass 20,412 bitwise destination/frame checks; three mutants fail with their intended wrong values. |
| Native composition | Actual SIMD UpdateBlocks plus extracted apply/scale/Euler-arm statements and inherited scalar update: original and refactor each pass 3,312 residual/destination/old-state checks. All Boolean masks for 2, 4 and 8 lanes tested; actual host width is two. Four mutants fail semantically. |
| Full selected integration | Two 20-iteration runs on the 20-node/24-triangle case, actual Roe/explicit Euler dispatch, no Jacobian; identical restart and history files. Final density minimum 0.9968326336627771 and internal-energy-density minimum 19.75401209085727 (nondimensional). This is observation, not a general positivity theorem. |
| Refactor experiment | Scalar local-variable refactor and scatter local-array renaming retain their mathematical contracts. Original/refactor binaries also match on this compiler/build of the fixtures; no production code was refactored. |

Native composed mutants produce: wrong right sign residual 9 versus 3; wrong mask residual
5 versus 8; wrong truncation state 18 versus 17; wrong Euler sign state 23 versus 17.
The proofs likewise distinguish the algorithms by semantics, not a checksum/parse failure.

The main reusable results are masked assembly correctness, composed local-step refinement,
source-memory cell execution/frame correctness, proved caller bounds, local-time balance,
and the carefully restricted uniform-time interior conservation specialization. The
unequal-local-time counterexample prevents extending the last claim without its assumptions.

Development corrections did not weaken any theorem: the build helper's Python import
context was fixed, the new parser's syntax/token support was completed, and the integration
checker was corrected to read SU2's actual `restart.csv` filename. The first frozen worker
checker missed eight attributed historical theorems; it failed its mandatory count of 52.
`CANDIDATE_REVISIONS.json` preserves that withdrawn candidate identity and the exact
collector-only correction. The accepted checker, all specifications and all theorem
statements/proofs remained unchanged. The replacement passed the complete validation.

Full commands, compiler options, logs, theorem types, transitive axiom reports, semantic
trials, measured integration data and evidence hashes are under `evidence/`. Negative
trial/probe files there are deliberately excluded from the accepted library dependency
graph. Large generated preprocessed translation units and binaries are reproducible from
the scripts and retained by digest, not shipped as trusted proof inputs.

Still open: boundary joint transformation (`finish`, post-boundary old state/truncation),
Roe flux, geometry/enumeration, initialization and complete loop/heap correspondence,
C++ language/compiler semantics, IEEE-754/fast-math, and the complete configured Euler
path. Unsupported solver configurations are listed in PATH/BASELINE. No whole-solver
formal verification or final independent acceptance is claimed.
