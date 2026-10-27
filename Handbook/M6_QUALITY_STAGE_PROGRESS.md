# M6 quality-stage progress

Reviewed design, 2026-10-27. Measurement/capture implementation is integrated in active
build18/session62205, not yet validated; this is not a convergence change. Complete accepted M6 adaptation remains the milestone.
See [staged adaptation](STAGED_NATIVE_ADAPTATION.md) and
[curved boundary mobility](CURVED_BOUNDARY_MOBILITY.md).

**Evidence and its limits.**

The pinned build13 six-worker Euler run reached quality improvement and rejected
at its unchanged 1800-second native deadline. Solver exit 0 did not mean native
acceptance. Its receipt is
[diagnosis.json](../integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/m6_runs/euler_build13/diagnosis.json).
Across workers it retained 774,314 private cells, 275,115 cached-invalid cells and
1,240,202 repair demands after 112,024 private publications. Three final cells lacked
a completed census. Work was 1,218,977/19,414,976; coarsening was not entered.
These totals do not show whether whole-stage debt decreased. Final samples had
unavailable q/J, and do not establish that boundary pins caused rejection.
The Euler configuration requests no first height; height remains essential to the
general Euler/RANS workflow, but is not an evidenced cause of this run's failure.

Current admission summarizes shape by a cavity minimum:
`Common/src/adaptation/CNativeMetric3D.cpp`, `ValidateMetricChange`, compares
`min(minimum_q/.20, minimum_J/.05)`, or independently accepts size-excess progress.
Validity-only quality progress may add size debt. `GeometryRepair` relies on the
caller's certified geometry/height progress without a combined debt comparison.
In `CNativeEngine3D.cpp`, `repair_kind` uses worst height merit;
`MeshOptimizer::RowScore`, `MinScore` and `FreshScore` also use minima.
`MeshOptimizer::Smooth` already proposes a sum-of-shape-hinges Gauss–Newton step,
but candidate admission still requires improved minimum score.

Two algebraic counterexamples describe this certificate, not demonstrated M6 moves:

- Scores [.2,.4]→[.2,.8] improve total shape debt but leave the minimum unchanged.
- Raising one minimum from .2 to .200001 can pass while many other scores fall
  from .9 to .21, increasing total debt substantially.

This is a concrete objective limitation; its actual contribution on M6 is unmeasured.
Increasing limits or adding another local proposal variant does not diagnose it.

**Measure before changing admission.**

Reuse the existing one-cell quality census/update in `Adapt::measure` and
`Selection::Add/Drop`. Cache per-cell q/J bad flags and squared normalized hinges
`max(0,1-q/.20)^2`, `max(0,1-J/.05)^2`, alongside existing size/geometry validity.
Keep counts and raw sums; averages can improve merely by inserting good cells.
For size, a per-cell sum of six edge hinges is cheap and ownership-independent;
label it incidence-weighted. Summing rank-local unique-edge maps would duplicate
shared edges and must not be called a unique global edge total.

Return the already computed `FirstHeightReport` optionally from `height_check`;
reuse its measured error/footprint values. Reuse `Selection::Add`'s existing
`SurfaceError` evaluations for geometry diagnostics. Do not run a second audit.
Construction adds only known edge/geometry data. Unknown shape/height stays
explicit; no construction centroid imports or q/J evaluation are introduced.

Add new rows and subtract removed rows during normal cache updates. Reduce a fixed
scalar record only at quality entry, existing progress windows, handoffs and exit.
Report separate channel sums/counts, measured/unmeasured counts, mesh size, and
move/reconnect/insertion deltas. No per-proposal collective is needed for trends.
This first change must leave admission, election, limits and final gates unchanged.

**Capture and replay.**

Retain the complete owner-local private mesh at first quality entry and rejection,
with each cell's ID/version/physical binding and matching active parameter payload.
Reuse `EngineOptions::rejected_state`/`SetRejectedStateObserver` for rejection.
A borrowed quality-entry observer at the completed `enter_phase` census is the
small missing hook; it must run at a collective-safe point, perform no collectives,
and retain no borrowed objects. Use rank-local shards, not a whole-mesh gather.

Reuse `fixtures/native_transaction_capture.inc`, `NativeFrozenVolume3D`,
`NativeTransactionReplay3D`, and the pinned `staged57/replay.py` runner. Their
bounded transaction capture supplies old/fresh patches, dependencies and donor
samples. Its donor union is only sufficient for that transaction: full-stage
replay needs the complete immutable original donor shards, not captured query
samples or a field rebuilt from the rejected mesh.

Save original sensor/source hashes once, immutable authority/reference version,
active parameter payload, configuration and composition/normalization scale,
wall-height policy, breakpoints, worker membership/ownership, phase/round/work
and ID-generation offsets, binary/source/helper pins, and complete-shard hashes.
Existing callbacks can be installed by the remesher test harness; no solver-label
dispatch or production configuration switch is needed. Captures remain explicitly
private progress, with final acceptance false.

A cold replay of `Repair` currently starts construction; it is not exact quality
continuation. Replaying the first quality-entry transaction can reuse existing
facilities. Exact mid-pass continuation additionally needs repair tickets, blocked
support/frontiers and remaining controls; do not imply those are serialized today.
Capture bytes/I/O must be admitted and reported; truncation invalidates completeness.

**Acceptance and subsequent decisions.**

Diagnostic acceptance requires unchanged construction centroid/q/J query counts,
no new per-proposal collectives, and no duplicate metric/height/geometry audit.
Cache sums must match existing measured fixture records and changed-row deltas;
ownership permutations must give the same owned-cell totals. Partial census and
truncated captures must remain explicit. Saved transaction replay must reproduce
its measured candidate data against the same original frozen field.

Aggregate gating could itself reject necessary progress: topology may temporarily
increase bad-cell count, and quality/height movement may need size or geometry debt.
Volume weighting can hide a vanishing bad sliver; unweighted sums penalize necessary
subdivision; fixed channel priorities can starve another channel. A lower scalar
merit does not certify feasibility or final acceptance. Use captured before/after
candidates to establish the actual tradeoff before choosing a shared optimizer and
admission objective or a phase-level progress rule.

Ultimate acceptance still requires complete M6 Euler then RANS adaptation, strict
q>=.20/J>=.05, edges<=1.8, requested first-height tolerance (±10%), prescribed geometry/features,
body separation/contact, complexity 500,000 and conservative transfer/BDF/restart
checks under unchanged resource limits. A diagnostic or local repair is no substitute.

Current implementation preparation: stage cache/borrowed observer, streamed owner
and immutable donor shards, bounded parameter payload, and explicit private
manifest. The opt-in frozen M6 diagnostic saves first quality entry and stops after
four measured improvement windows, saving `observation_stop` on every worker
before the collectively elected diagnostic exception. That deliberate stop is not
infeasibility or final mesh acceptance. Ordinary failure retains `rejected_exit`
when the observer is healthy. Function-valued composition/breakpoints and exact
queue continuation are explicitly not serialized.

The build13 `restart_flow_adap_00000.dat` has SHA256
`c91d06b38e840aaada5a8ac417d1f937cdda4b5f5de7d587f7d17bbfc08e236b`,
identical to diagnostic55's pinned extraction input. All 61,330 coordinates, IDs
and sensor tensors match the existing `sensor_original.bin` bit-for-bit. Reuse
that source with build13's configuration/authority; no repeated sensor extraction
or field substitution is needed. `staged57/run_cases.py m6_stage` (with buildN as
its first argument) pins these inputs and verifies complete rank shards, record
lengths, matching immutable donor hashes and four trace windows. Its standalone
`test_stage_receipts.py` rejects missing/truncated shards and changed donor data.
