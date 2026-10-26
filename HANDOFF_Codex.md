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

Current implementation checkpoint `4933586627` (focused repair/runtime passes;
connected serial/MPI and full M6 pending). Prior `3deed0ca3c` preserves46 implementation. Previous validated scheduling/concurrency
checkpoint `e398807a2c`; full M6 remains open.
Full M6 curved-source/field qualification
passes (validation32–34):450,221 source cells, all48,954 reference facets queried,
293,724 directed chords, exact serial/MPI2 record identity including ownership.
Persisted curved import, marker-name reordering, private geometry scoring and
original-parent support are connected. Baseline43 small cubic Euler/RANS runtime meshes
complete with conservative BDF2 accounting and SU2/CGNS reload. See
[curved-source history and current design](Handbook/DC18_CURVED_SOURCE.md).

Validated baseline43:105 serial cases,105 cases/rank on MPI2 and all-worker cubic
runtime transfer/reload pass. Full M6 MPI2 is still rejected:26,567 private edits,
218,099 cached invalid cells at1800s;238,305 shape refusals motivated runtime
admission work. The [progress companion](ADAPTATION_PROGRESS.md) and curved-source
handbook preserve41–44 failures, counters, timings and complete evidence paths.

Validation45 terminal: build66808 passes (790.59s, largest process4,092,364KiB).
Focused6243 fails12 assertions/3 of22 cases (1,066,854/1,066,866 pass),42.77s.
Mapped-star omission and planar complexity regression fixed. Hidden sphere now
finishes density/volume/first moments within cap but has8 zero weights from endpoint
rounding. Ordinary two-point radial integration breaks existing collapsed second
moment (.00504901 vs.006). Cubic Euler/RANS still exhaust work4096 with7 invalid
cells; reordering point-placement search is insufficient.989 source and19 runtime
pins verify; no MPI/M6. Failed evidence preserved under45.

ACTIVE: validation47 serial58788 running. Build19887 and focused13770 terminal/pass.
Focused12 cases/2,183 assertions,77.72s,largest process48,096KiB. Both cubic runtime
Euler/RANS complete with zero invalid cells and work1,180/4,096. Connected serial
then six-rank MPI/all-worker qualification and full M6 remain. Validation46 build56379 and focused7723 are terminal.
Focused21/22 cases,1,066,877/1,066,879 assertions pass. All volume/integration cases
pass, including hidden sphere positivity/second moments. Cubic Euler/RANS still
fails work4096 with14 invalid cells (25 optimizer calls,5 validated).990 source,
19 runtime and29 receipt pins verified before edits. No MPI/M6 for46.
47 changes only Engine3D: refund unused optimizer reservation from report.trials,
both resident and streamed callers; runtime guard rejects reservation overrun.
Keep pinned sources unchanged once build/run starts. Work cap4096 unchanged.
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
Do not treat silence as approval. Hidden-sphere integration passes46 under unchanged caps; whole-field normalization remains open.

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
