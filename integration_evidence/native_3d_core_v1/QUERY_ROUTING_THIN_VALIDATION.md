# Query-routing thin-domain correctness — 2026-10-12

Working repository: /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated. Branch: codex/native-3d-transport-accounting.

All three predeclared size4 wedge cases pass three refinement/coarsening/refinement events with the current qualified query-routing binary. Physical height is 2^-H; sensor xz/yz components scale by 2^H and zz by 2^(2H). These are new physical thin inputs, not byte-identical unscaled inputs. The original rotated, spatially varying P1 target, q>=0.20, J>=0.05, L<=1.8, noise0 and production512 rounds remain unchanged. No C++ or production setting changed.

| Case folder | H | N/M | Accepted tetrahedra by event | Minimum q | Minimum J | Maximum L | Maximum state residual |
|---|---:|---:|---|---:|---:|---:|---:|
| query_routing_thin_h10_n4_n4_m2_v1 | 10 | 4/2 | 289/48/314 | 0.238838158 | 0.102718740 | 1.799674941 | 3.8401e-16 |
| query_routing_thin_h20_n4_n1_m1_v1 | 20 | 1/1 | 289/52/308 | 0.257100414 | 0.146401738 | 1.795778117 | 3.8808e-16 |
| query_routing_thin_h20_n4_n4_m2_v1 | 20 | 4/2 | 289/48/314 | 0.238838158 | 0.102718740 | 1.799674941 | 4.6067e-14 |

Each case has a fresh independent audit of eight mesh frames, three current/BDF state pairs, exact orientation/conformity/no-overlap/domain/marker checks, original-P1 metric quality/length checks and actual SU2/CGNS readback. Transported metric tensor error is zero in both physical and normalized units. Volumes are exactly 1/2048 for H10 and 1/2097152 for H20. Changing surface counts confirms planar boundary connectivity adapts; all reference marker regions remain preserved.

After undoing physical z stretching and covariant sensor tensor scaling, all eight geometry/donor frames of each N4M2 thin case equal the qualified unscaled query_routing_n4_n4_m2_v1 frames exactly: sixteen comparisons. This includes connectivity, IDs, surfaces, facet groups/markers and original donor metrics. Physical affine solution fields change with physical z and are independently audited rather than equated across heights. Serial and MPI meshes need not be identical.

Reproduce the receipt/hash, strict-gate and exact covariance checks:

```sh
PYTHONDONTWRITEBYTECODE=1 python3 integration_evidence/native_3d_core_v1/check_query_routing_thin.py
```

QUERY_ROUTING_THIN_VALIDATION.json pins each complete validation/audit and the checker. Each case validation pins its source/object/archive/binary/tools/artifacts; binary SHA256 is bb5a7664dda805daac6bbcc4b16e3da3ee3759f33475937beedd6a177c35955e. Original logs, three accepted SU2 meshes, SU2/CGNS readbacks, current/BDF states and envelope_s0/s1/s2.vtu remain in the named case folders. Total retained case data is about 6.5 MB.

## Goal 1 gates still open

The current size8 N1M1 production512 campaign still rejects its second event after437accepted/75rejected attempts, with517valid cached cells and266pending coarsening directions; accepted CFD state is preserved. The separately certified1024diagnostic campaign completes all three events. Thin-case success does not remove that production work-budget limit. Engine maximum_rounds bounds distributed transaction rounds, not full mesh sweeps; the current checked-in limit is unchanged.

Closed instrumentation and bounded admission are tested, but sustained speedup, complete cost scaling, detail/profiler overhead, MPI imbalance/wait/traffic and allocator/RSS calibration need repeated cluster measurements. The SGE comparison is prepared separately; no jobs submitted here. These local cases use one CPU, nice19, MPI<=4 and one numerical thread under observed contention; their timings are correctness diagnostics only.

This establishes manufactured thin-domain covariance, not actual geometric BL generation, curved geometry, full 3D unsteady CFD, or affordability. Goal1 remains ACTIVE; Goal2 has not started.
