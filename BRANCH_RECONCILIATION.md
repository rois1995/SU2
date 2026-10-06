# Branch reconciliation — 2026-10-06

Pinned starting checkout: feat_adap_noExt, 88828474db587652ee1b0c09010de838ddbcfd06.
Native baseline: codex/native-su2-2d, 57f0550db4b286dc5f4c34ca98b246bcaeaec0c4.
Full initial refs and checkout invariants are in integration_evidence/.

| Source | Disposition | Reason |
|---|---|---|
| adap-sensors (6140508c23) | Included through Stage G ancestry | Already ancestor of current AdapNoExt; custom sensors retained |
| bl-fix (ba2ec1c2b1) | Included through Stage G ancestry | Two-pass BL, corner fixes, reference restart and fallback behavior retained |
| cap-tests (f794c7104f) | Included through Stage G ancestry | Capability/mesh/metric/temporal regression gates retained |
| mesh-elem-order (0b08e784f7) | Already native base | Dense element and boundary order remains enforced in both SU2 mesh writers |
| r6-fix-codex (c13402bf1d) | Already included | Distributed interpolation and retained locator workspace accounting |
| stage-g (1e39b683ee) | Merged as 40378dac9b | Includes current AdapNoExt plus Euler goal adjoint loop |
| Stage G dirty draft | Snapshot imported as 92a21cc23b | Recording counter, interruption and final sensitivity reporting fixes; original worktree untouched; patch/hash saved |
| origin/fixCGNSOutput (e41e7eabc7) | Merged as d3cb395bc4 | Parallel CGNS/SU2/ASCII output, precision and large-count fixes; adaptation memory boundaries and exact marker descriptors retained |
| b0-repair (c27a187a11) | B0Spike/ retained separately | Protocol/audit/replay tools useful; MMG partition-repair research remains unvalidated, excluded from solver/default tests |
| b0-spike (6d7c9fc3fa) | Included by b0-repair research files | Ancestor of repaired experiment; no production input-dump hook imported |
| master (0e1495c7ff) | Already historical ancestor | No new adaptation contribution |
| origin/feature_CGNS_output and feature_cgns | Superseded | 2018/2020 implementations; current CGNS port and output branch supersede them |
| origin/feature_MeshInterpolation, feature_Interpolate, feature_interp | Historical, not imported | 2015–2019 trees predate current MPI transfer; incompatible whole-tree imports |
| origin/feature_communicator, feature_mpi_comms, fix_addGlobalElementIndex | Historical, not imported | Existing current communicator/index machinery already serves these purposes |
| origin/feature_adap, xla27/feat_adap_mmg | Historical, not imported | Older external-adaptation approaches; current in-memory MMG and collective validation supersede them |
| origin/fix_mesh_deformation_checks (5df818a43f) | Reviewed, separate scope | Four useful SU2_DEF deformation checks; both remesh backends reject moving/deforming meshes, so these are outside the static adaptation envelope and need their own SU2_DEF validation |
| origin/develop/upstream/develop | Not wholesale merged | Broad unrelated upstream changes would expand the validation scope |

Remote refs here are locally cached refs, not a claim that network heads were
refreshed. Other worktrees have dependency symlinks and review docs; Stage G
was the only observed dirty tracked production source. Its snapshot is pinned,
not assumed final if that worker continues editing.

Merge decisions: keep one remesher factory across direct and goal drivers.
It selects native, MMG two-pass, or ordinary MMG. Native plus TWO_PASS errors
at configuration time, avoiding a missing BL metric. Native adjoint remains
rejected before solving. All remesh results pass the existing complete-status
gate before accepted mesh replacement. CGNS reserved/long marker names retain
SU2MarkerName descriptors. Parallel SU2 writers preserve in-memory boundaries,
mixed-volume element ordering and 17-significant-digit coordinate round trips.

Source reconciliation is complete for these pinned inputs. Fresh integrated
primal build,7 output cases and49 focused cases pass MPI1/2/4, as do native
memory and default-MMG controls. Independent output audits and corrected AD
mesh-swap gates also pass. Practical envelope and actual AD lifecycle validation are complete within the
declared scope; classified limits are reported in ROBUSTNESS_SCALING_RESULTS.md.

Revalidated during the campaign: all other local heads/cached remote refs still
match the initial snapshot, and the Stage G dirty diff matches its recorded SHA.
There is no new committed or changed Stage G draft source to reconcile. This
comparison does not refresh remote refs or change another worker's checkout.


## Later local-ref update: rotational periodic solver work

A new cached `origin/fix_periodic_rotation` at
c39428c1daf24b2ab4d7f7ab17b9dc1e8cd9a083 and an updated `origin/pr-images` at
29c71a64b3ae04ed2a5aab3e63744f220e24c68b appeared during the campaign.
This supersedes the earlier unchanged-cached-refs observation; this agent did
not fetch. Other local heads remain pinned, and the Stage G dirty draft retains
its original hash. Full assessment pin: integration_evidence/periodic_branch_assessment_v1.json.

The solver branch adds exactly three commits above common ancestor6db10127d1:
2990df482d (rotate both Jacobian block rows/columns), ce585c7422 (rotate the
periodic solution on all multigrid levels), c39428c1da (velocity limiter stencil
and extrema in the rotated frame). The implementation and regression diffs,
plus the companion evidence README, were inspected. They remain separate scope:
CNativeRemesher::CheckSupport rejects periodic boundaries/paired markers;
CMMGInterface rejects periodic physical boundaries before remeshing; Stage G
ADAP_ADJ_LAMBDA rejects periodic markers. Thus these changes do not repair a
reachable supported adaptation path or change mesh generation/transfer/output.
No periodic numerical fixes or refreshed solver reference values were imported
into this static nonperiodic adaptation branch. The companion solver checks are
prior branch evidence, not fresh integration evidence. Reassess if periodic
adaptation becomes an explicit supported target; do not infer such support from
ordinary periodic CFD runs.


## Current reconciliation and source coverage guards

`integration_evidence/reconciliation_guard_v2.json` rechecks all local heads,
all cached remote refs, every worktree's tracked source drafts, the original
Stage G draft, AdapNoExt and included merge ancestry. All checks PASS. The only
cached-ref changes are the two periodic-work refs assessed above; integration
HEAD itself has advanced. No network fetch or modification of other worktrees
was performed.

`executed_source_coverage_v2.json` verifies all759Common/SU2_CFD production files
unchanged since executed ADv14. The four differences from the original primal
matrix are covered: corrected AD halo seeds, opt-in scaling measurement probe,
and the rebuilt/validated BL-step and NACA-phase test-only instrumentation.
The actual compiled registrations, assertion/build options and runtime evidence
are recorded. Source hashes alone are not substituted for executed proof.
All9new NACA targets and timing/field checks, both accepted partial-mesh full
contracts and final40-case-per-rank MPI-transfer closure PASS. The test-only
patches and closure helper return correction are committed as ffb3fb4c46.
Final requirement/checkout evidence is in INTEGRATION_COMPLETION_AUDIT.md.
