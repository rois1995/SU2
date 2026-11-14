# M6 query-purpose regression fix — 2026-11-14

Status: implementation frozen; isolated build495 validation is pending (attempt3). No accepted mesh is claimed.

Input: build494 source on `codex/native-3d-bl`, exact clean-check Euler configuration/seed and C492 saved485 BL state.
Output: explicit Classify / Limit(cutoff) / Magnitude face-query needs; unchanged complete magnitude scoring and final gates.
Acceptance: all five checks below, with complete source/binary/input/runtime provenance and owned terminal handles.
Limits: 5400s native/event, 12GiB, geometry1e-4 and existing q/J/size/height/progression gates unchanged.

The expensive `EntityFaceError` magnitude refinement now runs only when its caller needs it.
A limit query can stop when a certified lower bound proves rejection. The caller recomputes its squared-excess sum;
if rounding leaves rejection undecided, it retries Magnitude. Lower-only results cannot enter the magnitude memo.
Complete limit results may reuse/populate magnitude entries; caller-owned guidance entries retain only complete face scores.
Census debt, old-surface scores, split prediction and accepted-trial scoring retain Magnitude.

Proposal correction: classification previously could conservatively refuse an ambiguous curved approximation without an outside witness.
`CNativeAffineReference.cpp` now separates strict ExactClassification from the preserved legacy conservative query.
Only a certified outside witness or a complete inside bound decides the new Classify need; no acceptance gate is relaxed.
The reviewer claim that the first old classification result always decided the true tolerance question was too strong.

| Check | Status / numbers |
|---|---|
| 1. Affine/backend unit tests, inside/near/far query needs and memo order | Attempt2:74/75 passed,8516/8517 assertions; compatibility fix pending |
| 2. Six-rank first collapse, work <=108459426; counts 480/472/344/263/473/464 | Pending; reusable gate prepared |
| 3. Exact C492 serial BL repeat, >=300 edits in388.8s; compare measurement v2 | Pending |
| 4. Existing geometry-magnitude481 and saved-surface-corrections481 checks | Pending; runner/input-preserving adapters prepared |
| 5. Full no-BL workflow, both adaptations accepted <=5400s/event | Pending |

Build494 and its original binaries are preserved. The isolated build is `integration_evidence/build-integrated-v3-p5-20261114`.
Attempt1 stopped after10.105s on a header declaration using INFINITY before its definition; source pins verified unchanged.
The header now uses `std::numeric_limits<double>::infinity()`; attempt1 receipts are retained, attempt2 passed in590.123s; its binaries are preserved.

Attempt2 units caught a changed legacy conservative refusal; attempt3 preserves it and isolates strict classification in its own memo key.

Evidence: `integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/purpose_query_fix_20261114/`.
`source_pins.json` freezes968 C++/header/fixture files; `source.patch` records the connected source change.
The first-collapse gate reports count differences without tuning. Historical counts do not prove equal collapse IDs.
Build293 timings are historical; build494's full unprofiled workflow rejected both adaptations.
See [cause diagnostic](ADAPTATION_M6_NOBL_CAUSE_CONFIRM_20261114.md), [clean check](ADAPTATION_M6_NOBL_CLEAN_CHECK_20261114.md),
[measurement v2](ADAPTATION_M6_MEASUREMENT_V2_20261114.md). File hashes alone establish preservation, never path validation.

Next: finish the five checks, make F1/F2 permanent, then the connected cheap items and one repeated A/B profile;
then the exclusive timer tree and one independently reviewed C/D/E design. No C/D/E implementation before user acceptance.
