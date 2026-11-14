# P5 certified classification memo reuse — source finding, unimplemented

The current ExactClassification fallback reads a legacy Classification memo only when its upper bound is <= tolerance (CNativeAffineReference.cpp:2086–2091). This safely excludes legacy upper-only refusals, but also excludes legacy responses with an actual outside witness. QuerySurfaceUnionAtResolution already returns `outside_tolerance` from the directed traversal receipt (:2399–2406). That proof is currently discarded before FaceMemo::Store at :2098–2101.

Smallest connected correction to consider after the full workflow is terminal: retain that proof bit in the existing numeric union memo entry; ExactClassification may reuse a legacy entry when either its upper bound certifies inside or its stored receipt certifies outside. A legacy above-tolerance entry without the proof bit must still refine. No magnitude query may use a classification-only outside entry as its complete magnitude. Keep the existing cutoff and magnitude keys distinct, quota unchanged, and final geometry checks unchanged.

This targets repeated exact classification of questions already answered by another caller. It does not change the classification algorithm or claim that unknown legacy refusals prove outside. Actual remaining-work savings are unmeasured; do not claim this will meet the 108459426 gate before checking it.

Focused check: fresh active cubic authority; issue a legacy query with a certified outside witness, then ExactClassification; require same decision and no additional upper work. Retain the existing conservative-unknown-refines and memo-order tests. Also compare complete Magnitude before/after classification cache population.

Runtime source is still frozen for full_workflow; this file is scratch only.
