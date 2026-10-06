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
memory and default-MMG controls. Independent audits, practical envelope and AD
validation remain in progress; consult HANDOFF_Codex.md and runtime JSON evidence.
