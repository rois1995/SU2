# Frozen reconstruction reuse diagnostic (SGE)

The downloaded jobs582333/582334 produced meshes that pass the numerical audits,
but fail exact byte identity. Coordinate differences reach3.145e-15 chord units;
four tensors differ even at unchanged coordinates. Roundoff/compiler arithmetic
is plausible, not established. This package isolates the two reuse improvements
without changing any target, geometry/height/metric acceptance threshold or the
ordinary strict comparison gate. It is a diagnostic, not a performance campaign.

Use **codex/native-unsteady-performance**, from the complete cluster checkout root.
No Git is required on compute nodes. Keep both previously tested executables and
use the same MPI/GCC10.2/CGNS/build settings. Do not rebuild or edit queued/running
checkouts. No local solver run or job submission by Codex is required.

## Preserve before rebuilding

On the login/build host:

```bash
git checkout codex/native-unsteady-performance
git pull --ff-only origin codex/native-unsteady-performance
python3 integration_evidence/native_cluster_reuse_diagnostic_v1/prepare.py --preserve-previous
./ninja -C build-native -j2 UnitTests/test_driver
python3 integration_evidence/native_cluster_reuse_diagnostic_v1/prepare.py
qsub integration_evidence/native_cluster_reuse_diagnostic_v1/RunReuseDiagnosticSGE.sh
```

Keep the build log. The diagnostic needs only test_driver; no CFD timesteps or
new SU2_CFD executable is used. Preservation requires the validated control
already at build-native/reconstruction-control/test_driver, SHA256
1a092865535502a3cf42a4f4d16c481c3cdc45dc6feff1a9844afa22c181634f. It also saves
the job582333/582334 candidate, SHA256
13751201942dfcf9731bc377e45a69b0793ce82f0e2abe0b72edfbcee0902626, as
build-native/reconstruction-previous/test_driver. Neither is overwritten. If
these binaries were replaced, recover the tested originals; recompiling a
historical revision does not establish their identity. All previous checkpoints
and result folders remain intact.

The new checkpoint is build-native/reuse_diagnostic_checkpoint.json. Preparation
refuses overwrites and pins sources, tests, tools, fixtures, build options and all
three binaries. PREPARED_NOT_VALIDATED certifies identities, not a successful
build or numerical correctness. Launch dependencies resolve inside the checkout.

## What runs

One ordinary shared aero-ags.q allocation uses four slots. All stages are
sequential: focused native units on MPI1/2/4, then seven frozen Euler-to-BL remeshes
with N4/M4 and no working repartition. This is the failing RAE workload, with the
same frozen raw sensor and actual-query geometric BL composition.

| Role | Executable | Score reuse | Second-chance metric retention | Fresh-hit audit |
| --- | --- | --- | --- | --- |
| control | validated job582199 | original | original FIFO | no |
| archived | tested job582333/582334 | enabled | enabled | no |
| OFF | new diagnostic | disabled | FIFO | no |
| SCORES | same new executable | enabled | FIFO | no |
| METRIC | same new executable | disabled | enabled | no |
| BOTH | same new executable | enabled | enabled | no |
| AUDIT | same new executable | enabled | enabled | yes |

OFF disables scalar score reuse and unchanged-star reuse; it retains the bounded
metric cache with FIFO, as before these two improvements. It is not a recompilation
of the historical binary: compare control vs OFF to detect rebuild/code-context
differences. Compare archived vs BOTH to detect instrumentation/build-context
differences, and OFF vs SCORES/METRIC/BOTH to isolate runtime policies. Existing
scratch reservations stay in non-audit modes, keeping their memory admission the
same; OFF still allocates the bounded score array. This is not a pristine timing
comparison against the pre-change code.

The environment-only switches SU2_NATIVE_REUSE=BOTH/OFF/SCORES/METRIC and
SU2_NATIVE_REUSE_AUDIT=YES/NO are collectively checked across CFD ranks. Unset
means BOTH and NO. They add no config options. Every actual working and retained
folder is printed.

AUDIT samples eight dynamic metric hits per imported patch, eight scalar hits
per score cache and eight current-star checks per joint configuration. Fresh
metric queries use the same ordered original sensor donors and geometric BL
composition, with an empty private cache. Authoritative vertex seeds are
excluded: their preserved values can intentionally differ from fresh rounded
projections. Scalar and complete current-star scores are recomputed without
score reuse. Additional bounded audit scratch is admitted before donor payload.
The original cached values are still returned; checks do not replace them.

Logs report collective check/mismatch counts, maximum relative entry/score
differences, and the first mismatch coordinates/values on each worker (kind1
metric,2 length,3 quality,4 star). The tensor entry ratio is not a directional
metric norm. Audit sampling is not an exhaustive proof; no observed mismatch
cannot establish that every hit is consistent. Extra queries can change cache
residency and low-bit results, so BOTH vs AUDIT is also reported. Audit timings
and diagnostic extra-work counters are not performance measurements.

## Interpretation and compact downloads

Every remesh must pass the existing independent topology, reference, height,
shape, length, transported-tensor and timing-accounting audits. Numerical failure
still stops the job. Identity and operation-count differences are retained as
diagnostic findings rather than aborting before all modes can be examined.
DIAGNOSTIC_COMPLETE means all seven numerical audits completed; it does **not**
mean byte-identity PASS, zero cache mismatches, converged aerodynamics or measured
speedup. The ordinary reconstruction comparison remains strict and unchanged.

Download only:

```text
ClusterResults/reuse_diagnostic_JOB_ID/
ClusterResults/jobs/JOB_ID_reuse_diagnostic_launcher/
```

Adapted grids and tensors are under cases/frozen_euler_to_bl_n4_m4_pNO_r1_ROLE/.
validation.json retains pairwise hashes/operation comparisons, fresh-hit checks,
full remesh costs and existing bounded profiles. Absolute UTC timestamps accompany
case runs. No flow-output series or ClusterRaw is produced. Selected configs,
logs, meshes, tensors, audit tools and required frozen inputs are hash-verified;
repeated data is hardlinked within the job when possible, otherwise copied.
New temporary work is deleted only after verified export. Old evidence is kept.

If a reuse policy introduces inconsistent cached/fresh values, fix that path and
rerun the ordinary strict gate. If arithmetic context explains the difference,
review a common/strict evaluation path before changing any reproducibility
policy. Do not weaken the numerical contracts or declare this a fix now. After
resolving identity, finish the original Euler-to-BL/BL-to-Euler and working-rank
matrix; those later cases were never reached in the failed comparisons.

Local package checks use fake files only:

```bash
PYTHONDONTWRITEBYTECODE=1 python3 integration_evidence/native_cluster_reuse_diagnostic_v1/check_package.py
```
