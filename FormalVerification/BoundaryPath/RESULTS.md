# Worker results and evaluation status

The new candidate is ready for a frozen identity check and external evaluation.
The candidate lock and delivery record identify its exact source/proof identity;
this report does not accept its own specification. The earlier EulerPath candidate
remains pending and unchanged.

| Evidence | Worker result | Scope |
|---|---|---|
| New Lake project | PASS: 81 explicit theorems | Includes parsed-source results, boundary contracts/refinements, complete owned-point/component memory refinement and semantic controls |
| New transitive axiom audit | PASS: 427 logical declarations, 200 theorem declarations | Only propext, Classical.choice, Quot.sound; 434 internal compiler-runtime artifacts excluded, no logical proof dependencies exempted |
| External bad-axiom probe | REJECTED as required | A new theorem depending on an unapproved external axiom is detected transitively |
| Preserved Lean candidates | PASS unchanged checker | 52 foundation, 30 source-gate and 49 EulerPath explicit theorems, preserved audits and controls |
| Baseline C++ build/unit tests | PASS | Pinned upstream, no production patches; 545 assertions in six selected unit cases |
| Actual C++ boundary/update methods | PASS: 7,747 checks | 20 mesh points, seven far-field regimes including sonic/zero cases, 55 owned far visits and 69 owned wall visits; cache/buffer frames and all update cells |
| Additional scalar boundary Roe checks | PASS, included above | 100 oblique/axis-normal state pairs plus the nonpositive-c² return branch; separate from interior SIMD Roe |
| New native fragment controls | PASS | Original and prefix-increment refactor pass; seven numerical mutations fail unchanged contracts |
| Preserved native controls | PASS | All eleven original/refactored/semantic-mutant variants produce their required outcomes |
| Selected full Euler run | PASS, twice | Same pinned configuration/mesh, twenty iterations; identical final conservative states and finite, observed admissible final states |

The full new frozen-check execution is recorded in `evidence/lean/result.json`.
It reruns the unchanged prior checker, source regeneration, fresh candidate build,
type inventory, axiom audit, bad-axiom probe and semantic rejection probes. Its
success is required before commit/delivery. Native results and full relevant logs
are in `evidence/native`, `evidence/preserved-native`, `evidence/integration` and
`evidence/baseline`. Build artifacts and compiled mathematical caches are not shipped.

## Semantic experiments

The approved-for-review contracts are unchanged across controls:

- Add versus subtract boundary flux: `FluxAccumulationContract`.
- Wrong-sign projection in each of old, residual or truncation: `ThreeBufferContract`.
- Last-point and energy-component omission: `LoopWitnessContract`, an instance of
  the full update/frame contract. `missing_cell_rejected` proves that executing a
  list missing a required cell leaves an incorrect initial current value unchanged.
- Timestep getter reading LocalCFL: `TimestepContract`, with a whole-update native
  witness as well.

Every mutation is accepted by its declared parser and remains valid C++ in the
source-fragment fixture. Rejection is semantic. Lean proves the negation of each
mutated contract; supplemental failed proof probes reduce that same contract to
False using the checked counterexample. Unknown syntax rejection has separate
controls and is not counted as semantic defect detection.

The new refactor changes postfix to prefix increment where the expression value
is discarded. Its parsed traversal order is proved identical, and the unchanged
native contract passes. It is a fixture experiment, not a production patch or a
verified compiler theorem. All previous refactoring controls remain intact.

## Trust and limitations

The boundary model is closed: no arbitrary finish/boundary-flux function remains
in the new composed theorem. It accounts for residual, old, truncation and ghost
cache, preserves production far-field-before-wall order and uses the selected
explicit Euler source arm. It also carries stored interior enthalpy explicitly;
its agreement with the ideal-gas formula remains a preprocessing precondition.

The complete update result concerns the numerical buffer loop. Actual heap/object
correspondence, surrounding diagnostic/communication effects, source dispatch,
vertex/edge traversal, interior SIMD Roe, geometry, initialization and finite
floating-point execution remain unresolved. Scalar boundary eigenmatrix entries
are manually represented and natively compared; their complete C++ translation
and mathematical eigensystem derivation are not certified. Exact-real algebra
never implies machine floating-point correctness. Release fast-math flags are
retained for the actual-method native binary.

Development used pinned existing mathematical artifacts and compiled the newly
needed analysis dependencies from pinned source. It is not an independent cold
build. Source-only offline restoration and a pinned compiler are supplied for the
external evaluator; final acceptance must occur outside the working agent's control.

An early extra proof-failure probe hit the elaborator heartbeat limit while trying
to unify unequal real arithmetic contracts. It was not counted as a semantic
rejection. The final probe uses the already kernel-checked counterexample and
requires an explicit False goal. The contract and counterexample theorem were
not changed. A separately terminated outer development command was rerun; its
incomplete output was not treated as a successful check.
