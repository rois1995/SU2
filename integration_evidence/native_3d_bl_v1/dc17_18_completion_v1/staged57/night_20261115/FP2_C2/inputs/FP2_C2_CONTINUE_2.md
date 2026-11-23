# FP2-C part 2, continuation 2: correct the "removals only" fixture (test code only), then go on with continuation 1

Author: reviewer, 2026-11-23 (design review by Codex). For the main agent. The user sends it; it is a user order.
Copy this file with its SHA256 into `R/night_20261115/FP2_C2/inputs/`.
Code state: your checkpoint `1e7335a71e` ("FP2-C2 continuation 1: fixtures FAILED"), build654. The production
sources of build654 do NOT change in this order. It continues `FP2_C2_CONTINUE_1.md` (SHA256 `7480af81...6ee2`,
"continuation 1"); where the two differ, this text wins.

## 1. Why

Your diagnosis is correct: no wiring error. The fault is in continuation 1: its section 4 covered the fixtures
with a fake geometry and omitted the "removals only" section of fixture (i), which removes (1, 1, 0) onto
(2, 1, 0) with the REAL overlay on S33. The S33 reference is not curved: it has no neighbour search,
and `NearbySurfaceFacets` (`AR:1952-1955`) gives the strict query only the inherited facet. A child that leaves its
inherited facet by more than the tolerance cannot be classified there, also when it is exactly on the plane. The
census of the lab on the rebuilt state would classify this S33 child above the tolerance too (the same function
and inputs), so test 11 does what its text says. The production final proof can still certify the whole entity in
both directions (its fallback is a proof of the whole entity). The three lab cases of ONERA M6 have a control net
and a search root for each of their 48,954 reference facets, also on the two planar sheets, so this limit cannot
occur there. It is a property of the model code for a reference without curves, not of step C, and it stays.

## 2. What to do (test code only, R3)

1. The "removals only" section of fixture (i). On rank 0, try the pinned edge one time with the default options
   and the plain real overlay: the returned result must be `Bound`, and `Snapshot`, `FrozenSnapshot` and
   `OverlaySnapshot` must be equal before and after (this pins the limit of the reference without curves). Then
   run the existing sequence of the section (acceptance, and the retry with `shape_test = false` only when the
   result is `Shape`) on a fresh mesh, with a test-only wrapper that forwards every function to the same real
   overlay and returns true from `WithinTolerance` (the forwarding pattern of `RecordingCollapseBound`; that
   recorder itself keeps its real answers in fixture (o)). The section keeps its purpose and its old expected
   results: the removal travels, the collective state build accepts with removals only, the bytes of `Snapshot()`
   are equal. The override tests the exchange of removals and the state build, not the bound.
2. No other fixture needs this correction: (a), (h), (c), (n), (o) and the other sections of (i) restore children
   inside their inherited facets; (e) uses a fake; (j) passed on build654. If one more fixture fails for this same
   reason, apply the same correction and write it in the report. Do not change a fixture for another reason.
3. The production sources and the solver hash must be those of build654 (verify the pins of `Common` and
   `SU2_CFD`). Build the test driver. Then continue continuation 1 at its item 2 from "All fixtures of the order and
   of this text": all fixtures, the option-off cohort, the 18 FP1 cohorts, G1; commit and push ("FP2-C2
   continuation 1: fixtures passed; default path not yet proven"); then its items 3, 4 and 5, not changed. The
   label "fixtures FAILED" of `1e7335a71e` is superseded by this order.

## 3. More report rows for the lab (test code)

In the lab report of continuation 1, item 3: the final count of surface faces for each entity (body, sheet,
marker), one time for each trial, from `state.Surface()` and `state.Entity(face.facet)`. The run of build653 kept
no such counts: write "not available" for it; do not run step 1 again. And in the C2 lab only: forward the real
bound query during `RunPass` and count each false answer by entity and by sweep stamp (through the existing
`SurfaceFixtureAccess` friend); report these counts beside the accepted collapses for each rank, pass and sweep,
with the pass total `c_refused_bound`. Reason: if test 11 refuses most collapses on one entity, the report must
show it. These rows are a report, not a gate; they are not in the repeat comparison.

## 4. Not in this order

A change of production code. A change of `NearbySurfaceFacets` or of the strict query for a reference without
curves. A change of a gate of continuation 1.
