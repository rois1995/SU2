# Current handoff — 2026-10-28

Continue DC-17/DC-18 under [AGENTS.md](AGENTS.md). No accepted complete curved/M6
adaptation, conservative history transfer and solver restart workflow yet. BL/RANS
remains part of the goal; primitive or private progress does not close either item.

User requirements and agent decisions are separated in the
[selected foundation](Handbook/ADAPTATION_ESTABLISHED_DESIGN.md): implement
SCOREC/MeshAdapt classified cavities, current nodal metric transport and
coarsen/refine cycles in SU2. Preserve movable boundaries on fixed geometry,
genuine features/bodies, absolute tolerance independent of h0, validity-only
construction then metric/BL improvement and every final gate.

Latest labelled WIP checkpoint contains build102 code/evidence on rois1995/SU2,
`codex/native-3d-bl`. TODO stays local. Leave unrelated files alone.

Latest source is build102: PASS72.187s with1295 unchanged pins. All13 geometry
backend cases pass62.154s; all10 DC18 geometry groups pass. Focused current-field
122 cases pass10.036s; compiled selection audit verifies26 filters (313 serial,
290 MPI,76 atlas,13 backend,122 nodal selected). Six-rank cycle3 cases pass5.906s. All owned jobs are terminal.
No full broad suite pass: legacy metric-shell assertions remain obsolete/pending.

Connected source: shared existing cubic Gauss–Newton/backtracking/simplex KKT
minimizer and polynomial arithmetic; immutable nets, body/sheet/feature filtering,
existing ADT and current association hints. Conservative [0,upper] distances do
not claim global optimality. A captured shared-edge case exposed a stricter
inward-noise test overriding the existing KKT tolerance; applying that tolerance
first fixes convergence. Candidate search accepts only converged eligible feet,
stops at requested accuracy and fails if none exist. No flat fallback or cap raise.
Certified ClosestCurved and every final gate are unchanged.

Actual mapped102 FAIL58.021s, pins unchanged: curved cases now enter improvement
but exhaust67,207,168 atlas work before transfer. Private edits245 include26 moves,
11 reconnects and41 insertions. Cached geometry debt11, metric/height debt21;
128 private cells still fail quality/size/geometry. Log-wide work is dominated by
ReferencePointKey/BindingParameters and feature validation. Fresh first collapse
builds with63232 work and error.000626<.001, then fails size/progress. Flat flow
and histories transfer and SU2/CGNS reload pass. None establishes curved restart.

M6 build102 FAIL292.221s: solver0/nativefalse, all pins unchanged/ranks terminal.
Native227.7s/import172.78s/refine40.3s;70 private edits, no completed length pass.
Exact blocker: no converged numerical projection on prescribed body/sheet/feature,
first failed facet11053. Whether this is numerical nonconvergence, classification
inadmissibility or candidate handling needs the actual query; do not infer it from
the aggregate error. No time/work exhaustion proves geometric impossibility.
Earlier M6 split-only growth and interrupted runs are preserved in the design record.

Next connected capability must replace unnecessary exact association/feature proof
machinery with fixed-model classification/parameters where justified, and distinguish
candidate projection refusal from backend failure across callers. Preserve genuine
features, bodies, owner/version atomicity, final geometry/quality/height gates,
conservative transfer/history and actual restart. Do not add local fallbacks or
raise caps to mask repeated work. Full BL/RANS remains unqualified.
See [projection findings](Handbook/ADAPTATION_PROJECTION_FINDINGS.md),
[transfer findings](Handbook/TRANSFER_OPERATORS_FINDINGS.md),
[progress](ADAPTATION_PROGRESS.md) and [history](Handbook/ADAPTATION_RUNTIME_HISTORY_20261028.md).

Check live owned handles and host load/RAM; one heavy job, nice19, build-j1,
single-threaded libraries, at most6 available local ranks. Compile temp on4TB.
Reuse only with valid source/binary/runtime/config/input/helper provenance.
Sampled process-tree RSS is not allocation peak. Push labelled checkpoints.
