# Current-source aspect2 turning-direction wedge validation — 2026-10-12

Working repository: /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated. Branch: codex/native-3d-transport-accounting.

The predeclared stronger metric matrix passes both serial N1M1 and MPI N4M2. Each size4 wedge starts with192tetrahedra, aspect2 sensor demand, spatial strength variation0.125 and turning principal directions (phi=pi*(0.25+0.25*x), theta=pi*y/6). The frozen field is original nodal P1, not the analytic formula re-evaluated at new points. Three actual native remesh/driver-replacement events refine, coarsen and refine again. Planar boundary connectivity changes while immutable facet groups and marker junctions remain preserved. No source, target, q/J/L or production512-budget change.

| Case folder | N/M | Accepted tetrahedra | Minimum q | Minimum J | Maximum L | Maximum state residual |
|---|---:|---|---:|---:|---:|---:|
| query_routing_a2_direction_n4_n1_m1_v1 | 1/1 | 402/54/424 | 0.360574696 | 0.165487609 | 1.796383465 | 3.7730e-16 |
| query_routing_a2_direction_n4_n4_m2_v1 | 4/2 | 402/71/435 | 0.203997406 | 0.058952194 | 1.795651615 | 3.7156e-16 |

Both cases have fresh independent exact embedding/conformity/no-overlap/volume/finite-facet/marker checks over eight input/accepted/readback frames, original-P1 quality/sliver/full-edge metrics, three current+BDF state pairs, actual SU2/CGNS readback and closed runtime/counter audits. Domain volume is exactly1/2; transported metric error is zero. q>=0.20/J>=0.05/L<=1.8 remain enforced. The MPI minimum q0.203997406/J0.058952194 are close to the guards: this is a bounded demonstrated envelope, not a margin guarantee for stronger metrics.

Serial and MPI trajectories differ. They are independently certified; byte identity across partitions and fixed-work timing are not claimed. Original complete receipts, logs, inputs, accepted SU2 meshes/current+BDF states, SU2/CGNS readbacks and three envelope_s0/s1/s2.vtu files are retained in the named folders. QUERY_ROUTING_DIRECTION_A2_VALIDATION.json pins the complete case receipts/audits and predeclared matrix. BinarySHA256: bb5a7664dda805daac6bbcc4b16e3da3ee3759f33475937beedd6a177c35955e.

These are one-CPU/nice19/MPI<=4/one-thread contention-authorized correctness checks. No local performance campaign was run. Size8 production512 completion remains limited; separate1024diagnostic feasibility does not change production. Repeated cluster total-cost/scaling/overhead/wait/traffic/memory qualification remains open. This matrix is manufactured anisotropic adaptation, not robust BL construction, curved geometry, physical unsteady CFD or fullGoal1 completion.
