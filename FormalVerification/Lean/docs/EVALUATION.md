# Independent evaluation handoff

The user chose a frozen handoff for their evaluator. No agent-created reviewer, local
subprocess, test harness or second build is being represented as an independent evaluator.
The evaluator must operate under the user's authority outside the working agent's control.

## Freeze and approval

`SHA256SUMS` records every released source, specification, script and evidence file.
Its own digest is supplied with the handoff. The evaluator should preserve that digest
outside this directory before execution. Check the archive digest and then run
`sha256sum -c SHA256SUMS`. Hash agreement establishes identity, not correctness.

Read `SPECIFICATION.md`, `SU2/Spec.lean`, the actual theorem statements, `CORRESPONDENCE.md`
and `REPORT.md`. Record approval or rejection of their exact hashes in an evaluator-owned
record. There is no existing independent approval to assume. Do not permit statement,
definition, input-domain or acceptance-rule changes to count as fixes to a failed proof;
a changed contract requires a new version and a new approval decision.

## Reproduce under evaluator control

1. Extract the release into an evaluator-owned directory or isolated container/VM. Do not
   use the working agent's `.lake`, `.toolchain`, executable or `.olean` artifacts.
2. Obtain Lean `leanprover/lean4:v4.19.0` independently. Check the version, compiler commit
   and relevant platform checksums in `environment.json`; use the approved source release
   and trusted build process if binary provenance is insufficient for the threat model.
3. Keep `lean-toolchain`, `lakefile.toml` and `lake-manifest.json` unchanged. Resolve the
   exact manifest revisions. Network downloads are dependency acquisition, not proof of
   trustworthiness. Rebuild imported dependencies from their pinned sources for the
   stronger check instead of trusting the author's cache. A fresh project can use:

   ```sh
   MATHLIB_NO_CACHE_ON_UPDATE=1 LEAN_NUM_THREADS=2 lake build
   python3 scripts/verify.py
   ```

4. Independently inspect theorem types and run `#print axioms` or the pinned Lean
   `Lean.collectAxioms` API on every accepted declaration and its dependencies. The only
   approved logical axioms are propext, Classical.choice and Quot.sound. Do not rely solely
   on a working-agent-written audit. Kernel re-elaboration of all project sources is
   mandatory. An independently implemented kernel checker compatible with the pinned
   environment is an additional trust reduction; none was run by the working agent.
5. Verify actual source hashes and the historical repair diff against the SU2 Git objects.
   Read each source/model mapping and every excluded branch. Run the source experiment
   as supplementary evidence; reject any claim that it constitutes source semantics proof.
6. Recheck the counterexample and the refactor without changing the specification. Review
   the finite witness's admissibility, the unit-normal precondition, density increments,
   and the distinction between exact reals and floating point.
7. Publish the evaluator-owned result to the user: approved hashes, environment provenance,
   commands and outputs, accepted statements, assumptions, C++ evidence level, rejected
   claims, and remaining obligations. No external sending/posting is automated here.

## Fail conditions

Fail acceptance if a required theorem is missing/uncheckable; a forbidden axiom or proof
placeholder enters accepted dependencies; a specification is weakened, circular or
silently restricted; the environment/source identity is not the approved one; C++ or
floating-point correctness is overstated; an omitted branch/configuration is hidden; or
the working agent's own validation is substituted for independent final evaluation.

The negative audit fixture intentionally introduces a temporary unapproved axiom outside
the accepted source modules, expects rejection, and removes that fixture. It is not an
accepted proof dependency. The source extractor also self-checks missing/duplicate
boundary rejection. These checks test the local validation machinery; they do not replace
an evaluator-owned assessment.

## Compiler-generated runtime declarations

Lean 4.19 emits unsafe internal compilation artifacts such as
`SU2.edgeContribution._cstage1` and `Pi.instZero._elambda_1`. The first expanded audit
flagged these while inspecting every environment entry; their unsafe/internal status was
then checked against the pinned compiler. They are not the logical definitions or proof
dependencies. The final audit checks every safe SU2 logical declaration and the full axiom
closure of every theorem, rejects unsafe public declarations, and reports the count of
excluded internal runtime artifacts. No new axiom was added to the allowlist and no proof
dependency is exempted. Project source scanning separately rejects unsafe declarations.
This is another reason runtime/compiler correctness is not claimed from the logical proofs.

The local cold dependency build is retained in `evidence/dependency-source-build.log`;
its final failure is that audit classification issue, after all SU2 proofs had compiled.
The subsequent successful validation rebuilt every project module using those freshly
source-built, pinned dependencies. `verify.py` removes only the project build directory;
for a fresh dependency check the evaluator must begin with no dependency build artifacts.
