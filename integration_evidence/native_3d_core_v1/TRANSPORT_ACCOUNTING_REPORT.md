# Native3D transport accounting — 2026-10-11

Repository: /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated.
Branch: codex/native-3d-transport-accounting, based on pushed core checkpoint
6b3fefd039bfc3d1937b8add7d21d0ea41344bc9.
Status: configured C++ build and serial regression/audit PASS; MPI qualification pending. Goal 1 remains ACTIVE.
The cluster checkpoint on codex/native-3d-core is unchanged.

Implementation/evidence checkpoint475820f366366f471e0cdb50e362d7c5e572c328 pushed to origin/codex/native-3d-transport-accounting; full remote SHA verified. origin/codex/native-3d-core remains6b3fefd039bfc3d1937b8add7d21d0ea41344bc9. Subsequent publication-note commit changes documentation only.


## What was missing

The runtime exported only World-wrapper bytes from its parent and worker Worlds.
Geometry import creates a separate World, so its counted traffic was omitted.
The parent total was assigned only after reader return. A rejection before that
phase therefore lost any already-counted parent work. Existing World wrapper
calls/elapsed time and directory-election counters were not exported in 3D.

## Change and counting contract

BridgeStats now records per-call byte/call/elapsed deltas through its existing
Envelope guard. Successful imports already copy stats after that guard closes;
there is no new publication mechanism. Weight/return calls use the same parent
World and subtract their starting counters, including on refusal. EngineStats
copies per-call deltas in its existing finish path and exposes already-collected
directory-election count/time. Remesh captures attempted parent/worker work
before report emission and exports three disjoint World totals:

    tracked total = import World + parent World + worker World

Weight/return detail is inside parent totals. Refine/coarsen detail is inside
worker totals. Never add these nested scopes again. An elected exception that
escapes an engine before finish can leave that engine's returned detail absent;
the surviving worker total still captures the attempted wrapper work.

Only scalar snapshots, arithmetic and existing record fields are added.
There are no additional runtime queries, MPI collectives or clock calls.
No mesh operator, sensor interpolation/composition, target, q/J/length guard,
BL/fade/noise policy, worker selection or 2D/MMG code changes.

## Limits of the measurements

World bytes are packed payload bytes sent, including self traffic and replicated
metadata. They exclude count-exchange and reduction payloads. A wrapper call
to AlltoallvRounds counts once even if its implementation makes several rounds.
Elapsed collective time includes internal packing/progress and possible wait;
it is neither pure MPI wait nor a complete communication elapsed total.

Failure elections, direct CPassiveComm/ADT point-directory calls, communicator
operations, ParMETIS/vendor internals and CFD rebuild/transfer communication are
not covered by World-wrapper totals. Directory elections count directory API
calls, with their existing nested elapsed timer; these are not individual MPI
calls and must not be added to the wrapper totals. There is no wall-minus-CPU
waiting inference. Full traffic/wait/allocator/vendor calibration remains open.

## Tests and evidence

Existing selected C++ cases retain their 69-case selection and unchanged meshes/
targets. Assertions compare actual per-call World deltas after engine success
and injected engine/weight/reader query rejection, including reused World
objects and timing OFF/ON. Default runtime tests check disjoint total closure,
nested subset bounds and failed private progress without accepted-state change.
Budget tests verify successful imported stats are identical to the finalized
external stats while refused output/reference sentinels remain unchanged.

The shared independent auditor validates complete new fields, integer/nonnegative
counts and import+parent+worker closure. Profile summaries label historical
bytes-only logs as legacy_bytes_only and logs without counters as not_measured.
Old results are not upgraded to current transport qualification.

package_checks_v11 initially passes 19 synthetic/mock checks. package_checks_v12
passes19 checks (0.480s unittest /0.778s wrapper), the final-source repetition after tightening partial-schema checks and including
the changed C++ test recipes in exported bundles. Lightweight checks do not
establish numerical/scaling success.

transport_build_v1 passes all 14 actual configured nice19/j1 build steps
(253.908s), with all68 driver objects,774 thin-archive members and source/binary
pins. Resource gate measured at least25% idle CPU and load below8 before launch;
compiler scratch stays on4TB. Existing 2D/MMG implementations are unchanged.

transport_serial_v1's actual native process passes10189 assertions in69 selected
cases; its serialMMG process passes48 assertions in2 cases. The independent
bridge process passes12 exact geometry/marker/metric mesh frames,6 affine-current+
BDF state pairs and6 actual SU2/CGNS output/reader round trips, with5 corruption
controls rejected. Maximum state relative error is2.7000623958883732e-15.

The wrapper then failed importing audit_lifecycle because its collector omitted
the helper directory from sys.path. Preserve that original FAIL receipt. The
postprocessing-only transport_serial_audit_v2 explicitly imports the helper,
reverifies every original artifact/source/binary/object/archive pin and successful
process exit, and returns SERIAL_AUDIT_PASS without rerunning numerical work.
All26 non-CGNS raw meshes/reference/readback/state fixtures are byte-identical
to reference_admission_serial_v1. Three CGNS container bytes differ; their
independent exact mesh signatures and actual reader round trips pass.

The default runtime emits two actual one-rank rows:
- Rejected private-progress event:46296 tracked bytes =4224 import +11040 parent
  +31032 worker;80 tracked calls.
- Accepted event:276144 tracked bytes =4224 import +23544 parent +248376 worker;
  955 tracked calls.
Both also close elapsed wrapper time. These self-inclusive wrapper values are
not physical network traffic or rank-scaling evidence.

## Remaining qualification and next actions

The feature branch is isolated from the user's core6b3f cluster campaign.
REFERENCE_ADMISSION_MATRIX adds only the two changed production/test source paths
to its original15-path expected footprint, retaining69/full68/MPI1/2/4,
26 byte-identical n4 files, unchanged operations, strict geometry/metric/state
and actual SU2/CGNS persisted-restart gates. Its qualifier additionally requires
current transport schemas. None of those pending MPI/n4/restart campaigns has
been completed for this branch. The small serial bridge is not their substitute.

No owned job remains live after collection. MPI2/4 expansion was deferred while foreign workloads remained active;
do not infer performance from these short tests.
Prepare a newly built/frozen bundle only when the feature branch is selected;
never modify a running core package or combine binary/package identities.
Goal1 also still needs repeated cluster timings/instrumentation-overhead checks,
actual n8 size/rank pilots and full traffic/wait/memory calibration.
No speedup, robust3DBL, curved geometry, full unsteady3D or affordability claim.

Publication checks: code/docs and all retained files pass Git whitespace checks except the two original Catch stdout logs, which end in an extra blank line. Preserve these hash-pinned raw logs unchanged.
