# FP1 whole-item source check

Checked before the first production edit against `a59380df18`. The scoped production and test paths show no tracked changes from the FP0/N22 baseline. The archived order matches SHA256 `3f195641c334c78f260cfdb140887615a8a776fcbfcc8182cc80e2d6f46a624f`.

The missing capability is deterministic validity-first interior adaptation across the worker partition, with physical faces and surface points fixed during the core. FP0 measured the component costs and a separate Refine on an already valid exit mesh. Acceptance is staged: local operators and replays (A), deterministic rank-border migration and repair (B), default-OFF identity followed by two full qualified ON workflows (C). ON acceptance requires the unchanged two-event physical, transfer, restart, time, storage, coverage and output-determinism gates.

## Source and reading used

- Section 3 calls the FP0 point check bitwise. The prototype uses floating equality, which accepts opposite signs of zero. Reading used: compare the individual coordinate and tensor component representations. The new source uses the existing strict-arithmetic library and preserves the FP0 file.
- Section 5's explanatory equality `c = S / 2` is approximate because `S` has the specified margin. Reading used: keep the two literal definitions; do not add a hysteresis rule. The sweep limits still apply.
- Section 7 refers to the collapse stamp rule in the split sweep. Collapse defers on the star of an eligible removal end. Reading used for a split: defer if either endpoint has a cell stamped by an earlier split in the sweep, including a frozen endpoint of an otherwise eligible closed ring. A scratch stamp per point in the split sweep accounts for all local components of a border point's star.
- Section 8 says donor ids count toward the bases. EngineSetup conditionally scans original donors only for curved geometry, and CPassiveComm supports the prescribed uint64_t reduction. Reading used: scan working rows and worker_source on every worker for both base ids. Allocate ids only for successful point changes and writebacks. Sort local cavity choices by point ids so slot changes do not affect point-id creation.
- Section 9 points at World::exchange and existing migration. World::exchange already counts its send buckets; include only the additional live buffers in its caller baseline. Reuse exact OwnedCell serialization and admit the replacement map before publishing. AdaptWorker has a worker-only communicator and catches CElectedFailure. Elect local exception messages before going to another protocol collective.
- Section 10's call boundary is exactly before SnapshotSource in NativeRemeshRun.inc. Current-nodal cells already carry sensors there; SnapshotSource copies their current state and ApplyMetric runs the existing engine, which uses its stage source. Current-nodal FrozenField uses the one-piece endpoint formula, and MeasureMetricCells queries the centroid. EvaluateCavitySample keys equal parents by sorted point ids. No production engine policy needs changing.
- Section 10.2 concerns the timer tree. Legacy stats.phases[8] is recorded around ApplyMetric, so existing legacy phase and other accounting can stay intact while the timer tree attributes the core with Construction / Topology. The OFF option adds no timer scope or worker collective.
- Section 12 says only the stated files change. Parts A and B leave the remesher, options and G5 config alone. The section 13 handbook changes belong to reporting after the gated parts. The C option follows the existing fourth config location, the native-remesher option guard.

## R3 evidence-helper corrections

- The first build597 preflight stopped before any compiler launched. The pin updater covered source and test files but omitted the newer, tracked Work order 3 in AGENTS.md. Included that committed change in the refreshed source manifest and retained the preflight as a failed attempt.
- The initial build helper shared the module name of FP0's runner. Renamed it to `build_fp1.py` so the G1 and build helpers can be imported together.

- Build597 compiled the four core source files. The operator fixture then failed to compile at its first `Edge` declaration, where both namespaces provided an `Edge` type. Qualified the two fixture declarations with the local namespace. Waited for the build controller and remaining compiler to exit before editing the frozen inputs.
- Independently of the fixture correction, the pre-gate source review found that sweep completion used counts from the report struct. Wired completion to accepted edit results and gated the printed counts on the timing flag, as required by R11 and section 10.4.
