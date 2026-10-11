# Native tetrahedral production runtime checkpoint — 2026-10-11

Working repository: `/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated`.
Branch: `codex/native-3d-core`, predecessor `5d83dcda7958f5af81ddeb3235b1a0a56f686a01`.
This is a validated, limited runtime checkpoint. **Full Goal 1 remains active.**
The combined qualification is `RUNTIME_CHECKPOINT_VALIDATION.json`; historical
failed receipts remain failed.

The opt-in native factory now dispatches 3D tetrahedra to `CNativeRemesher3D`.
It privately imports the accepted CFD mesh/sensor, estimates tetrahedral work,
optionally partitions, migrates original sensor donors to an explicit M<=N
communicator, refines/coarsens with the frozen core, and returns a distributed
reader and original-P1 sensor to the N CFD ranks. Accepted geometry reference
publication happens only after the existing driver finishes `ReplaceMesh`, and
is idempotent. Initial proposals/rejections do not publish the reference. The
versioned `.native_ref` sidecar retains original finite planar facets across mesh
renumbering and validates bounded counts, marker ordinals/names, points and closed
surface topology. Marker order follows SU2 config order rather than alphabetical
order. Shared 2D and MMG paths retain their existing implementations.

Requirements for this experimental 3D path: primal compressible tetrahedral FVM,
`ADAP_REMESHER=NATIVE_CAVITY`, `ADAP_SURFACE=YES`, `ADAP_HESSIAN_NOISE=0`, simple
faceted domains and no BL requests. Unsupported moving/paired/periodic/mixed
configurations are rejected. The original authority is bounded to 256 triangles;
this is a current capability limit, not a demonstrated general size envelope.
3D BL integration/generation, curved geometry and full unsteady CFD remain outside
this checkpoint. Current/finer original sensors are preserved; no composed wall
tensor is interpolated or hard-normal reset introduced.

## Qualified evidence

Actual configured Meson production libraries/CFD executable and test objects were
used; no standalone native/house overrides. Current qualified production source,
archives and thin archive member pins are verified by the checkpoint manifest.
One owned low-priority heavy job at a time, j1, MPI<=4 and library threads1; four
foreign FSI ranks were left untouched. Host records are retained.

- `runtime_v3`: MPI1/2/4, all M1..N, barycentric/conservative transfer and detailed
  timers OFF/ON; three refine/coarsen/refine events each. All **84 accepted driver
  events**, strict independent meshes and current/BDF2 transported state checks
  pass. **42 exact fixed-work OFF/ON pairs**, **42 actual SU2/CGNS output/reader
  round trips**, raw driver SU2 outputs/sidecars, 28 one-rank budget veto events
  and 336 per-rank remesher records. Immutable references and original sensors
  survive repeated replacements. Current and time_n both hold the end-step state;
  time_n1 is distinct. Timer controls preserve exact geometry/topology, original
  sensor and transported states. Frozen gates remain q>=.20, scaledJ>=.05 and
  metric edge<=1.8. Observed minimum q=.6280684264164493,
  minimum J=.3973691568036606, maximum L=1.793741946119929;
  transported sensor error0, maximum affine state error1.1196825653302053e-12.
- `runtime_regression_v1`: PASS native2D actual driver repeated replacements and
  affine state checks, plus existing MPI-aware MMG serial-versus-distributed
  bitwise mesh/reader comparisons in 2D and 3D, at MPI1/2/4.
- `runtime_defaults_v1`: PASS default N-worker CFD ownership with native
  repartition disabled, rejection **after private accepted edits**, unchanged
  accepted geometry/solution/sensor/reference, successful retry, transported
  metric and idempotent publication/sidecar. Three independent actual output
  audits verify strict geometry/marker/quality/sliver/metric contracts.

Partition choices produce different valid trajectories. For N4, M1 gives
48/6/48 cells, M2 48/6/47, M3 48/10/43 and M4 48/10/48. OFF/ON and the two
transfer methods agree at fixed N/M. Different N/M are changed-work comparisons.

Inspectable final N4/M4 mesh and corresponding states:
`runtime_v3/mpi4/runtime_barycentric_m4_d0_mesh_00003.su2` (original sidecar adjacent),
`runtime_v3/mpi4/runtime_barycentric_m4_d0_s2_written_CGNS.cgns`,
`runtime_v3/mpi4/runtime_barycentric_m4_d0_s2_states.json` and its mesh `.json`.
These are manufactured unit-cube meshes and affine states, not real CFD solutions.

## Retained failures and practical limits

`runtime_build_v1` selected a nonexistent executable target before compilation;
`runtime_build_v2` PASS. `runtime_v1` failed relative runner path handling before
build/MPI. `runtime_v2` reached initial accepted replacements but reference
encoding wrongly required lexical marker order; fixed without changing marker
ordinals or the mesh contracts. `runtime_v3/validation.json` remains **FAIL**:
a later incorrectly selected serial-only MMG round-trip test tried to load empty
nonmaster gathered meshes on MPI2. All earlier 3D stages/audits passed. The
unchanged existing MPI-aware MMG test and native2D checks then passed separately
in `runtime_regression_v1`; the qualification links both evidence sets explicitly.
No MMG implementation was modified to make that serial test pass on MPI.

Remesher phase rows close against their own per-rank computation wall time,
including an explicit residual and subset wait; reporting cost is separate in
statistics. Inactive ranks use a sleeping nonblocking barrier. Detail timers OFF
preserve counters and disable the query timers checked by tests. This does **not**
yet close metric preparation, full rebuild/transfer/output at per-rank resolution.
The existing replacement reports give per-phase maxima, which must not be summed
into elapsed time. Tracked communication is not every house helper's traffic.
The budget veto proves collective rejection and preservation, **not** complete
aggregate memory admission: caller-held copies, initial import/workspace budgets
and peak RSS still need a full audit. One contended diagnostic matrix supplies no
performance, scaling or affordability claim. Driver reload/restart and larger,
thinner/rotated actual-lifecycle targets remain pending.

## Reusable follow-up

`run_runtime.py /absolute/new/result/folder` rebuilds relevant production archives
and actual Meson tests, runs the same small sequential matrix, the correct
MPI-aware MMG/native2D regression and the default-path checks, and audits output.
It uses the current configured `integration_evidence/build-integrated-v2` build and
local `/tmp/native-build-tools/ninja`; a configured build/tool path is required.
The component tests/audits are qualified above; the final combined orchestration
has syntax checks but has not been launched as another redundant full campaign.
Keep binaries/objects out of Git; compact source pins, logs, failures, meshes and
state evidence are retained. Prepare cluster jobs after full phase/memory controls;
do not request cluster launches overnight.
