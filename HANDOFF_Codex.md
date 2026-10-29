# Current handoff — 2026-10-29

Continue DC-17/DC-18 under [AGENTS.md](AGENTS.md). No accepted complete curved/M6
adaptation, conservative history transfer and solver restart workflow yet. BL/RANS
remains part of the goal; primitive or private progress does not close either item.

User requirements and agent decisions are separated in the
[selected foundation](Handbook/ADAPTATION_ESTABLISHED_DESIGN.md): implement
SCOREC/MeshAdapt classified cavities, current nodal metric transport and
coarsen/refine cycles in SU2. Preserve movable boundaries on fixed geometry,
genuine features/bodies, absolute tolerance independent of h0, validity-only
construction then metric/BL improvement and every final gate.

Latest pushed WIP checkpointa8135c6e63 contains captured fixes and per-pair staging on rois1995/SU2,
`codex/native-3d-bl`. TODO stays local. Leave unrelated files alone.

**Build114 PASS261.364s; diagnostic115 RUNNING session53266;1295 source/test/helper pins fixed. Do not edit.**
Build109 fresh MoveVertex replay reproduces genuine corner0 refusal, work0/no
exhaustion. The earlier empty first-repair capture was a probe before any operator;
now excluded.112 labels the actual operator and logs cached debt/descriptor counts
with consistency flags at phase boundaries; no new q/J/geometry measurements.
Build110 logging gate was inactive; fixed before112. No accepted curved mesh.

Supporting build108 checks: opposing-length serial and MPI6 focused8 cases/rank,
current-field122 cases,28 compiled selections and all10 DC18 groups pass. Actual
mapped108/109 reaches8 improvement stages after one pair (75 edits/44 moves/19
reconnects), but exhausts unchanged67,207,168 atlas work before curved transfer.
Detailed evidence/previous terminal M6 outcome follow below; actual111/112 M6
staging qualification and repeated accepted transfer/restart still required.
ma/LICENSE is non-commercial; root LICENSE BSD. No MeshAdapt source copied.
Build106 projection paths PASS16 cases/547 assertions64.187s; filter audit28
selections passes. Small curved workflow FAIL89.521s at same67,207,168 atlas work
after331 edits/83 moves/39 reconnects. First fresh collapse builds within tolerance
but adds edge1.973>1.8, despite reducing changed-edge excess .189605 to .029831.

M6 build106 FAIL1870.389s, solver0/nativefalse, pins unchanged/all ranks terminal:
unchanged1800s native limit;11567 private edits,307011 private cells, two full
collapse/split pairs, no quality stage or transfer. Sampled tree RSSmax3.43GiB
(not whole-job high-water mark).154 refused queries/46 distinct scopes repeat
up to12 times; every record has a raw model foot within requested spatial accuracy.
Detailed current receipts: staged57/build106_mapped_summary.json and
staged57/build106_m6_summary.json. Captures remain in the corresponding case dirs.

Build107 closes those captured shared paths: accept a classified original-model
foot when its existing conservative distance bound meets requested accuracy even
if numerical stationarity stalls; during validity-only construction admit a
cell-removing collapse with strictly lower changed-edge debt. Final gates unchanged.
Captured regression fixtures pass; actual smaller workflow is running. No new projector,
representation, schedule, fallback or work limit. Full accepted workflow remains open.

The original exact M6 failure at build103 is captured/replayed: hint11540,
body0/sheet10154; all7 permitted world-coordinate solves fail,6 normalized solves
converge, hint distance4.78e-11. Fixed geometry and body/feature identity remain
original authority; ordinary canonical associations skip redundant exact feature
keys. Typed unavailable vs backend failure is carried through existing stage votes.

See [projection findings](Handbook/ADAPTATION_PROJECTION_FINDINGS.md),
[transfer findings](Handbook/TRANSFER_OPERATORS_FINDINGS.md),
[progress](ADAPTATION_PROGRESS.md) and [history](Handbook/ADAPTATION_RUNTIME_HISTORY_20261028.md).

Check live owned handles and host load/RAM; one heavy job, nice19, build-j1,
single-threaded libraries, at most6 available local ranks. Compile temp on4TB.
Reuse only with valid source/binary/runtime/config/input/helper provenance.
Sampled process-tree RSS is not allocation peak. Push labelled checkpoints.
