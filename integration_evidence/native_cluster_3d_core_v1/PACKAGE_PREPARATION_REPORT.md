# Initial native3D SGE package preparation — 2026-10-11

Package implementation/evidence checkpoint: `b8add069d4009706fa297a48125bcb52b65522d8`, pushed to origin/codex/native-3d-core; full remote SHA verified. This qualifies orchestration only.

Repository NativeIntegrated, branch codex/native-3d-core, predecessor
b441f55b412ff30dbd997890746c6aff62480bdf. FullGoal1 remains ACTIVE.

The closed SGE package source is prepared and its orchestration checks pass.
No native3D C++ build, new mesh run or real qsub was performed. The pending
reference-admission draft remains unvalidated;1fafe9e83c is the last validated
production checkpoint. The actual configured Ninja dry-run finds19 pending
steps and preparation refuses before creating a bundle or changing the binary.

prepare.py gates an up-to-date complete68-object normal driver, required
MPI/MMG/CGNS/tests, pins sources/objects/archives/thin members/build options,
checks preparation drift and copies the executable, native3D implementation/
fixture recipe and existing independent auditors into regular closed files.
Frozen bundles reject missing/changed files, extra unpinned files and symlinks.
Compute nodes require no Git or compiler. System dynamic libraries remain
cluster-platform dependencies, not files copied into the bundle.

Seven serialized SGE jobs are planned: four-slot correctness (serialMMG,
native/reference/2D+3Dbridge MPI1/2/4, SU2N1M1/CGNSN4M2 persisted restarts),
actual metric/Hessian lifecycle at N1/2/4, then n4 manufactured wedge timing
at N1/2/4. Subsequent jobs gate the same-package correctness result; timing
also gates the same-rank lifecycle result. Partial qsub acknowledgements,
job IDs and attempted commands are persisted before later submissions.
No scheduler job has been submitted.

Timing runs N=M and M=max(1,N/2), one warm-up and three paired repetitions with
alternating detailed-profile OFF/ON order.13 raw mesh/source/state/readback
hashes must match within each fixed N/M. The reviewer reports sample spread,
medians, accepted-work rates, max/mean, admitted bound and lifetimeRSS.
Exclusive and nested phase rows are labelled; max rank parent wall is elapsed.
Different N/M trajectories are independently audited, not called fixed-work
speedups. Manual metric envelopes are separate from actual Hessian lifecycle.

Every run logs its full working/result folder. Failures preserve raw files,
stage exit/timeout records and failure reason. Passing large JSON evidence is
gzip compressed only after audit and verified exact decompression; restoration
checks hash/length and refuses modified raw output. Raw logs, SU2/CGNS meshes,
original sidecars and BDF state files remain directly inspectable. Results
contain code/recipes/auditors/pins, not executables/objects/build trees.
Download ClusterResults/native3d_* and matching launcher/native3d_* only.
README.md documents preparation, submission, review, restore and ParaView export.

package_checks_v1/v2/v3 retain their respective source pins/logs. Final v3
passes9 fake-file/synthetic-timing/short-owned-process/mocked-qsub tests:
closed bundle/drift, exact compression/restore/no-overwrite, existing compressed
output preservation, failed/timeout process logs, serialized hold/result-gate
chain, stale build refusal, disabled-required-option refusal, timing medians/
artifact/accepted-work corruption and partial submission receipt preservation.
Shell and Python syntax pass. No fake executable or real scheduler was invoked.
preparation_refusal_v1 verifies the real stale configured build is rejected,
with unchanged test-driver hash and no output bundle.

Local numerical work remains deferred: foreign FSI kept CPU saturated after
a foreign nice19/j2 build ended; load was~19 then~10, sampled idle4–7%.
The root filesystem temporarily refused a shell here-document; this command
made no changes, and retrying without shell temporary allocation succeeded.
Workspace had~77GiB free while root space fluctuated around1GiB. Only small
owned /tmp scratch was found; foreign directories/data/jobs were untouched.
No large cleanup would have been justified from that evidence.

Next: recheck real host resources, finish the pending reference-admission
configured regression/fixed-work/actual-restart matrix, qualify and publish it.
Then build/freeze the cluster driver on its login host and launch only when
the user requests. Complete cache-miss exports, traffic/privateCPU/wait/memory
calibration and larger size/rank first-limit cases still remain. This initial
MPI1/2/4 package and its orchestration tests are not Goal1 completion, full3DBL,
curved geometry, full unsteady3D/PDE or affordability evidence.
