# Codex handoff — current state, 2026-10-26

Active request: keep implementing until the TODO and newly found defects are
verified closed, beginning with DC-17/DC-18. Do not end with a WIP final while
independent work remains. Keep [ADAPTATION_TODO.md](ADAPTATION_TODO.md) current.
The [progress companion](ADAPTATION_PROGRESS.md) gives the concise checkpoint/result
history and must stay current alongside the TODO. The [completion handbook](Handbook/DC17_DC18_COMPLETION_2026_10_25.md) preserves
all diagnoses, failed trials, source/runtime provenance and current acceptance.

Standing target: accepted full M6 Euler then RANS adaptation, general geometry/
metric-driven operators, unchanged q>=.20, J>=.05, edge<=1.8, first height within
10%, geometry, solution-transfer and restart gates. No accepted full M6 exists.
User curvature decision: configured ADAP_HAUSD in mesh units, independent of h0
and ramp. No implicit chord inference or silent tightening to h0.

Current implementation checkpoint `3deed0ca3c` (explicit WIP: validation46
corrections building;43–45 failures preserved; progress companion added). Previous validated scheduling/concurrency
checkpoint `e398807a2c`; full M6 remains open.
Full M6 curved-source/field qualification
passes (validation32–34):450,221 source cells, all48,954 reference facets queried,
293,724 directed chords, exact serial/MPI2 record identity including ownership.
Persisted curved import, marker-name reordering, private geometry scoring and
original-parent support are connected. Small cubic Euler/RANS runtime meshes
complete with conservative BDF2 accounting and SU2/CGNS reload. See
[curved-source history and current design](Handbook/DC18_CURVED_SOURCE.md).

Collision reuse and original source/field qualification are validated; detailed
records and previous failures are in the curved-source handbook. Baseline41 is
terminal/rejected:41,632 edits,511,810 private cells at1801.78s. It retained2,638
geometry violations with EMPTY geometry queue and292,368 metric/height violations.
95,049 metric trials failed shape vs1,944 size-progress failures. Original CFD state
retained. Source/runtime pins verify; this motivated scheduling and admission work.

Validation42 is terminal;988 source/input and19 runtime pins verify. Build25343
passes (1003.45s, process peak4,090,520KiB); geometry/runtime70748 passes9 cases/
1,323 assertions. Cubic Euler/RANS each reactivate75 geometric requests and
complete transfer/reload. Geometry memo key checks pass. Volume66000:9/10 cases
pass, hidden sphere still exhausts65536 points (retained65446, panel297, ray1).
No M6 run42. Shared IntegrationColumns3D removes duplicate local recursion but
needs better off-centre angular conditioning; preserve failure and limits.

Validation43 is terminal;989 source/input and19 runtime pins verify. Build78606,
serial105 cases/15,872 assertions, MPI2 105/rank (16,513/16,205) and all-worker
397/431 assertions pass. Cubic Euler/RANS each complete with2 simultaneous commits,
strict final mesh, conservative transfer and SU2/CGNS reload.
Full M6 MPI2 session75390 is terminal/rejected:26,567 accepted private edits,
435,293 private cells,218,099 cached invalid cells,184,650 queued edges at the
1800s native limit (1908.44s whole run; largest reported process1,548,176KiB).
21,866 geometric requests reactivated. Current-mesh/peer collision checks and final
reservations ran;238,305 metric shape refusals vs6,177 progress refusals remain.
Rank0 retains1,203 geometry violations with no currently admissible geometry edge.
This is failed acceptance, not a speedup or a completed capability.

Validation44 terminal: build6283 passes (481.39s, largest process4,092,072KiB),
989 source/input and19 runtime pins verify. Repair10/11 cases: new mapped star
omits mandatory positive surface tolerance; plain optimized star passes. Volume8/10:
hidden sphere still hits65536 points (65443/panel297), and longest-edge splitting
on ordinary collapsed charts regresses planar complexity648.985 vs656.152.
Connected serial104/105 cases (15,578/15,580 assertions): cubic Euler/RANS reaches
2 invalid cells then work cap4096;24 expensive placement trials run before cheap
moves. No MPI/M6 run44. All failures and terminal receipt pins retained.

Validation45 terminal: build66808 passes (790.59s, largest process4,092,364KiB).
Focused6243 fails12 assertions/3 of22 cases (1,066,854/1,066,866 pass),42.77s.
Mapped-star omission and planar complexity regression fixed. Hidden sphere now
finishes density/volume/first moments within cap but has8 zero weights from endpoint
rounding. Ordinary two-point radial integration breaks existing collapsed second
moment (.00504901 vs.006). Cubic Euler/RANS still exhaust work4096 with7 invalid
cells; reordering point-placement search is insufficient.989 source and19 runtime
pins verify; no MPI/M6. Failed evidence preserved under45.

ACTIVE: validation46 build56379; pinned C++ unchanged until terminal.
Connected redesign: reuse existing topology optimizer (interior edge/face swaps and
collapses; fixed boundary, no smoothing/insertions/new nodes) on bounded repair
cavities after cheap moves/reconnections. Remap local cell IDs into reserved range,
keep physical skin, exact ValidateFixedInterface, height and full frozen metric
revalidation. Candidate operations remain capped160; cache1024; existing scratch
admission. Both resident/streamed callers; new optimization_trials/validated stats.
ValidityOnly Repair now uses the optimizer's minimum normalized q/J progress,
allowing spare margin in one measure to repair the other. Absolute-mode behavior,
nonincreasing changed-edge excess and final q>=.20/J>=.05/edge gates unchanged.
Focused policy test checks improvement/reversal/worst-score regression.
Quadrature now derives2 nodes/weights for the actual quadratic collapse measure,
keeping polynomial moments through cubic degree; stable remaining-distance
arithmetic avoids zero weights at rounded endpoints. Existing5/7 comparison,
all tolerances/caps retained. Hidden sphere adds second-moment assertions. Independent
weighted_radial_probe.py checks positivity and moments for7 ratios; not acceptance.
See46/summary.json and implementation.patch. Must resolve45 failures before the
six-rank milestone and full M6. No accepted full M6 exists.

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

User reminder2026-10-26: up to6 MPI ranks are authorized. Use a six-rank
qualification at the next connected milestone after serial failures are resolved;
check actual RAM before the full M6 run. One owned heavy job, nice19, libraries
single-threaded, build-j1. Ninja1.13 at
/home/rausa/Software/su2-develop/ninja; compile scratch on4TB at
integration_evidence/build-integrated-v2/compile-tmp. Check actual host load/RAM
and live handles before sustained work. Never edit pinned C++ during a build/run.
[AGENTS.md](AGENTS.md) contains the permanent implementation/validation cadence.
The [October20 archive](Handbook/HANDOFF_2026_10_20_ARCHIVE.md) is historical.
