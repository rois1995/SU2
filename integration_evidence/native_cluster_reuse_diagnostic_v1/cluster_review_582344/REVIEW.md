# Frozen reuse diagnostic 582344

**All seven numerical audits pass. All five runs of the new executable are
byte-identical in the accepted mesh and all four transported-tensor CSVs.** This
includes OFF, SCORES, METRIC, BOTH and the fresh-hit AUDIT. Operation/selection
counts are identical in every comparison, including the historical control.

MPI1/2/4 focused native gates pass63 tests per rank. The sampled audit checks
540,894 dynamic metric hits and424 scalar/current-star scores, with **zero bit
mismatches**. Audit extra work changes sampling/eviction counters slightly but
leaves the final outputs identical. These checks are bounded, not exhaustive.

| Run | Full remesh seconds | Private CPU seconds | Metric requests | Evaluations |
| --- | ---: | ---: | ---: | ---: |
| Historical control | 28.6165 | 23.7142 | 70,107,612 | 15,489,394 |
| Archived reuse candidate | 27.5995 | 22.7306 | 46,835,131 | 15,054,440 |
| New OFF | 28.9314 | 24.0507 | 70,107,612 | 15,489,381 |
| New SCORES | 27.5769 | 22.7076 | 46,835,131 | 15,249,419 |
| New METRIC | 28.8401 | 23.9641 | 70,107,612 | 15,448,810 |
| New BOTH | 27.5512 | 22.7048 | 46,835,131 | 15,054,440 |
| New AUDIT (extra work) | 27.8691 | 23.3466 | 46,847,608 | 15,054,381 |

The historical control reproduces its original mesh hash380c6efa8e98…; the
archived candidate and all new modes reproduce fcdde637d5cc…. Therefore the
historical difference remains when reuse is disabled in the new binary. Runtime
score reuse and second-chance eviction do not explain the differing final values
on this workload. This strongly points to arithmetic/code-generation context in
the source/build change; it does not isolate a particular compiler transformation
or prove the behavior on every other case.

Historical differences are unchanged:41 coordinate rows, maximum displacement
3.1447278462035766e-15 chord units;43 changed tensors, maximum entry delta divided
by largest tensor entry1.0304235148384509e-13. Four tensors differ at unchanged
coordinates:3287,6377,6998,11787. Connectivity, marker records and point IDs are
identical. These differences have no demonstrated practical consequence here.
The entry ratio is not a directional tensor error norm.

Each accepted mesh has12,235 points and24,078 triangles. All reports retain
qmin0.18960875818070652, maximum metric edge1.799999529453445,
maximum relative first-height error3.812949955772638e-12 and maximum transported
metric directional defect7.04577330522297e-11 (tolerance1e-7). Geometry/reference
and topology audits pass. No sensor, geometric BL, fade or acceptance rule has
been changed to obtain these results.

Same-executable BOTH vs OFF observes4.77% lower remesh time and5.60% lower private
CPU time. Score reuse accounts for most of this observation; second-chance-only
remesh reduction is0.32%. Requests fall33.20%, evaluated samples2.81%, evictions
9.75%; most saved requests were hits. These are one ordered, frozen workload on
four ranks, not general speedup/scaling results. Sampled other compute processes
are zero in this job, and absolute UTC times verify our cases ran sequentially;
this does not establish exclusive hardware. Private max/mean remains about2.23
with BOTH (2.33 OFF). Imbalance and expensive new-query work remain.

## Decision

Stop chasing historical byte identity as a practical accuracy issue. Keep its
failure in the record. Preserve numerical geometry/height/quality/transport
contracts, and use **same-executable OFF vs BOTH** for strict operator-invariance
checks. This does not widen any numerical tolerance. The original historical
comparison gate is unchanged and still fails, as expected.

Next finish frozen Euler-to-BL and BL-to-Euler at N4/M4 NO, M4 YES, M3 YES and
M2 YES using that strict same-executable reference. Larger core counts, unsteady
CFD lifecycle checks at the new diagnostic source, converged viscous quantities
and combined-field gradation remain separate validations. No speculative
arithmetic fix, blanket strict-math CFD flags, exhaustive trace or further
roundoff-only campaign is justified by this evidence.

## Evidence and reproducibility

Working checkout: /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated.
Downloaded cases: ClusterResults/reuse_diagnostic_582344/cases/.
Source revision0bb829910ad646496e9e87ab9a5983487e69efc7; new executable SHA256
5827ffbe8b7ac9dd0336d000a33a8d649975579f1cd57f623a440a00fb87019e.

review.py verifies1,045 unique source/tool/data/log hashes, source/binary receipt
identities, compact manifests, saved numerical reports, bounded profiles,
operation/timing closure and exact output comparisons. review.json retains those
hashes, both historical differences and all case summaries. Original downloads
are unchanged. Executables were not downloaded; receipt hashes are not independent
compilation provenance. Saved numerical audits are checked, not rerun locally.

```bash
PYTHONDONTWRITEBYTECODE=1 python3 integration_evidence/native_cluster_reuse_diagnostic_v1/cluster_review_582344/review.py
```

The reviewer uses the recorded source revision from Git for pinned files changed
by later commits. This historical-source lookup runs only on the local review
host, never a compute node. No local C++ build, SU2, MPI, performance run or heavy
numerical audit was performed.
