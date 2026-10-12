# Batched native 3D dependency admission

2026-10-12. Branch codex/native-3d-transport-accounting, based on
679fe68ef414551e754eb8bf71581b4bea4e0a74. Goal1 ACTIVE, full scope retained.

`Directory::LookupBatch` returns one complete dependency star per feature using
existing query/reply tickets and shared exchanges. Each candidate retains the
64-cell limit. Missing/oversized stars refuse only their candidate; common byte
admission, malformed input/footer or inconsistent shared identities refuse the
whole batch. Closed vertex/edge/face dependencies remain authoritative; no mesh
or directory publication occurs in lookup. Callers MUST check both common batch
validity and each candidate's validity before importing or reconstructing it.

Legacy Lookup uses the same implementation with its original merged64-cell
union policy, wire records and collective sequence. Its map allocation pattern
is retained. Batched output maps, shared-identity checks, footer statuses and
candidate buffers are included in conservative requested-byte admission. Global
allocator/RSS calibration is not established by this model.

Targeted MPI1/2/4 checks pass:65independent stars despite the legacy union cap,
byte equality with five separate legacy calls, repeated features, sorted face
and edge features, oversized65-cell and missing stars alongside good candidates,
empty peers/all-idle calls, feature-count/malformed/memory refusal and unchanged
mesh/directory snapshots. A complete footer with no dependencies is rejected.

Current full102-case mixed C++ regression passes per rank on MPI1/2/4, along with
fresh independent bridge and seven distributed-record suites, actual current/BDF
state and SU2/CGNS readbacks, default/rejection/actual-driver C++ checks and serial
MMG2D/3D controls. The runtime-only auditor is not applied to mixed logs; no new
independent runtime certificate is claimed. Numerical JSON records match the
previous static-fix checkpoint exactly:199/331/511 for MPI1/2/4 (1,041 total).

For five equivalent lookups, existing World wrapper counts are30separately vs6
batched per rank. Total serialized payload is640bytes in both paths (including
self traffic); returned dependency bytes agree. Direct failure elections, MPI
internals and network/wait are not these counters. This demonstrates a reduced
metadata coordination count, not elapsed speed or useful remeshing throughput.

BATCH_LOOKUP_VALIDATION.json pins all cases, compiled source/68objects/archives,
auditors and numerical equivalence. Binary SHA256:
fb76dd30b4f8a7f17a4d3ad5063cbe6c403fb77436f128cae1cc4e343cad7778.
Initial targeted case folder: batch_lookup_v1/mpi1,mpi2,mpi4. Exact source snapshots
are retained before the nonempty-footer guard and explicit counter reporting.
Current full folders: batch_lookup_regression_v1/mpi1,mpi2,mpi4. The parent holds
raw logs; each case holds independent audits, inspectable grids/donors/states.
OneCPU/nice19/j1/thread1/MPI<=4 correctness after actual resource checks; all jobs
terminal. About52.8MB retained, no copied executable/object/archive or symlink.

New core cluster bundles declare101cases; old97 and frozen100 declarations remain
supported. Actual serial registration confirms101, and34lightweight package
checks pass. No new scheduler submission or local performance campaign.

Next implementation: batched payload fetch with per-candidate validity and
shared-cell consistency, per-candidate reservations with full vertex conflicts,
then private multi-proposal engine reconstruction/validation/atomic publication.
Keep frozen original-P1/actual-query geometry and q/J/L contracts. Measure the
whole transaction and return/rebuild/transfer, not only lookup. Engine currently
still proposes one edit per worker per round; default512completion, repeated
cost/scaling/weights/wait/memory and fullGoal1 gates remain open. No curved/3DBL/
physical-unsteady or affordability claim. AdapNoExt and foreign work untouched.
