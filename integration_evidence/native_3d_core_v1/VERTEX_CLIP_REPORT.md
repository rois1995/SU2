# Exact donor-vertex clipping checkpoint — 2026-10-11

Published implementation/evidence checkpoint: `9dc21db012be53a1e723b3f01e7649c7cad2b5ab`, origin/codex/native-3d-core, full remote SHA verified.

Branch codex/native-3d-core, baseline7bc3869babb19171fe402fdfb0e96f745b769703. The larger wedge profile identifies exact segment clipping as approximately99% of metric-edge evaluation work. CNativePredicates3D.cpp now recognizes an endpoint exactly equal in binary64 coordinates to a donor vertex and reuses the already validated exact donor volume or zero for its barycentric numerators. Other endpoints retain exact determinants. Positive donor validation, interval roots/order/coverage, overlap/gap checks and conformal ties remain unchanged. No tolerance, target, caching, geometry cap,2D or MMG change.

VERTEX_CLIP_MATRIX.json froze MPI1/M1,MPI2/M2,MPI4/M2,MPI4/M4 against their larger_wedge baselines. Independent exact geometry/q/J/L/original-P1/current+BDF/SU2CGNS audits pass all four. All52 raw geometry/source/state/nodal-readback/frozen-readback files are byte-identical; accepted operation counts and rank-local trajectories also match. Provenance qualification finds exactly one changed production thin-archive member, adaptation_CNativePredicates3D.cpp.o; the six runtime test objects are unchanged. The only other source change is a targeted unit test, exercised by the full configured regression.

The new test covers12 directed tetrahedron vertex edges at powers-20,0,300, a nextafter endpoint that must not be snapped to a vertex, and a negatively oriented donor after a valid donor that must still be rejected. Existing tests cover gaps, overlaps, face/edge ties, reversed chords and arithmetic extremes. vertex_clip_regression_v1 uses the complete68-object actual configured test driver:66 native/reference/2D+3D bridge test cases per MPI1/2/4 rank; actual serial MMG2D/3D round trips48 assertions. All source/object/archive pins remain stable. vertex_clip_syntax_v1 passes configured scalar/DD-macro/AD-macro syntax for the passive predicate TU; this is not an active AD solver link/run or derivative certificate.

The isolated vertex_clip_probe_v1 retains old/candidate source, exact inputs, commands and raw output. It tests all1622 original/accepted chord-union queries over the three serial baseline events, with no over-budget candidate set skipped. Returned donor/interval/width/endpoint-weight values match exactly. Three alternating O2 kernel timing samples per event give:

| Event | Chords | Exact determinant calls:old→new | Median kernel reduction |
|---|---:|---:|---:|
| 0 | 518 | 70206 → 40343 | 31.24% |
| 1 | 538 | 103819 → 62509 | 38.20% |
| 2 | 566 | 35921 → 24200 | 20.72% |

Those percentages concern the isolated kernel, not CFD, indexing/import or complete adaptation. Actual configured integration pilots retain the following event-parent elapsed maxima (seconds):

| Ranks/workers | Baseline events0/1/2 | Candidate events0/1/2 |
|---|---|---|
| 1/1 | 1.594655 / 3.114147 / 1.296751 | 1.280673 / 2.694163 / 1.196627 |
| 2/2 | 1.122300 / 3.457725 / 1.094855 | 0.983534 / 2.582177 / 0.889527 |
| 4/2 | 1.371933 / 3.199143 / 1.196165 | 1.193571 / 2.694954 / 1.082925 |
| 4/4 | 1.836336 / 5.207394 / 3.023744 | 5.147926 / 5.869953 / 2.645046 |

One contended sample per implementation: MPI4/M4 is slower in two events. This is retained, not explained away or used for a speedup claim. Cluster repetitions and measured instrumentation overhead are required for full-cycle conclusions. The phase CSV contains374 closed rows and useful accepted-work rates; nested cost is not added to its parent. Requested native bound8MiB, maximum candidate lifetimeRSS32,235,520bytes (30.74MiB); neither is a complete allocation calibration or memory-scalability certificate.

Requalify without remeshing:

```sh
PYTHONDONTWRITEBYTECODE=1 python3 integration_evidence/native_3d_core_v1/analyze_larger_wedge.py
PYTHONDONTWRITEBYTECODE=1 python3 integration_evidence/native_3d_core_v1/analyze_vertex_clip.py
```

Both qualifiers verify saved receipts/artifacts; vertex qualifier binds probe, baseline/candidate production delta,52 fixed-work files, full configured regression and syntax evidence. Actual source/binary pins are in each validation.json; ELF/object/pycache files are excluded from publication. All earlier failures remain untouched. Inspect vertex_clip_n4_n{1,2,4}_m{1,2,4}_v1/envelope_s0/1/2.vtu and actual final SU2/CGNS/state files.

Full Goal1 ACTIVE. Next: decouple the256-triangle global authority limit from local cavity bounds with scalable storage/search and memory admission, then complete communication/cache/private-wait/memory accounting, detail overhead and closed SGE repeated/scaling runs. Full sensor-source persistence, curved geometry,robust3DBL,physical unsteady3D and affordability are not certified here. User authorizes studying/reusing suitable existing operators when a concrete limitation warrants it; examine selected source licenses and benchmark saved failing cavities before transplantation. No external operator or new dependency was introduced in this checkpoint.
