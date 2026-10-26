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

Current curved-source stage: validation32 admits the complete M6 source shell
under unchanged gates:146,862 positive/conforming exterior tets,4,860,163 exact
contacts and650,106 control-hull separation checks;450,221 total source cells.
Whole63.66s, process peak817,540KiB, requested storage1.63GB. Serial29 cases/
3,055 assertions; MPI2 each29 cases,3,741/3,780 assertions. Source/runtime pins
verify. The154.9MB source snapshot has exact serialization round-trip validation.
Validation30's incomplete polytope axes were diagnosed and fixed; previous folded
shell failures remain preserved. See [curved-source details](Handbook/DC18_CURVED_SOURCE.md).
Validation33 field job7949 passes all48,954 facets:489,540 point queries,
293,724 directed chord integrations, original P1 priority retained; worst reversal
relative error2.16e-19. Whole126.95s, process peak526,068KiB. All pins verify.
Next qualify two-rank M6 source construction against the exact saved records.
Curved clearance/local differential geometry checkpoint61bf6cc4c1 remains validated
on325 M6 queries, including65 certified fluid-side1e-6 offsets. No adapted mesh
has passed the complete acceptance gates; automatic curved import/BL stay guarded.
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

Next: qualify the admitted source through actual field queries/chords, then
connected metric/support/quadrature, wall refinement and initialization.
Import and BL construction explicitly refuse incomplete curved production use;
remove these guards only when all affected consumers are qualified. FIRST_ROW W
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
