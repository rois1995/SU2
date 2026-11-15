# Next cheap group — source gaps, 2026-11-15

P6c checks1–6 and the actual isolated event2 CFD restart pass. This is the source check before the next connected B1/B2/B3/B4/B6/A2 change. No replacement design or numerical change was made here.

The reviewer handoff section0 says: “If the source or a measurement shows that a proposal is wrong, stop that item, report the line or the number, and wait for the corrected proposal.” Its [received snapshot](../integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/purpose_query_fix_20261114/p6c_20261115/reviewer_handoff_received.md) defines the role and results-identical requirement. B4/B6 need a corrected contract before their broad results-identical claim can be implemented as written.

## B4 — old cavity values from census rows

- `Common/src/adaptation/CNativeEngine3D.cpp:300` stores raw q/J, aggregate geometry debt and six edge-weighted geometry scores. `CachedRow` at310 serializes that state and edge lengths. It does not store each physical face’s magnitude/binding/coordinate key in the order used by old-cavity guidance.
- At825 the face magnitude is measured; at828 its excess is folded into aggregate debt; at851 it is weighted by physical edge length squared. At863 only edge scores are retained. The individual magnitude is discarded. Aggregate/weighted scores cannot simply stand in for the face-ordered guidance comparison cited by B4; equality of the arithmetic/face scope has not been established.
- `OptimizedRepairPatch` at1120 renumbers old global cells to local0..N−1 before its optimizer. A census lookup by that local id would not name the original owned row.
- Raw q/J reuse may be possible at suitable callers, but the proposal also cites old geometry guidance and optimizer callers. Please correct the scope/identity/arithmetic contract. No new per-face cache or optimizer API was independently added.

## B6 — checkpoint the census using migration serialization

- `CachedRow` at310 contains vertex ids and row versions, not old coordinates or a persistent complete-target identity. `Restore` at975 checks ids/versions, then at985 constructs its edge certificate with coordinates from the newly supplied OwnedCell. Its subsequent reuse test therefore does not independently compare against the old coordinates. Existing migration assumes a live event identity; a disk checkpoint must establish the relevant provenance before reuse.
- More importantly, `seed_next` at2759 progressively measures composed/sided planning, choosing `initial[seed_slot++]` at2771. Rows already present are skipped. Queue visibility depends on measured rows and the construction frontier (280,865,935).
- Thus preloading a complete census before the cold progressive schedule exposes candidates earlier. Rows alone do not establish equal selected edits or an exact scheduler continuation. This is a source-level limitation; a changed mesh has not been measured because no B6 experiment was launched.
- The current bounded BL result is still partial:31747measured surface-demand cells of39914 required, census_complete=false. Please specify the comparison/resume contract and required saved state; do not infer complete-census or replay equivalence from migration row serialization alone.

## A2 — current finite-quota path already differs from the old table

- `Common/src/adaptation/CNativeAffineReference.cpp:1398` admits the numeric memo from the supplied query-cache bytes;1399 uses a one-eighth share, bounded by remaining ceiling;1401 installs the resulting limit. DefaultCapacity at1189 is the unconfigured fallback, not the measured finite-quota production limit.
- Actual P6c BL exit: memo_limit401030920B, memo_bytes15146592B, memo_evictions0. This production path is already stage-admitted. No new quota change is justified by the older fixed2^14-entry description alone.

B1/B2/B3 uncompiled drafts remain preserved under `R/purpose_query_fix_20261114/p6_20261115/cheap_group_set_aside`; the earlier B4/B6 drafts are preserved under `R/purpose_query_fix_20261114/handoff_alignment_20261115`. They are outside active production source. The connected-group A/B reprofile and F3 have not run. The reviewer’s one-engine design remains unchanged; its steps8.2–8.7 still require user acceptance.
