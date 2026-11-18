# N17 M27 source check

Checked at phase C checkpoint `167e353ca7099c9892e9ffcadb3d182ca5557a4f` with build569. G3, G4 and GQ have passed. The pinned M27 handoff and its SHA-256 are in the phase C inputs.

`EngineSetup` creates the existing D9 root under `options.timings`. Its `Data` is local to one event and `active` is thread-local. The PartScope member belongs in the existing D9QueryScope. The four named guidance calls now live in `TrialChecks::GeometricChange` in EngineTrial.cpp. There are exactly two old-patch and two fresh-patch calls. `MeshOptimizer::SmoothBoundary` retains its two named calls, target loop, best-candidate update and boundary move counter.

`MappedFacetReference::BoundPhysicalSurface` has two bypass returns, then a memo hit branch and a computed miss branch. The canonical-or-physical query lies between the bypass branches, and the second bypass remains outside the memo query counts. Mark a cache hit after the existing stats updates and before its return. The miss work is the existing work delta before memo Store. Its clock surrounds the existing QuerySurfaceUnion call. The observation needs no additional geometry queries, reserve or admission decisions. The tick, work and memory limits already present still govern the workflow.

## R4 readings used

- The handoff uses the pre-cut name `geometric_change`. The source maps it directly to `TrialChecks::GeometricChange` through the two matching rejection messages. All six guidance calls keep their original arguments and evaluation order.
- The table row specifies face count and asks for the table's size against a memory expectation. Keep the specified fields and add an estimated byte count from entries, node links and buckets. The estimate omits allocator bookkeeping and is a report value.
- A first computation during Census is marked immediately as reached, as specified. The first-computation row receives the later-census credit only when a previously seen face reaches Census. A memo hit absent from this event's observation table is counted in the specified `from_earlier_event` bucket.
- The miss observer handles allocation failure locally by setting the failure flag and ceasing observation. There is one prior diagnostic root and no timer-tree or transported state changes. New map and counters are local to its Data as specified.
