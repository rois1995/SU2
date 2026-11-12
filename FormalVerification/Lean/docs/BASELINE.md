# Selected SU2 baseline

The workspace root is a collection of checkouts, not a Git repository. It also contains
concurrent adaptation/Jacobian work. No production checkout was edited. The selected
historical commit is e0d3ee8d72598767227e62a9be5834a4554532b6; its parent is
371fcae6f2d408f881f9db130f9feedd6a791e9d. Exact files, Git blob identifiers and SHA-256
hashes are in `source/manifest.json`. The existing accepted-for-this-scope repair is
`source/accepted-moving-wall.patch`; no additional production patches are applied.
This is not a claim that all code in this historical commit is defect free, nor a claim
about the latest upstream release. Source files retain SU2 licensing (`source/COPYING`).

## Defect triage

| Item | Status and evidence | Effect on selected scope |
|---|---|---|
| Classical compressible moving-wall projection uses current density for old state, omits density coupling in residual/correction, and omits associated work | Already repaired by e0d3ee8d72; patch and 182-line production unit test inspected. Historical `pr2943_wall_projection_control_20261012/wall-unit-tests.log`: 1 test, 12000 assertions passed. `final-unit-tests.log`: 5 cases, 13041 assertions passed. Existing pre-fix log fails a Jacobian assertion. These are historical records, not reruns here. | Reuse repair. Lean counterexample isolates the residual density-coupling defect in its parent. Full historical suite does not substitute for Lean proofs. |
| Sharp-edge merged wall normal | Recorded confirmed in `todo/bug-wall-normal-sharp-edges.md`; `SU2_SharpEdgeNormals` inspected at 803cb03001e59a31df94759c31643e285d1ea544; existing multi-normal repair on that separate branch | Not merged into this historical baseline. Single prescribed unit-normal operation only; sharp-corner paths cannot be claimed verified. |
| Under-relaxation may accept negative pressure | `todo/bug-underrelaxation-positivity.md`; existing optional POSITIVE_UPDATE_FLOW repair 49bc1fef31 | Not duplicated. Unconstrained update positivity is not asserted. Wall correction positivity is proved separately. |
| Newton missing wall projection | HANDOFF.md records repair fd9d5faa04; moving grids not checked in that record | Newton dispatch/integration excluded. |
| Moving-wall viscous work Jacobian G13 | Existing fix d6b6f5386b, historical audit notes | Viscous terms and Jacobians excluded from this proof path. |
| Motion/restart defects G1/G2/G9/G16 | Existing repairs recorded in `todo/review-mesh-deformation-motion.md` and `Prove/audit_tests/archive/notes/consolB/` | Grid-velocity production, DG and restart are not assumed verified; w is an explicit input. |
| SST, adjoint, MPI/periodic and adaptation findings | FINDINGS.md, ADJOINT_FINDINGS.md, TODO.md retain the workspace records | Separate unresolved solver paths. No global validity inferred. |

No new defect claim against current upstream is made. The phase deliberately pins a known
repair rather than re-fetching or changing branches used by other ongoing work. The
selected baseline source is obtained with `git show COMMIT:path`, so unrelated dirty
working-tree files do not enter it. The new formal artifacts and ledger live in SU2_Lean;
shared project ledgers were not rewritten during concurrent work.

Historical logs are copied under `evidence/history/`, with their original paths and hashes.
Their exact binary-to-commit binding was not independently established; the fresh extracted
source experiment instead uses the manifest-pinned files directly.
