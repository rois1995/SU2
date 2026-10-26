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

Current local checkpointfdb6ad2683. Full M6 curved-source/field qualification
passes (validation32–34):450,221 source cells, all48,954 reference facets queried,
293,724 directed chords, exact serial/MPI2 record identity including ownership.
Curved first-height clearance/local differential geometry remains validated on
325 M6 queries, including65 certified fluid-side1e-6 offsets (validation25).
See [curved-source history and current design](Handbook/DC18_CURVED_SOURCE.md).

Production persisted curved import now works, including marker-name reordering.
Validation37 fixes private mapped geometry scoring and reuses exact original-parent
support/nodal chords. Both small cubic runtime branches complete with159 cells,
conservative BDF2 accounting and SU2/CGNS reload. Full M6 job25094 finished with
adaptation rejected: all303,359 initial cells measured in41.24s,99 accepted edits,
then301.36s deadline. Original CFD state retained; no accepted full M6 exists.
The next limitation is whole-mesh collision-index rebuild on every surface edit
(174.42s unclassified refine time). Reuse Directory/CADTElemClass with publication
invalidation, exact contacts and remote queries; design is in the linked handbook.

Validation38 corrects the RANS test oracle for closed-wall volume conservation
and no-slip redistribution. Independent saved-mesh flow/history error8.88e-16.
Serial46476 passes98 cases/15,718 assertions. MPI24127 exposes local-only wall
classification of halo nodes (owned states correct); validation39 gathers physical
wall global IDs for the oracle. Build85486 and MPI2 job20715 pass98 cases/rank (16,342/16,021 assertions);
serial runtime61407 passes487 assertions. Validation39 is terminal; its pins verify. The checkpoint retains production validation37;39 only changes tests.
New validation40 implements Directory collision-index reuse, version checks,
atomic added-cell tracking and exact owner vetoes. Direct distributed tests cover
movement/reused IDs, deletion, rejected publication, rebuild and byte refusal.
Build48180 failed on missing LINE enum include/fixed-record codec; corrected
with bounded std::array proposal fields. Build13114 passed; final count/thread-bound review fixed and build47859 passed.
Final build90802 passes. Serial43635 passes99 cases/15,752 assertions; MPI94064
passes99 cases/rank,16,376/16,055 assertions. All-active-worker MPI57077 also
passes408/442 assertions. Full M6 job20721 is terminal:3,122 accepted edits (was99),38 collision builds,
1,040,674 exact contacts in22.11s; total300.89s,135,178 invalid cells and80,523
queued edges remain. No accepted M6; original CFD state retained. Source pins
verify. This is time exhaustion during active refinement, not operator stagnation.
Validation41 adds final demand diagnostics and a bounded longer qualification:
native1800s/outer2400s,12GiB/rank, same500k/q/J/edge/geometry/transfer gates.
The300s comparison remains preserved in40; no mesh acceptance condition changes.
Build passes; serial23227 passes3 cases/7,689 assertions. Full M6 job23818 running; preserve pinned C++. Gates/deadline unchanged. Automatic curved initialization
is absent and BL support/quadrature remains guarded.
- Shared Option A field is used by normalization, profile selection, construction,
  reassessment and original-family correction; finer sensor directions survive.
- Registered-column cavities resolve the remaining M6 first-height witnesses.
  The diagnostic mesh still fails final quality/edge/budget gates.
- Explicit ADAP_NATIVE_MEMORY_LIMIT (MiB/rank), final publication-budget guard,
  query-error propagation and historical distributed-repair fixtures are fixed.
- Curved mapped controls have strict realization/child-error/Jacobian/shared-edge
  checks, v7 persistence and source-envelope displacement. Full M6 admits48,954
  patches with zero fallbacks (validation22); no volume construction.
- Validation23 adds bounded closest-distance queries restricted to connected
  same-marker components and includes cubic controls in cross-rank agreement.
  Serial passes12 cases/1,025 assertions; MPI2 passes12/1,026 on each rank.
  All195 M6 queries from the saved sidecar meet1e-8 distance accuracy; worst
  work2,336/16,384, query time1.59s, whole10.09s, process peak314,412KiB.
  Source/input/binary/runtime pins verify for validation23.

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
