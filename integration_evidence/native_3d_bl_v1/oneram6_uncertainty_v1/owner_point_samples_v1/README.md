# Canonical source-owner point samples

Base5ebed3f3b81244c54fd89b6814d70a8f12a64987, branch codex/native-3d-bl.
Full Goal3 remains ACTIVE/incomplete. The earlier whole-donor caches stay withdrawn.

## Connected change

Partition weights and mesh return now use the existing immutable source-owner ADT
instead of importing whole donor stars and constructing another point index per
batch. Owners search exact physical P1 donors first, then requester-eligible
extensions, in canonical node-key order. The requester selects the global winner
and receives sensor-only data, ordered original node IDs/native weights and the
actual publisher. Missing points, invalid controls and storage/protocol failures
reject collectively without partial answers. Cavity/edge donor caps are unchanged.

Local/owner interpolation share identical strict InterpolateSensor arithmetic.
Barycentric metadata retains full binary precision using integer mantissa chunks
and an exponent through the existing fixed integer/double codec. Interpolation is
performed before transport; BL targets are never sent. Caller point queries use
one bounded2048-entry sensor-only memo per immutable source/policy call. It replaces
the former per-batch FrozenField cache/index, rather than adding an overlapping
index. Batch answers stay pinned during FIFO eviction. Composition runs at EVERY
actual query, including hits, with the same geometry, frozen core, finer demands
and fade. No geometry/height/positivity/q/J/full-edge/transfer acceptance changed.

Requester extension declarations share the fixed query transport. Source/index,
peer buffers, memo and current-batch scratch enter native storage admission; those
models exclude CFD/allocator/RSS. Profiles now report source point queries/payloads,
lookup time and owner search/interpolation separately from composition and index
build counts. Owner search is local work; lookup time also includes collectives.
Native byte/call counters retain their documented World-wrapper scope.

## Saved original M6 results

Same512 canonical original tetrahedra on1/2/4 ranks; all2560 actual query coordinate,
sensor and composed tensors and integer weights exactly match fresh evaluation.
The same archived serial cold-reference weights match across ranks. Original
hosting-rank RAW9 integration fields remain untouched; this replay uses the separately
archived owner-UPPER6 backend source and original reference/core/fade. No CFD solve,
new normalization, construction or solution transfer is attempted for M6.

| Ranks/cells | Source lookups | Memo hits | New / cold indexes | New / cold rounds | New / cold preparation |
|---|---:|---:|---:|---:|---:|
| 1 /512 | 966 | 1594 | 0 /124 | 64 /125 | .03337 / .12975s |
| 2 /512 | 966 | 1594 | 0 /124 | 64 /125 | .03086 / .11124s |
| 4 /512 | 969 | 1591 | 0 /123 | 49 /95 | .02960 / .08883s |
| 4 /4096 | 8098 | 12382 | 0 /1852 | 304 /1119 | .26134 /2.28299s |

Times and rounds are maximum rank; index/hit/lookups are sums. This is a3.00–3.89x
selected512-cell preparation gain and8.74x on4096 cells. The4096 replay compares
ALL20480 query answers exactly. Successful misses5151/2947 on the two active
requesters require at least4002 total FIFO evictions at capacity2048; the source
and count-based argument are recorded in its summary. No value changes occur.
The original source-owner index is still built once before these measured queries.

These are contended local selected-cell observations, NOT complete M6 preparation,
full adaptation or scaling. Cold reference reproduces historical fresh8/halve
imports but omits production admission bookkeeping. Count reductions and removal
of the second index are direct evidence; timing ratios are not general guarantees.
One cannot extrapolate these times to first-row construction/edge integration.
The earlier real initializer415.97->16.47s remains separately qualified at83fb;
this change neither improves its uncertainty nor makes budget100000 attainable.

## Correctness and complete scoped runtime

18 serial cases/2941 assertions pass canonical sampling, exact source ties,
physical/declared-extension priority, shifted storage and actual source publishers,
missing/nonfinite points, asymmetric memory refusal, full-precision weight transport,
existing bridge/current+BDF/conservative/barycentric transfer, declared exterior
coverage and composed-edge controls. A288-donor genuine star still rejects ordinary
full import at the unchanged256 cap, while point-only sampling returns its canonical
answer. MPI4 canonical/bridge controls and affected curved-runtime return pass.

Initialized RANS/SA N4/M2 construction/removal passes the existing independent
geometry/full composed metric/height/flow+BDF/transport/SA/restart/SU2+CGNS audit:
24->6 accepted cells, exact volume1, h0 error0, q minima.70846/.68723, J minima
.45142/.40825, max full-target edge1.71991/1.73205 and transported sensor error0.
Flow+BDF integral defects<=2.75e-16. This is a scoped initialized cube workflow,
not evolved viscous accuracy or accepted M6 wing construction. Its composed audit
still reports44 violations/max24.5184; sensor gradation is not a combined certificate.

## Reproducibility and resources

campaign.py is resumable with source/build/binary/config/input pins; it reuses
passing serial controls/pilot before the MPI/runtime checks. larger_replay.py uses
the same binary and field for4096 cells; larger_resume.log verifies the saved native
result rather than rerunning it. comparison.json, per-rank query counts/weights,
independent_mpi4_m2.json, raw logs and resource receipts retain exact scope.
The initial compile rejection for raw long-double wire fields and its source patch
are preserved; the house codec was not broadened or metadata silently narrowed.

Build-j1/nice19/thread1, one owned heavy job, local MPI<=4. Host38–39GiB available,
foreign CFD/ClusterResults/dependencies untouched. Successful build7m48s/peak process
2.39GiB; serial pilot36.02s/1.63GiB, remaining connected wrapper88.00s/885.92MiB,
larger wrapper29.63s/529.50MiB. Process peaks are NOT aggregate MPI job memory.
GNUtime reports0 swaps; host swap has no established attribution. Compile scratch
stayed on4TB. No cluster submission, CFD convergence rerun or unchanged rejected
M6 construction campaign. All owned native jobs are terminal.

From repository root after building both binaries:

```sh
OMP_NUM_THREADS=1 OPENBLAS_NUM_THREADS=1 MKL_NUM_THREADS=1 nice -n19 python3 integration_evidence/native_3d_bl_v1/oneram6_uncertainty_v1/owner_point_samples_v1/campaign.py
OMP_NUM_THREADS=1 OPENBLAS_NUM_THREADS=1 MKL_NUM_THREADS=1 nice -n19 python3 integration_evidence/native_3d_bl_v1/oneram6_uncertainty_v1/owner_point_samples_v1/larger_replay.py
```

Changed native pins require a new sibling case folder. Links close inside versioned
repository inputs. Do not overwrite old field/evidence or reinterpret selected-cell
results as a full adaptation benchmark.

## Remaining work

Full original-field uncertainty calibration/allocation and finer-mesh reassessment/
bounded correction epochs; accepted difficult curved/M6 construction under unchanged
mesh constraints; complete lifecycle affordability and viscous accuracy. Source point
sampling does not retain arbitrary reconstruction campaigns, cache composed edge
integrals or batch independent mesh publications. The saved first-row transactions/
edge integrals remain a large separate constructor cost. Keep the full goal intact.
