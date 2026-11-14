# M6 query-purpose regression fix — 2026-11-14

Status: attempt6 units/BL/regressions pass; work and full-workflow gates fail. Attempt7 units/BL/481/2D pass; work and full-workflow gates fail. No accepted mesh is claimed.

Input: build494 source on `codex/native-3d-bl`, exact clean-check Euler configuration/seed and C492 saved485 BL state.
Output: explicit Classify / Limit(cutoff) / Magnitude face-query needs; unchanged complete magnitude scoring and final gates.
Acceptance: all five checks below, with complete source/binary/input/runtime provenance and owned terminal handles.
Limits: 5400s native/event, 12GiB, geometry1e-4 and existing q/J/size/height/progression gates unchanged.

The expensive `EntityFaceError` magnitude refinement now runs only when its caller needs it.
A limit query can stop when a certified lower bound proves rejection. The caller recomputes its squared-excess sum;
if rounding leaves rejection undecided, it retries Magnitude. Lower-only results cannot enter the magnitude memo.
Complete limit results may reuse/populate magnitude entries; caller-owned guidance entries retain only complete face scores.
Census debt, old-surface scores, split prediction and accepted-trial scoring retain Magnitude.
Limit may return a complete magnitude upper score or an early certified lower score; rejection is relative to the original magnitude-score sum, not a true-distance cutoff.
Classify uses the entity geometry tolerance. Independent read-only review found no remaining correctness findings; it does not establish validation.

Proposal correction: classification previously could conservatively refuse an ambiguous curved approximation without an outside witness.
`CNativeAffineReference.cpp` now separates strict ExactClassification from the preserved legacy conservative query.
Only a certified outside witness or a complete inside bound decides the new Classify need; no acceptance gate is relaxed.
The reviewer claim that the first old classification result always decided the true tolerance question was too strong.

| Check | Status / numbers |
|---|---|
| 1. Affine/backend unit tests, inside/near/far query needs and memo order | Attempt7 PASS:76 cases/8537 assertions,157.628s; all1539 pins unchanged |
| 2. Six-rank first collapse, work <=108459426; counts 480/472/344/263/473/464 | Attempt7 FAIL:116540211 work (limit108459426); all six counts match494; inner426.100s |
| 3. Exact C492 serial BL repeat, >=300 edits in388.8s; compare measurement v2 | Attempt7 PASS:300 splits/304578 positive tets; owned/parameters/donors byte-identical to v2 A; API389.774s, cfg388.8s |
| 4. Existing geometry-magnitude481 and saved-surface-corrections481 checks | Attempt7 PASS:merit55cases/5763assertions; saved1case/611527assertions with qualified v3 target (v2 rejected) |
| 5. Full no-BL workflow, both adaptations accepted <=5400s/event | FAIL:first event5401.986s, all6 incomplete; stopped after rejection; event2 not evaluated |

Build494 and its original binaries are preserved. The isolated build is `integration_evidence/build-integrated-v3-p5-20261114`.
Attempt1 stopped after10.105s on a header declaration using INFINITY before its definition; source pins verified unchanged.
The header now uses `std::numeric_limits<double>::infinity()`; attempt1 receipts are retained, attempt2 passed in590.123s; its binaries are preserved.

Attempt2 units caught a changed legacy conservative refusal; attempt3 preserves it and isolates strict classification in its own memo key.
Attempt3 first-pass work fell88.68% versus494 but is8.03% over the unchanged limit; all owned handles terminal, pins unchanged.
Its rank0 memo had10245 evictions/4MiB. Attempt4 reuses complete magnitudes for Classify and pulls A2 forward:
the answer memo takes at most1/8 of the existing admitted query quota; prepared storage loses the same reserve. No total quota increases; the legacy4MiB floor remains when it fits.
Attempt5 removed all evictions but saved only0.45% upper work: quota pressure is not the remaining main cause.
Attempt6 completes cross-purpose reuse of legacy certified-inside bounds; conservative above-tolerance responses still refine.
Its work116540211 still misses by7.45%; no further quota tuning. BL surface remains unready/unaccepted despite identical300-split output.
The original481 saved-target check fails the pre-existing checkpoint-v3 guard (CNativeTargetCheckpoint3D.hpp:178).
Its assertions pass with the fidelity-qualified492 target from saved CFD (no solve), same saved geometry/contexts; this is a port, not exact old-target preservation.

Evidence: `integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/purpose_query_fix_20261114/`.
`source_pins.json` freezes968 C++/header/fixture files plus571 unchanged build494 dependencies; `source.patch` records the connected source change.
The first-collapse gate reports count differences without tuning. Historical counts do not prove equal collapse IDs.
Build293 timings are historical; build494's full unprofiled workflow rejected both adaptations.
Attempt6 full first event:24359 q/24622 J/356 geometry bad cells; source/runtime pins unchanged, all owned handles terminal.
Only9 quality batch rounds completed after quality entry; rank3 upper-bound timer3561.998s (nested guidance728.256s); no off-CPU attribution.
Host load2.97–15.03; max modeled per-rank native storage10.186GiB, max individual reported RSS5.972GiB; sampled concurrent solver RSS12.759GiB.
Attempt7 retains certified-outside receipt bits; unknown refusals still refine; complete magnitude stays separate. First-pass work saving is zero.
F1/F2 execution gates: R/shared_engine_regression_gates.json; 2D native BL/no-BL1/2/4 pass in6.479/6.451/6.516s (39cases/rank).
Owned process group deliberately terminated after first rejection; exit-15 and manual_stop_after_first_rejection.json record it.
See [cause diagnostic](ADAPTATION_M6_NOBL_CAUSE_CONFIRM_20261114.md), [clean check](ADAPTATION_M6_NOBL_CLEAN_CHECK_20261114.md),
[measurement v2](ADAPTATION_M6_MEASUREMENT_V2_20261114.md). File hashes alone establish preservation, never path validation.

Next: finish the five checks, make F1/F2 permanent, then the connected cheap items and one repeated A/B profile;
then the exclusive timer tree and one independently reviewed C/D/E design. No C/D/E implementation before user acceptance.
