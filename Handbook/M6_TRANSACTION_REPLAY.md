# M6 transaction replay — diagnostic56

Current: the single bounded MPI capture and initial-candidate replay pass. All26 recorded construction calls and11 shape refusals reproduce exactly
in the standalone replay (offline2); the bounded fixed-skin repair harness is being
corrected. No accepted local repair or full-M6 mesh has been obtained; DC-17/DC-18 remain
open. This implements the next experiment from the
[independent review reconciliation](M6_REVIEW_RECONCILIATION.md).

## Verified capture

Evidence: `integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/diagnostic56`.
The source/runtime/input-pinned six-rank run is terminal (session94073, exit0):
72.287s native, 80.475s launch, work2048 rejection. All ten diagnostic55 numerical
work/state counters match on every rank. This does not prove identical full trajectories.
Sampled summed process RSS peaks at3,636MiB, largest sampled driver619MiB;
shared pages are counted repeatedly, so neither is a physical whole-job peak.

At round4, edge23754–24333 on rank2 has:

- A saved global current mesh of303,428 cells and61,353 nodes, with unique owners
  and consistent shared coordinates. The complete initial edge star is cells114549
  and136288. Independent checks verify722 dependency records across the70 events.
- A byte-identical first split and exact first shape-failure message on replay.
  Initial patch minima q=.232448088/J=.062308272; candidate minima
  q=.183971960/J=.042789630. Maximum integrated candidate edge1.598362316 passes1.8.
- A reconstructed directory confirms the initial dependencies, versioned reservation
  for the single candidate, and outside-contact clearance. This does not replay
  competing peer reservations or publish a replacement.
- Twelve `ReconstructSplitPatch` calls and twelve successful growth reservations,
  followed by thirteen `Reconnect` calls. Two reconstructed fans report retained-skin
  visibility failure; other fans fail shape. One reconnection passes construction
  but fails geometry-error progress. The transaction remains rejected.

The previous rejection-only snapshots could not show this complete outer trial
history. The12-growth cap is now observed on this occurrence; exhausting it does
not prove that more growth would help or that the requested mesh is impossible.

## Implementation and scope

`EngineOptions` has an opt-in first-feature observer, forwarded by the native
remesher and consumed only by hidden tests. It snapshots every worker's current
cells before construction and records the selected outer geometry/metric trials,
versioned dependencies and donor batches. Callbacks cannot enter collectives;
diagnostic selection/error votes occur in the engine only when enabled. The
ordinary numerical policy and all mesh gates are unchanged.

The test helper freezes boundary parameters rather than retaining the live mutable
view. The six captured parameter payloads agree byte for byte. Storage is bounded
and truncation invalidates the capture. Serialization occurs after the timed native
event. Large whole-mesh/parameter snapshots stay local with hashes and reconstruction
scripts; compact reports, code and receipts are reviewable checkpoints.

The first compressed archive exceeded the25MiB output limit because replicated
parameter payloads were stored six times. The failed size check is preserved in
`analysis.log`. Hash-checked lossless deduplication reduces the complete archive to
25,114,331 bytes, below25MiB; `capture_archive_receipts.json` maps every logical file
to its stored bytes. `analysis_final.log` passes. This did not drop captured state.

Independent exact-containing-donor centroid analysis (`candidate_ranking.json`)
checks all11 geometrically constructed split/fan candidates. The initial four-cell
split is best by min(q/.20,J/.05)=.85579; the best grown fan scores only.55782.
Some better-looking first-failed cells therefore conceal worse whole-patch minima.
Growing these particular fans did not approach a fully admissible replacement.

The initial standalone launch (offline85224) refused reference decoding before
any construction replay: a1GiB conservative allocation allowance was incorrectly
used for the intended1GiB actual-RSS limit. The corrected test uses the existing
2GiB decoder allowance, while the serial launcher monitors process RSS and terminates
its owned process group above1GiB. This setup failure is not meshing evidence.

Offline2 reproduces all26 construction calls and11 shape-refusal messages exactly
(2,404 assertions pass before a setup exception). It does not replay growth selection
or the MPI schedule. The repair phase then fails its harness setup: wrapping an
unchanged sensor in a deadline composition requires a geometry edge policy. The
correction retains the original uncomposed field and enforces CPU/wall limits in
the launcher; it does not add dummy geometry bands or change metric arithmetic.

## Remaining experiment

Use the saved input records to reproduce every recorded construction call and
shape refusal before interpreting an alternative. This is local call replay, not
re-execution of the MPI peer schedule or proof that the growth-selection policy is
optimal. Then test bounded existing connectivity repair and, if needed, an interior optimizer
from the same initial candidate, preserving its curved skin. The alternatives share
256 trials and60s, and do not chain an incomplete repair or change production policy. Restore Absolute admission for the terminal
transaction; independently check all integrated edges, geometry/interface, source
queries and external contact. Report transaction and final-patch acceptance separately.

No second MPI prefix, larger construction caps, production shape-policy switch,
new partitioner, full1800s campaign, conservative transfer or restart is justified
by the capture alone. Preserve source/build pins throughout each running stage.
