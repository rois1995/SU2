# Findings

* Actual upstream baseline fetched and pinned; no selected-path production repair is needed
  on the current evidence. This is not a whole-codebase defect-clearance claim.
* Actual ideal-gas Roe dispatch always uses SIMD here. The older scalar Roe implementation
  is not substituted in the proof or source connection.
* The actual explicit-Euler caller does reach the same scalar AddSolution overload; the
  former implicit call-site observation did not establish this and was checked afresh.
* The new composed model includes masks, source-order scatter, truncation addition,
  volume/local-time scaling and parsed scalar update, with a boundary-processing interface.
* Unequal local pseudo-time breaks the inference from interior residual cancellation to
  global updated-state conservation. A Lean counterexample demonstrates this limitation.
* The moving-wall repair remains in an existing open PR and is not imported into this
  fixed-mesh baseline. Historical repaired/parent evidence is retained with its original pin.
