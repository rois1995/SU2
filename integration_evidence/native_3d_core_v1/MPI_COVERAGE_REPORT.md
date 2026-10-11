# Native3D MPI coverage and lifecycle check — 2026-10-11

Repository: /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated.
Branch: codex/native-3d-transport-accounting; predecessor
7398f47f076e19c7ff80f57f4044a3ea4a14bb06.
Evidence/tools checkpoint a2f8b2f1c7c13b9b45b3221d3ee6bb87b364b6ea pushed to origin/codex/native-3d-transport-accounting; remote SHA verified. Production remains475820f366. Core remote remains6b3fefd039bfc3d1937b8add7d21d0ea41344bc9.

No production C++, test fixture, target, guard or run-package change.
codex/native-3d-core remains at6b3fefd039bfc3d1937b8add7d21d0ea41344bc9.

## Proven counting gap

audit_mpi_coverage.py verifies the original source/build/log/receipt identities
of40 actual serialMPI1 engine records. Build configuration contains HAVE_MPI.
The source proves each counted World collective wrapper invokes at least one
MPI collective, and each directory-election API call invokes at least one
severity Allreduce. These are disjoint counter scopes:

    minimum collective API calls = wrapper calls + directory elections

The40 logged engine invocations contain16573 wrapper calls and9411 directory
elections, establishing at least25984 collective API calls. At least36.22% of
that known collective-call work is missing from the wrapper counter. The true
under-count is larger when hidden round collectives and other elections are
included. This is a lower bound on API counts, not unmeasured elapsed time or
physical network traffic; election timers are already separate nested records.

For example, restoreC_edge4_unit_m1 accepted one edit with926 wrapper calls
and582 directory elections: at least1508 collectives in its engine invocation.
coplanar_m1_s2 accepted14 edits with a1793-call lower bound. These are bounded
manufactured repair tests, not practical CFD workload coefficients.

The current success path in ElectFailure already returns after its first
severity reduction. A blanket optimization of that shared function is therefore
not justified by this source audit. Keep its deterministic severity/gid/root/
message election and failure safety intact.

Three lightweight controls pass: integer/nonnegative/finite/rank/envelope checks,
zero-work handling, incomplete current schemas and missing records. Actual-source
coverage evidence: mpi_coverage_audit_v1/validation.json. Original failed collector,
its checked recovery, all meshes and original tests remain unchanged.

## Remaining communication paths

| Path | Present coverage | Missing measurement |
|---|---|---|
| World exchange | Self-inclusive packed bytes; outer count/payload wrapper calls and elapsed | Individual round/planning MPI calls, exact off-rank payload versus self/count/reduction traffic |
| Directory votes | API-call count and enclosing elapsed | Individual MPI calls and their CPU/blocking behavior; other World.Fail/engine/donor votes |
| Point-directory import/lookup | Exclusive import/query scopes | Direct passive point transport/reductions/request completion |
| Partition | Closed graph/ParMETIS/migration elapsed scopes and logical migration work | MPI internal to vendor partitioner; vendor/allocator peak workspace |
| Worker subset | Communicator and completion phases; inactive completion CPU | Comm split/free, Ibarrier/Test or fallback Barrier details and sleep versus MPI progress |
| Geometry/solver rebuild and transfer | Closed lifecycle wall/CPU phases and state audits | Direct/shared/vendor MPI below those scopes |
| Diagnostic output | Explicit report costs; nested emission included by parents | Its collectives must be separated from computation in deep tracing |

Do not add nested election/wrapper timers to exclusive phases or sum rank maxima.
A wrapper's elapsed includes packing/progress and possibly waiting. MPI completion
elapsed is not pure waiting either. No elapsed-minus-process-CPU inference is
valid. M<N completion intentionally uses a1ms sleep between incomplete tests.

The next useful measurement is a calibrated MPI API trace/profile, correlated
with existing phase windows and parent/worker communicators. Cover blocking
collectives, nonblocking issue/completion, communicator calls and vendor calls;
distinguish API elapsed/CPU from rank arrival imbalance and requested logical
payload from wire bytes. Include an instrumentation-disabled fixed-work control
and preserve ordinary correctness/audit checks. The current suite remains the
numerical gate; profiling must not replace it or modify a running frozen package.
A full trace tool has not been implemented or calibrated here.

## Actual one-rank lifecycle correctness

transport_lifecycle_mpi1_v1 passes462 assertions in the existing lifecycle case.
Six fresh manufactured boxes exercise the fixture's OFF/ON controls and actual
metric/Hessian construction, weighted remesh, geometry/solver replacement,
affine current+BDF2 transfer and requested SU2 mesh/reference output. This is
not a CFD time-integration run. Only one owned MPI rank ran, nice19/thread1,
after its fresh25%-idle/load<8 gate; audits ran afterward, not concurrently.

Independent lifecycle audit passes6 strict frames,21 closed wall/CPU stage
records and21 nested records,3 phase-corruption and5 field-counter controls.
The new import/parent/worker totals and all relevant bridge/engine detail
schemas pass independent closure checks. rank_profiles.json preserves critical
rank/phase distributions and explicit wrapper coverage for these actual records.

These built-in repetitions validate fixed-work profiling interfaces; the host
is contended and no local instrumentation-overhead/scaling/affordability conclusion
is drawn. N1 partitioning does not validate multi-rank ParMETIS behavior. No MPI2/4,
M<N, n4 envelope, persisted-restart, n8 limit or repeated cluster qualification
was completed here. Actual archive/object/source/binary pins are reverified,
and no production source has changed since475820f366.

Goal1 remains ACTIVE. Before merging into core, complete its frozen MPI and
independent geometry/metric/state/output/restart gates. Continue to actual repeated
cluster costs and practical limits, and then calibrate full traffic/wait/memory.
No curved geometry, robust3DBL, full unsteady3D or affordability claim.

Publication whitespace checks: the initial check flagged six generated solver CSV headers. Code/docs and remaining artifacts pass the scoped check; preserve those raw CSV headers and the raw Catch stdout extra EOF newline unchanged. No original artifact was normalized or overwritten.
