# Implementation and validation cadence

User instruction, 2026-10-12: prioritize substantial, connected implementation
steps over repeated small checkpoints.

- Group changes that solve the same end-to-end capability before running a broad
  regression. Do not require the full suite after each small edit or checkpoint.
- During implementation, run focused tests when they resolve a concrete failure,
  exercise new risky behavior, or validate a changed interface. Documentation and
  routine low-impact edits do not need a new test campaign.
- Run appropriate broader regression at meaningful integration milestones and
  before declaring the integrated result validated. Never weaken acceptance gates
  or omit necessary correctness checks to satisfy this cadence.
- Reserve MPI campaigns for substantial connected implementation milestones; do
  not repeat them for each small local edit. Use focused serial checks during
  development where they resolve a concrete risk.
- Push reviewable checkpoints regularly. Clearly label incomplete or unvalidated
  work; a checkpoint does not itself require repeating a previously passing suite.
- Updates should state progress toward the actual end-to-end goal and its remaining
  blockers, rather than treating each passing intermediate test as a milestone.

## Capability-first implementation

Permanent user instruction, 2026-10-14; applies across context compaction and
future sessions.

- Organize work around a complete end-to-end capability and its known difficult
  failure, rather than repeatedly extending isolated operators until toy cases pass.
- Before a substantial implementation, state the missing capability, the evidence
  for its underlying limitation, and the acceptance conditions. Choose a connected
  design that addresses that limitation across the affected callers and constraints.
- Use the known difficult target and a small set of meaningfully different fields
  or geometries to assess the complete change. Do not defer the difficult case
  until a succession of easier checkpoints has passed.
- Simple tests establish primitive correctness or resolve specific risks; they do
  not establish robust mesh generation and must not be reported as capability
  milestones. Keep private intermediate progress distinct from an admissible final
  mesh and a successful adaptation/solution-transfer workflow.
- The next meaningful curved-construction milestone is a complete accepted mesh
  under unchanged constraints. If the current recovery cannot deliver it, reconsider
  the construction strategy before adding another local fallback.
- When a strategy stalls on the difficult target, preserve the diagnosis and
  reconsider the construction or coordination it lacks. Do not keep adding local
  variants or raising limits without evidence that they address the blockage.
- Deliver substantial connected improvements, then validate at the cadence above.
  Preserve the full agreed goal and all acceptance gates; neither a prototype nor
  a passing fixture justifies narrowing the goal or declaring it complete.

## Reuse and resumable validation

Permanent user instruction, 2026-10-13; applies across context compaction and
future sessions.

- Reuse existing implementation and validation facilities before adding new ones.
  Keep implementation and checks tied to the full agreed goal and its exclusions.
- Reuse passing evidence when its scope covers the requirement and the relevant
  source, build/binary, configuration, inputs and validation helpers remain valid.
  Verify the needed provenance; source hashes alone do not prove unchanged runtime
  behavior. Rerun checks affected by changes, failures or unresolved concerns.
- Make long validation campaigns resumable: save complete independently validated
  cases or stages with their provenance as they finish. A timeout must not discard
  completed evidence or force successful work to repeat. Partial results cannot
  establish whole-campaign success.
- Resume a saved result only after verifying its relevant hashes and acceptance
  conditions. Preserve failures and diagnostics, keep resource/work limits explicit,
  and require every planned case or stage before declaring completion.
- Before restarting a job, verify its existing process/session/job handle. Elapsed
  time, an observation timeout or stale logs alone do not establish termination.

## Adaptation architecture

User instruction, 2026-10-14: Euler-to-BL and BL-to-Euler describe validation
scenarios, never production transition modes. Mesh construction and removal must
follow the target metric, geometry authority and currently requested wall-height
constraints through general remeshing operators. Do not dispatch reconstruction
from Euler/RANS labels, an inferred input-mesh class, case names, saved coordinates
or fixture identities. Solver compatibility and solution-transfer checks remain
separate from mesh reconstruction. Wall-aware operators are allowed to enforce
actual geometric constraints; limited fixture success does not establish general
robustness. Qualify wider fields/geometries rather than adding scenario switches.

## Handbook maintenance

User instruction, 2026-10-14: keep HANDOFF_Codex.md current and concise. Put detailed
current topics and accumulated history into smaller linked handbooks when the entry
point grows. Preserve findings, failures and evidence; historical job/status notes
must not be mistaken for current state. Update the current state in place rather
than prepending repeated copies of prior updates.
