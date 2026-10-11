# Native 3D bounded reader return — 2026-10-11

Repository `/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated`, branch
`codex/native-3d-core`, predecessor `4c9f56b8c861433158e58459ee0d5c157742bb9b`.
Full Goal 1 remains active and incomplete.

The native tetrahedral return now calls a recoverable overload of
CReaderSlices::FromDistributed. Collective requested-storage admission precedes
validation scratch, point rendezvous, duplicate-ownership signatures, linear
point/reply buffers, replicated volume rows and root boundary collection.
Incoming point/signature counts and routed volume byte counts are exchanged
before the associated payload allocation. The root is charged for the complete
global physical boundary collection, including decoded-row vector growth.
Caller storage, old output, input arrays, tree nodes, requester/vector growth,
flattened send/receive buffers and passive transport scratch are included.
Refusal elects a common reason containing phase, requested bytes and ceiling;
the output is published only after all phases succeed. Saturation refuses even
with SIZE_MAX as the nominal ceiling. Invalid input retains existing fatal checks.

Native reader assembly also now includes both maps, ordered points, keys,
coordinates, metrics, volume/reference/boundary connectivity and marker names.
Its outer profiling exports the corrected maximum and includes reader admission
in reader_seconds. The original sensor interpolation and actual-query geometric
composition are unchanged. Native2D/MMG still use the two-argument reader overload,
which has no new budget collectives or allocations. No configuration knob, target,
operator or quality/sliver criterion changed.

## Verified evidence and scope

READER_CHECKPOINT_VALIDATION.json pins final code, the analysis and receipts.
reader_runtime_v1 retains the interrupted initial build: review found an admission
block accidentally duplicated into the complete-mesh unpacker; the owned build
was stopped before source correction. Its FAIL receipt and source snapshots are
preserved. The corrected reader_runtime_v2 is PASS,42 stages, actual complete
production archives/executable/test objects, MPI1/2/4:

- Collective budget refusal, populated/empty output preservation, retry,
  SIZE_MAX caller saturation, empty input ranks, 127-byte transport rounds and
  exact bounded-versus-legacy reader buffer equality, in both dimensions with
  and without a point metric. Later new-peak phases are refused on rank0 and the
  last rank; unreached phase counters remain zero.
- 84 actual driver accepted events,42 detailed OFF/ON controls and42 SU2/CGNS
  readback roundtrips, including all M1..N at each N1/2/4. Independent positivity,
  incidence/embedding/marker/volume/metric and affine current/BDF checks pass.
- 36 further actual-computed-sensor lifecycle events, three fresh repetitions
  per N/M/detail;357 exclusive rank-stage and357 nested records remain closed.
- Native2D/MMG/default ownership/private-progress rejection regressions pass.
  No numerical guard or rejection state-preservation contract was relaxed.
- Against the predecessor:126 accepted/readback mesh frames,84 normalized
  transported current/BDF/metric frames and36 computed-metric lifecycle frames
  are unchanged, including original sensor tensors.

reader_skew_v1 is PASS,11-stage reader-only follow-up after adding one test.
All production archive/member pins equal the full-run pins; the only changed
source is AdaptationMPI_tests.cpp. It reruns all NativeBudget3D cases MPI1/2/4.
A disjoint-tetrahedra transport fixture routes every point key to rank0, refuses
there at the linear-point phase after earlier phases pass, preserves output and
retries to the exact legacy result. This is a routing/memory adversary, not a CFD
or adaptation-domain qualification. Seven per-rank bounds are in phase_bounds.json.
At N4 the root peak is18,258bytes, other ranks10,419bytes; rank0 linear-point
admission exceeds its prior-phase peak15,170bytes.

Main runtime records retain336 rank rows. Largest modeled bound is8MiB, dominated
by the existing conservative reference allowance; maximum process-lifetime RSS
in those rows is37,904,384bytes. These are different quantities. No statement
about tightness, allocator-inclusive peak or a practical mesh-size limit follows.

## Cost and remaining work

The bounded overload adds three count Alltoalls and nine scalar Allreduces on a
successful native reader construction; failure additionally elects its reason.
These calls are timed inside the existing reader/return stages. Full house/vendor
MPI traffic and wait accounting still remain incomplete; World counters alone do
not count these reader collectives. Existing legacy calls incur none of them.

reader_phase_comparison.json retains three matched detailed-ON samples per N/M
for reader, whole return and outer testcase maxima. Phase maxima are not additive.
For example N2/M2 reader medians are0.0940ms old and0.1184ms new, whereas N4/M4
are0.3116ms old and0.1643ms new. The campaigns ran at different times on a machine
with foreign four-rank FSI work; that variability prevents isolating admission
cost or claiming speedup/scaling. Fresh repetitions, exact mesh/target agreement
and retained parent timers permit a later controlled cluster comparison.

Allocator/MPI/ParMETIS/CFD internal workspace calibration and some native
query-cache scratch models remain open. The tree-node/factor-three vector model
uses the current standard-library growth assumptions; it is requested storage,
not total physical memory. Full MPI wait/traffic, scheduled unsteady sampling
detail, larger/rotated/thin/restart lifecycle envelopes, practical rank/size limits
and closed SGE campaigns remain Goal1 deliverables. No full BL/curved/unsteady3D
or CFD affordability claim is made. Do not ask the unavailable user for launches.

## Replay and inspectable results

The regular run_runtime.py includes the new tests via NativeBudget3D; execute it
with a new absolute output folder using the documented configured local build.
The reader-only follow-up retains its exact run_script.py and final pins.
Regenerate comparisons/manifests locally on the pinned checkout with:

    python3 integration_evidence/native_3d_core_v1/analyze_reader.py

Inspectable actual computed-metric cases are in
reader_runtime_v2/lifecycle_mpi4/ (lifecycle_m1_d1_r0.json, corresponding
mesh_00001.su2 and immutable native_ref, CFG).
Donor/accepted metric/current/BDF states and CGNS are in reader_runtime_v2/mpi4/
(runtime_barycentric_m4_d0_s2_states.json, its mesh JSON and actual
runtime_barycentric_m4_d0_mesh_00003.su2/native_ref, written_CGNS.cgns).
Compact receipts/logs/audits/meshes/states/failures are retained; ELF/object/pyc
files are excluded from Git. Other workers, submodules and ClusterResults remain
untouched. Owned heavy checks were sequential nice19/thread1, buildj1, MPI<=4.
