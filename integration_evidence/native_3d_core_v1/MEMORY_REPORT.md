# Native 3D runtime memory checkpoint

Repository: `/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated`.
Branch: `codex/native-3d-core`; predecessor `0864f56e60f0f7d66e5db6cf8779cb8aaa1703c7`.
Goal 1 remains active and incomplete.

The initial CFD tetrahedron import now elects storage refusal before point-directory
payload allocation and before physical-face/reference construction. Failed imports
leave the caller's output and original reference unchanged; accepted CFD mesh and
solution are read-only. The original four-argument import entry point remains
available. Runtime admission also includes caller-held geometry authority, native
maps, donor vectors and partition result buffers, with shared immutable authority
counted once. Models use saturating arithmetic and requested container storage.

The runtime moves the private imported map into the working mesh, borrows the
original source when native repartition is disabled, and releases replaced maps
and unused split/collapse records before coarsening. It keeps original-P1 donor
interpolation, private/remote query behavior and existing numerical gates. The
3D production path still requires planar geometry, adapted surfaces, no BL and
`ADAP_HESSIAN_NOISE=0`; this checkpoint adds no 3D BL capability.

Each NATIVE3D rank row records `native_bound` separately from `process_peak_rss`.
RSS is the process-lifetime high-water mark from getrusage, including CFD/test
allocations; it is not an event-local peak or a measurement of native heap alone.
Tracked communication now includes the reader-return exchange. House helper
traffic remains outside this counter.

## Validation

`MEMORY_CHECKPOINT_VALIDATION.json` links the terminal receipts and their hashes.
`memory_build_v1` built actual configured production archives, CFD executable and
affected Meson unit objects (nice19, j1). `memory_runtime_v1` passed all 33 stages
using those complete actual archives, with unchanged source/library/object pins:

- New budget tests passed MPI1/2/4: one-rank elected early/late refusal, preservation
  of initialized and uninitialized reference/output, retry and SIZE_MAX overflow.
- MPI1/2/4, all M1..N, barycentric/conservative transfer, three refine/coarsen/refine
  events and detailed timing OFF/ON: 84 accepted actual driver events,42 strict
  fixed-work controls and42 actual SU2/CGNS parse/reader roundtrips.
- Independent geometry/incidence/embedding/volume/marker/strict frozen metric and
  current/BDF2 state checks passed: qmin0.6280684264164493,
  Jmin0.3973691568036606, Lmax1.793741946119929, transported tensor error0,
  maximum affine state relative error1.1196825653302053e-12.
- 126 accepted/output-reloaded mesh frames exactly match the published runtime_v3
  signatures and original sensor samples; normalized transported states also match.
- 336 runtime rank rows close their remesher phases and report memory/RSS. The
  largest modeled bound is8MiB (the conservative reference-setup allowance); the
  largest process-lifetime RSS in this runtime matrix is38,666,240bytes. These
  numbers cover different quantities and must not be compared as a heap certificate.
- Native2D driver, MPI-aware MMG2D/3D and default no-native-repartition regressions
  passed MPI1/2/4. Default-path refusal after private accepted edits preserves
  accepted state and retries successfully; its raw SU2/reference audits pass.

All numerical checks use small manufactured cubes/affine fields, without CFD
iterations. Host load remained contended by a foreign four-rank FSI job; all owned
runs were sequential at nice19 with one numerical-library thread. No speedup or
scaling claim is made from these correctness timings. The reusable run_runtime.py
now runs the new budget tests before the driver matrix.

## Inspectable results and remaining work

See `memory_runtime_v1/mpi4/runtime_barycentric_m4_d0_mesh_00003.su2`, its
`.native_ref`, `runtime_barycentric_m4_d0_s2_written_CGNS.cgns`, and the adjacent
`runtime_barycentric_m4_d0_s2_states.json`/mesh JSON for original donors, accepted
mesh, transported metric/current/BDF histories and immutable facets.

The bound excludes allocator metadata, MPI internals, CFD buffers and internal
reader/partitioner workspace. Existing reader-assembly/field-cache estimates still
need calibration; this is a memory-admission improvement, not completion of the
full bounded-memory requirement. Full metric preparation, geometry/solver rebuild
and transfer per-rank exclusive lifecycle accounting, actual restart qualification,
larger/thinner/rotated lifecycle cases, repeated instrumentation overhead and
practical size/rank limits remain required before Goal1 completion. Prepare closed
SGE campaigns after those profiling/qualification gaps are addressed; do not ask
the unavailable user to launch cluster jobs overnight.
