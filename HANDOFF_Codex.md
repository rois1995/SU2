# Codex handoff — current state, 2026-10-26

Active request: keep implementing until the TODO and newly found defects are
verified closed, beginning with DC-17/DC-18. Do not end with a WIP final while
independent work remains. Keep [ADAPTATION_TODO.md](ADAPTATION_TODO.md) current.
The [completion handbook](Handbook/DC17_DC18_COMPLETION_2026_10_25.md) preserves
all diagnoses, failed trials, source/runtime provenance and current acceptance.

Standing target: accepted full M6 Euler then RANS adaptation, general geometry/
metric-driven operators, unchanged q>=.20, J>=.05, edge<=1.8, first height within
10%, geometry, solution-transfer and restart gates. No accepted full M6 exists.
User curvature decision: configured ADAP_HAUSD in mesh units, independent of h0
and ramp. No implicit chord inference or silent tightening to h0.

Current local checkpoint `79bfba6f8f`. Full M6 curved-source/field qualification
passes (validation32–34):450,221 source cells, all48,954 reference facets queried,
293,724 directed chords, exact serial/MPI2 record identity including ownership.
Persisted curved import, marker-name reordering, private geometry scoring and
original-parent support are connected. Small cubic Euler/RANS runtime meshes
complete with conservative BDF2 accounting and SU2/CGNS reload. See
[curved-source history and current design](Handbook/DC18_CURVED_SOURCE.md).

Collision-index reuse is implemented and validated (validation40): serial99 cases/
15,752 assertions; MPI2 99 cases/rank,16,376/16,055 assertions; all-active-worker
runtime408/442 assertions. Tests cover ID/version movement, deletion, rollback,
rebuild and collective memory refusal. Exact contacts and owner vetoes remain.
Full M6 comparison job20721 is terminal:3,122 accepted edits (was99),38 collision
builds,1,040,674 exact contacts in22.11s; native300.89s,135,178 invalid cells and
80,523 queued edges remain. Original CFD state retained; no accepted M6.

Validation41 is terminal/rejected, process exit0; source/runtime pins verify.
Native1801.78s, whole1992.14s, peak2,836,992KiB. Accepted41,632/48,705 edits:
9,622 geometric,21,252 size-progress;511,810 private cells. Remaining geometry
violations2,638 with EMPTY geometry queue;292,368 metric/height violations,
257,684 metric edges. No accepted mesh; original CFD state retained.

Validation42 is terminal;988 source/input and19 runtime pins verify. Build25343
passes (1003.45s, process peak4,090,520KiB); geometry/runtime70748 passes9 cases/
1,323 assertions. Cubic Euler/RANS each reactivate75 geometric requests and
complete transfer/reload. Geometry memo key checks pass. Volume66000:9/10 cases
pass, hidden sphere still exhausts65536 points (retained65446, panel297, ray1).
No M6 run42. Shared IntegrationColumns3D removes duplicate local recursion but
needs better off-centre angular conditioning; preserve failure and limits.

ACTIVE: validation43 full M6 MPI2 session75390 running; pinned C++ unchanged.
Build78606 passes (898.60s, process peak4,090,492KiB);989 source/input pins
verify,19 runtime files pinned. Serial70556 passes105 cases/15,872 assertions;
MPI24618 passes105 cases/rank (16,513/16,205). All-worker55579 passes397/431
assertions; both cubic branches reach2 simultaneous commits and complete strict
mesh, conservative transfer and SU2/CGNS reload.
Directory selects compatible peers only after current-mesh owner vetoes; peer
conflicts defer without blocking the edge. Curved refinement uses the existing
concurrent path; final dependency reservations/atomic parameter publication stay.
Tests cover overlap, independent peers and vetoed-first/valid-second interaction.
All-worker cubic runtime requires multiple parallel commits. Qualify connected
serial/MPI before full M6 MPI2 at1800s and all unchanged mesh/500k gates. CPU use
changes from1 to2 workers, so do not claim a same-resource elapsed speedup.

New caller gap: normal runtime Refine still uses Absolute per-edit shape gates;
ValidityOnly/RefineOptimize is serial and not connected there. Qualify construction
plus repair/optimization under the user’s final-only quality policy; do not simply
relax admission. See current curved-source handbook.

Next: complete production geometry construction and its runtime workflow, then
connected metric/support/quadrature, wall refinement and initialization.
Qualify the newly connected persisted curved import through full runtime workflows;
keep the separate BL refusal until its support/quadrature consumers are complete. FIRST_ROW W
currently lacks a SurfaceError demand. Fitted-plane seam release must survive
curved promotion. The new positive bound certifies local normal clearance, not global embedding.

DC17 field/budget blockers remain:21/9,393 changing-frame gradation violations
(worst ratio1.406). Normalized M6 event1 passes12GiB admission but all nine allowed
profiles exceed500k before construction. Validation20 uniform-sensor diagnosis:
profile0 821,265+/-53,688; profile1 2,298,938+/-198,670, mostly the outer tail.
Validation21 rejects the diagnostic normal cap (best1,182,909+/-67,994).
The existing normal-only relaxation contract remains in force. An asynchronous
question about permitting tangential growth outside the protected band is pending.
Do not treat silence as approval. Hidden-sphere quadrature work exhaustion remains.

Automatic approval review blocked pushing to origin; destination approval remains
pending. Do not retry remote export without approval. Local checkpoints are allowed.
Leave unrelated dc11/dc15 campaign logs and untracked planning files untouched.
Git status needs --ignore-submodules (pre-existing broken Eigen submodule gitdir).

One owned heavy job, nice19, libraries single-threaded, build-j1. Ninja1.13 at
/home/rausa/Software/su2-develop/ninja; compile scratch on4TB at
integration_evidence/build-integrated-v2/compile-tmp. Check actual host load/RAM
and live handles before sustained work. Never edit pinned C++ during a build/run.
[AGENTS.md](AGENTS.md) contains the permanent implementation/validation cadence.
The [October20 archive](Handbook/HANDOFF_2026_10_20_ARCHIVE.md) is historical.
