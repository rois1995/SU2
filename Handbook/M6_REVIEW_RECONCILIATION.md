# Independent M6 review reconciliation — 2026-10-26

Reviewed the [supplied report](reviews/M6_INDEPENDENT_REVIEW_f5e9456f06.md)
against checkpoint `f5e9456f06`, saved diagnostic55 witness/timing records and the
current metric admission and streamed measurement code. The report is preserved
verbatim. No production numerical defect requiring an immediate code change was
demonstrated. DC-17/DC-18 remain open; no accepted full-M6 mesh or transfer exists.

## Accepted findings and corrections

- Construction/certification is the next question to resolve. Positive orientation
  is insufficient, and the early surface witnesses do not establish the cause of
  the late full-M6 timeout. The review's priority recommendation is sound; a claim
  that it has proved the dominant full-run bottleneck would go beyond its evidence.
- Added a first-failed-cell versus whole-patch table to the construction report.
  Edge 81–17687 has candidate J minimum 0.019286643, despite the first failed cell
  reporting J=0.096229616. Its old patch already has J minimum 0.039710928.
  Edge 23754–24333 starts with a shape-compliant old patch and is the cleaner first
  experiment. Both candidate patches fail q and J.
- Retained the explicit warning that geometry-failed `fresh` records may be partial
  or stale. Their positive volumes do not certify a constructed replacement.
- Changed the 1.428 figure from a conditional "ceiling" to a hypothetical accounting
  quotient. MPI duration is neither pure communication nor causal waiting; fixed
  outside-MPI time is not invariant under a coordination change.
- Clarified that matched operation counters do not establish identical execution
  trajectories. The shared AllreduceMax helper still lacks semantic attribution.
- Weight saturation is absent only in the offline replica. The aggregate matches
  production, but per-cell equality and useful-work prediction remain unverified.
- Original input mesh, metric and authority data are local, hash-pinned inputs.
  The external review checked committed source/receipts, not those input bytes or
  a numerical rerun. This distinction does not invalidate the local saved runs.

## Next experiment and implementation qualifications

Adopt the review's single-edge, complete-context capture/replay specification;
the [decision document](M6_DIAGNOSTIC_DECISION.md) records the bounded protocol.
Prove faithful strict replay before any private alternative. Report local transaction
acceptance and the stronger final-patch edge certificate separately. Source inspection
confirms `ValidateMetricChange` can carry residual oversized edges under its progress
rules, whereas `ValidateMetricPatch` enforces the absolute maximum of 1.8.

The existing test observer cannot implement the proposed pre-transaction snapshot:
it sees rejected patches after construction, without ownership/version, complete
external-contact coverage or growth history. Bounded diagnostic capture must reach
the existing engine transaction/dependency/source facilities; calling it test-only
does not make those missing records available. Avoid creating a second meshing engine
or replacing the frozen field with centroid-only measurements. An incomplete capture
must be rejected before interpreting repair feasibility.

`SetShapeAdmission` changes a process-wide atomic policy. An isolated alternative
must run outside live remeshing, restore the previous policy on every exit, and use
Absolute admission for its terminal candidate. It cannot safely switch only one
concurrent transaction. No production policy change follows from this review.
The user's final-only quality-policy intent remains a separate open integration
requirement: current production explicitly selects Absolute, and merely toggling
ValidityOnly has not established a successful complete construction.

Use the named edge only as a diagnostic selector, never as production dispatch.
Stop on missing context or resource truncation; neither outcome proves infeasibility.
Defer weight changes and broad MPI optimization until this construction question or
specific semantic coordination evidence supports them. A successful local replay
still leaves full M6 Euler/RANS, curved BL, transfer and restart to qualify.

## Provenance and checks

- ZIP SHA256: `5598d2acf6a10b7d2db42901798b1907a98dff1277bd717747f589421b293fa6`.
- Verbatim report SHA256: `78aa583db02bb3201e67a6ab715e99bac6420378f47d7ca0af720924221185eb`.
- Cross-checked both exemplar rows against `diagnostic55/witness_analysis.json`
  and raw witness records; inspected streamed first-failure measurement and both
  metric validators. Documentation changes preserve original receipt hashes.
- No new build, MPI campaign, numerical replay or paper experiment was run for
  this reconciliation. Existing passing evidence is not broadened by this review.
