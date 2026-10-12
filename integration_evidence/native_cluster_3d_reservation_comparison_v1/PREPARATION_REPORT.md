# Reservation comparison tooling preparation checkpoint

2026-10-12. Repository `/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated`;
branch `codex/native-3d-transport-accounting`, starting tip
`6b319ea51989c2836429e50f580bc8c87b940cc0`. Goal1 remains ACTIVE.

**Reusable SGE comparison tooling is prepared and locally checked; no cluster
jobs, MPI adaptation or local performance campaign ran in this checkpoint.**
The actual isolated reservation algorithm's previous full97-case MPI1/2/4,
stronger aspect/H20, CGNS restart and six lifecycle evidence remain separately
pinned. Production CPP, normal executable/objects/archives, AdapNoExt and other
workers are unchanged. The existing donor fixed-work tools are unchanged.

## Comparison and scope

The scheduler prototype changes accepted meshes and subsequent frozen P1
triangulations. The new independent campaign under
`integration_evidence/native_cluster_3d_reservation_comparison_v1` therefore
certifies one full reference per scheduler/N/M, requiring exact repeatability
within that group and the same initial geometry/frozen target across schedulers.
Later requested analytic sensor recipes and strict q/J/L contracts match, but
later P1 fields are trajectory-specific. Changed work across schedulers is
reported, rather than mislabeled fixed-work or rejected as inherently invalid.

It reuses the existing closed-bundle identity checker, core preparation,
complete actual regression/auditors, rank/phase profiles, paired schedule and
SGE conventions. Serialized correctness then N1/N2/N4 jobs exercise full/half
worker allocations, warm-ups and three paired detailOFF/ON repetitions.
Optional MPI API profiling retains separate overhead controls and limitations.
The parent wall includes metric/observer/remesh/partition/migration/return/
rebuild/transfer; diagnostic observation is labelled and physical-CFD affordability
is not claimed. Detail overhead must retain the same work. Report total elapsed,
per-stage/rank costs, useful accepted work/sec, state/metric bounds, final sizes,
proposal veto/accept/reject distributions and variability together.

Fresh exact audits enforce q>=.20,J>=.05,L<=1.8 with finite values before repeat
hash reuse. Failed stages retain complete artifacts; successful identical
repeats deduplicate only after output and accounting checks. Download compact
ClusterResults with one compressed source/mesh/current+BDF reference per group,
logs/configs/phase/rank/source/binary/audit pins and failures. No ClusterRaw,
binaries or build tree is needed for analysis. Wrapper traffic and lifetime RSS
remain distinct from physical wire traffic, pure waiting and allocator limits.
The existing size8 diagnostic1024/default512distinction remains explicit.

## Actual preparer smoke and failure

`preparation_check_v2` exercised the real current configured build:68actual
objects, unchanged source/options/archives/members and the standard binary.
It froze two regular-file closed packages, compiled only the qualified private
directory, linked it before the normal archives, and verified exclusion of the
normal directory member. The variant differs from the prior qualified source
only in its include path. Both closed bundle identities were verified.
Normal binary SHA256 `9811747b59cbb64dc91a95a53d8f4bb06d281249d95e2f608705a5681b8076ec`;
isolated preparation binary SHA256 `eae0e020259b103a9c5460be582241ba2a6715b4077e454ff7d70b612b693e45`.
No new MPI/numerical certificate is inferred from compilation or preparation.

The first attempt stopped before compilation due to a missing relative Ninja
executable. It also exposed a wrapper bug: a caller-relative `./ninja` was passed
to the child after changing working directory. The wrapper now resolves and
validates Ninja before creating a bundle. `preparation_check_v1` retains raw
stderr/commands/failure, and an explicitly labelled reconstruction of the first
wrapper source reversing the sole correction. The second attempt used the real
pinned `/tmp/native-build-tools/ninja` successfully; this tool is a local smoke
requirement, not a runtime input or cluster command. No files are needed from/tmp
for the eventual cluster package or job.

Actual closed bundles were verified **before** removing the two terminal local
binary copies, freeing1595519616bytes. The extra temporary object had already
been removed after successful link with its hash retained. Production binary
remains. `PREPARATION_SMOKE_VALIDATION.json` pins all retained artifacts and
the removed identities. The retained `preparation_check_v2` is historical smoke
evidence, **not a runnable prepared cluster bundle** after disposal. Prepare a
fresh closed `prepared_v1` from the actual cluster build as documented in README.
Build/copy logs, source, complete linker map and original manifests remain.

## Lightweight checks and remaining proof

`selfcheck_v1/validation.json` records16passing synthetic-record orchestration
checks and Bash SGE syntax. Coverage includes paired full/half-rank schedule,
independent changed-trajectory references, same-scheduler drift and initial-target
mismatch rejection, strict/nonfinite metric limits, preserved failures, altered
files/extra files/symlinks, mocked serialized qsub/gates/environment, changed
accepted work analysis, exact raw-runtime verification and detail/mpiP controls,
complete68-object recipe parsing and early missing-Ninja rejection. They are
not numerical meshes, actual submissions or performance measurements.

The exact login preparation, submission, download and analysis commands are in
README.md. No user run is requested overnight and no scheduler was contacted.
Still open: actual repeated cluster operational cost/scaling/overhead, rank-cost
weight calibration, traffic/wait distinction, practical memory envelope and the
large-case production512completion limit. No production scheduler promotion,
Goal1 completion, Goal2 activation, curved geometry,3DBL,physicalunsteady or
universal affordability claim follows.
