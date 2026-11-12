# Independent cold evaluation

The worker's successful build, audit and native tests are evidence, not final
acceptance. An evaluator outside this agent's control must choose the candidate
identity, approve the specification and run verification independently. This
handoff cannot establish that independence by invoking another worker process.

1. Obtain the archive hash and candidate-lock hash through the separately delivered
   record. Check the archive before extraction and approve the exact
   `SPECIFICATION.md`, `CORRESPONDENCE.md`, `PATH.md` and protected Lean statements.
   Record that approval in your own evaluator record, outside this checkout.
2. Extract the handoff to a new directory. Use its review copy's
   `FormalVerification/BoundaryPath/scripts/restore.py` with the extracted root,
   a fresh output directory and `--expected-candidate-sha256 YOUR_APPROVED_HASH`.
   This checks all payload hashes, restores Git source snapshots and official Lean
   4.19.0, and restores source-only mathematical dependencies. No project or mathematical-dependency `.olean` files
   or worker-built Lean cache are supplied; the pinned compiler distribution includes
   its standard library artifacts. Only the preserved publisher's
   ProofWidgets JavaScript assets are supplied, explicitly checked by inventory.
3. In a fresh process/environment, follow the resulting `commands.txt` sequentially.
   The new checker first runs the unchanged prior EulerPath checker, which also
   verifies the accepted foundations and source gate. It then checks the new source
   extraction, builds the new proofs, checks theorem types, audits transitive
   axioms and verifies parseable semantic mutants. The check is restricted to two
   CPU threads. The authorized owner may add `--allow-busy-two-threads` to Lean
   checking only; native work waits for one-minute load below 6.
4. Run the pinned baseline native build, actual-method and new fragment controls,
   preserved eleven native controls, and the two selected integration runs. They
   are separate from the Lean proof evidence. Native commands have timeouts and
   use at most two build threads; run them after Lean work has finished.
5. Audit independently: disallow unfinished proofs/new axioms; verify all imports,
   the complete `SU2Boundary` namespace and its transitive dependencies; allow only
   `propext`, `Classical.choice`, `Quot.sound`. Check theorem statements rather than
   relying on theorem counts. Confirm injected external bad-axiom rejection and
   that semantic negatives parse successfully and violate unchanged contracts.
6. Compare the entire preserved `FormalVerification` tree from commit
   `15ee28adf032a866e292926502a5f022e38199c8`, including the pending EulerPath lock
   `0a5781dad650c8de5bf24ef57804c6a422dbef3aa2be3f373b7ebc21371ccac6`.
   Do not interpret this preservation as acceptance of that pending candidate.
7. Publish your acceptance/rejection record with candidate commit/lock, specification
   hashes, compiler/dependency identities, cold-build provenance, commands/results,
   audit results and exact accepted coverage. Any change after freeze creates a new
   candidate identity and requires re-evaluation.

Compiler: Lean 4.19.0 (`6caaee842e94`), exact binary fingerprints inherited from
`../Lean/environment.json`. Mathlib:
`c44e0c8ee63ca166450922a373c7409c5d26b00b`. All other package revisions are in the
checked Lake manifest. The new `Real.rpow`/analysis imports can make a cold build
substantially longer than prior milestones. No network cache is required.

The archive contains a shallow Git snapshot spanning this candidate, the pending
EulerPath commit and the pinned upstream baseline. The existing offline evaluator
kit provides compiler, dependency sources and native dependency snapshots. Build
SU2 in the recipe's private clone, not a linked worktree, to avoid shared submodule
configuration changes. Default release fast-math flags are retained and disclosed.

The trust base includes the Lean kernel/compiler distribution, mathematical library
sources, operating system/hardware and the unverified extraction/build scripts.
The C++ interpretation, heap layout, compiler, floating-point operations and library
sqrt/pow are not established by these exact-real proofs.
