# Cluster reconstruction-reuse review: 582331, 582333 and 582334

**Correctness PASS; frozen paired comparison FAIL.** The two performance jobs
stopped at their first baseline/candidate Euler-to-BL pair because adapted
coordinates and transported tensors were not byte-identical. The strict gate
behaved as intended. It has not been relaxed. Neither job reached BL-to-Euler
or the weighted M4/M3/M2 variants; the remaining14 cases per job are untested.

Reviewed implementation: `d0dc9822fc6c3c1a84fcb1ec0907c869822d3580`, branch
`codex/native-unsteady-performance`. Original downloaded output is unchanged.
Review source folders:

- `/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated/ClusterResults/predict_correctness_582331`
- `/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated/ClusterResults/reconstruction_reuse_582333`
- `/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated/ClusterResults/reconstruction_reuse_582334`

`review.py` freshly verifies1247 unique source/suite/tool/data/log hashes,
source/binary identities recorded by the jobs, manifests, saved numerical audit
reports, profile CSV/log timing/counter closure and pair differences. It reads
saved output only: no local SU2/MPI/build, solver timing experiment or numerical
mesh-audit rerun. Source pins establish identity, not independent compilation
provenance. Binaries were not downloaded; identity is checked through matching
job/preparation receipts and the previously validated control hash.

## What passed

Correctness582331 completes all20 stages: native/metric MPI1/2/4 suites, five
expected rejection guards, six Euler/CGNS/straight-wall SA RANS cases and three
partial-window restarts. Native suites report111 cases per rank; focused reuse
jobs pass61 cases per rank at MPI1/2/4. Negative guard exit1 is expected and is
not a failed correctness stage. Across11 actual replacements: minimum quality
0.5143406279, maximum Simpson length1.7950970958, maximum two-history relative
integral defect3.9790689697e-15 and RANS relative height error4.4408920985e-16.
Positive density/pressure and partial-restart SPD/snapshot checks pass. This is
small-case lifecycle evidence, not converged RAE aerodynamics/native3D or a
composed sensor-plus-BL gradation certificate. The known RANS terminal composed
nodal diagnostic remains ratio2.73259 with76 residual directed edges; sensor
gradation reports zero residuals. This predates reuse and remains unresolved.

All four frozen RAE executions (two pairs) return COMPLETE and pass their
independent geometry/topology/marker/reference/height/transported-metric audits.
Both versions produce12235 points and24078 triangles, minimum quality
0.1896087582, maximum Simpson length1.7999995295, maximum relative wall-height
error3.8129499558e-12, and transported directional tensor defect
7.0457733052e-11 (tolerance1e-7). Every recorded per-rank selected/attempted/
reconstructed/committed/cell/selection-scan count matches. Both logs have45
coordinated commits,27 conflicts and zero memory/dependency-size/stale rejections.
The new scratch admission did not change these particular workloads.

## Why the comparison failed

The two repetitions reproduce EXACTLY the same baseline/candidate file hashes,
41 changed point-coordinate rows and43 points with changed tensor entries.
45 tensor CSV rows differ in total (some coordinate-only changes). All other
mesh records, including connectivity, point IDs and boundary marker records,
are byte-identical. Maximum coordinate displacement is3.1447278462e-15 chord
units, at point7712 near x0.0347,y-0.02346. Maximum tensor-entry difference
normalized by the tensor's largest entry is1.0304235148e-13. This entrywise
quantity is **not** a new directional eigenvalue/Loewner residual; the existing
independent directional audit remains the metric correctness evidence.

Tensor values also differ at four points whose coordinates are identical:
3287,6377,6998,11787, mostly by one last-bit entry change. Thus moving points
alone cannot explain every discrepancy. Full lists and first-pair hotspot
records are retained in `review.json`; the original grids/tensors remain under
`ClusterResults/reconstruction_reuse_JOB_ID/cases/frozen_euler_to_bl_n4_m4_pNO_r1_{control,profile}`.

The source globally appends `-ffast-math -fno-finite-math-only` in meson.build;
the separately compiled orientation/determinant primitives disable fast math
and contraction. Interpolation, metric quality/length, BL composition and point
movement remain partly in inline/ordinary fast-math code. The candidate changes
call/return/storage contexts and cache residency. Reassociation/inlining/rounding
is a plausible explanation, but the data do **not** establish whether score reuse,
second-chance eviction, a cache-hit versus fresh-evaluation inconsistency or
compiler context caused these low-bit differences. Canonical donor tie handling
and authoritative vertex seeds remain intact in the source. No evidence here
shows an invalid reconstruction or changed numerical threshold; bitwise invariance
has nevertheless failed and remains unresolved.

## Recorded performance, with limits

| Job | Baseline remesh(s) | Candidate remesh(s) | Change | Private process-CPU change |
| --- | ---: | ---: | ---: | ---: |
| 582333 | 30.646764 | 29.601347 | -3.41% | -3.09% |
| 582334 | 32.367875 | 29.324879 | -9.40% | -4.78% |

These are two frozen N4/M4 NO pairs on shared node-a-ag1, with up to64 other
visible compute processes and broad per-rank24-CPU masks. The exports do not
record absolute case start/end timestamps or reliably identify another owned
campaign as contention; sequential submission/nonoverlap cannot be certified
from these files. Different timing changes under the same numerical/work counts
are further reason not to report a verified general speedup. No full partition
matrix or fixed-work scaling result exists for this candidate.

Both repetitions give the same count changes:

| Private counter | Baseline | Candidate | Change |
| --- | ---: | ---: | ---: |
| Metric requests | 70107612 | 46835131 | -33.20% |
| Actual evaluations(cache misses) | 15489394 | 15054440 | -2.81% |
| Dynamic evictions | 4463044 | 4028086 | -9.75% |

Most eliminated requests were already cache hits. The evaluated-sample count is
not a distinct-coordinate count or Hessian count. Imbalance still matters:
private max/mean drops from2.290 to2.214 in582333 and2.208 to2.210 in582334;
there is no demonstrated load-balance solution. Rank maxima and nested scopes
must not be added to form a lifecycle cost. Detailed exclusive scopes, including
working/return partition costs already in remesh, are preserved in review.json.

The previously highlighted round31436 (46 cells near the trailing edge) reduces
requests10563384->3337323, evaluations1329104->1295742 and wall2.26185->2.01152s
in582333. This is consistent with a useful but modest improvement: most new-query
evaluations remain. Counts alone do not measure exclusive interpolation/BL time. Coordinated phase7.3590->6.3211s and7.2848->6.2843s.
The per-attempt profiler still combines ordinary split, seed search, joint repair
and final cache resampling; it cannot attribute these counts to one substage.

## Next diagnostic

Preserve both actual executables: baseline
`build-native/reconstruction-control/test_driver` hash
`1a092865535502a3cf42a4f4d16c481c3cdc45dc6feff1a9844afa22c181634f`, and candidate
`build-native/UnitTests/test_driver` hash
`13751201942dfcf9731bc377e45a69b0793ce82f0e2abe0b72edfbcee0902626`.

The useful next step is a SMALL same-executable frozen N4/M4 NO diagnostic,
separately enabling score reuse and second-chance eviction, plus checks that a
cached metric/score equals a fresh evaluation at exactly the same ordered
coordinates. Also compare a rebuilt candidate with both optimizations disabled
against the archived baseline: this distinguishes compiler/build context from
runtime reuse. If arithmetic context is responsible, use one shared evaluation
path for miss/direct/cached checks before considering stricter isolated native
arithmetic. This needs prepared runtime controls and cluster tests; it is a
recommendation, not an implemented or completed diagnostic. The unchanged
byte-identity gate remains, and broad scaling repetitions should wait for its
cause to be understood. No production fix or blanket tolerance relaxation is
made by this evidence-review commit.

Reproduce this lightweight saved-evidence review from the matching implementation
checkout (later intentional source changes require its historical checkout):

```bash
PYTHONDONTWRITEBYTECODE=1 python3 integration_evidence/native_cluster_reconstruction_reuse_v1/cluster_review_582331_582334/review.py /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated
```
