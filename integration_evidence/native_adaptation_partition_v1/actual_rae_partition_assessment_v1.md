# Actual RAE worker-count evidence, current exclusive-profile source

Production checkpoint: `99aa0d7f725b0c872f969d6bd64c6ee9f8414165` on
`codex/native-unsteady-performance`. Application SHA256:
`77fbefa77179acd6d8e5c66da3eed61974f8dede4f4d2ce5937544cf9185c6f6`.
Source-pinned builds, quiet-machine gates, numerical threads=1, CFD ranks=4.

## RANS from coarse Euler geometry

Two 200-step adaptation windows in a 600-step SA RANS BDF2 run. These are
short developing-flow controls, not converged aerodynamic validation.
Only adaptation worker count differs; weighted partitioning enabled for both.
First donor conserved arrays and saved Float64 sensor metrics are exactly equal.
`rae_rans_subset_pair_v2.json` independently checks matched setup, full timing,
mesh gates and all four donor histories. Each case retains inspected grids,
original donor VTUs, transported restarts and its independent audit.

| Cost, seconds | M=3 | M=4 |
|---|---:|---:|
| Whole process | 482.4585 | 478.7907 |
| CFD including ordinary output | 398.0890 | 397.2730 |
| Metric construction/accumulation | 40.6189 | 33.0217 |
| Remesh including working partition and reader return | 42.9013 | 47.5823 |
| CFD geometry replacement including transfer | 0.3431 | 0.3496 |
| Adapted output | 0.0598 | 0.0584 |
| Total adaptation | 83.9231 | 81.0119 |
| Adaptation / CFD | 21.0815% | 20.3920% |

Transfer is nested in replacement; never add it separately. Exclusive worker
means close against engine-adapt means. Rank maxima cannot be added into a
critical-path time. Initial CFD geometry partition is outside adaptation;
return geometry partition/migration/preprocessing is inside replacement.

M=3 reduced remeshing by 9.84%, but total adaptation was 3.59% larger than
M=4. The first remesh favored M=3; the second favored M=4. These are different
meshes, operations and subsequent CFD trajectories. One run per mode plus
recorded editor/browser contention does not establish a performance advantage.
Do not recommend three workers from the frozen or first-window time alone.
The per-window comparison separates terminal metric accumulation from applied
adaptations: applied-window costs were 62.5885 s (M=3) and 66.3788 s (M=4),
while metric accumulation in the terminal window without remeshing was
21.3345 s versus 14.6332 s. This explains the opposite full-run ordering;
repeated controls remain necessary before claiming a reproducible advantage.

Both modes passed topology, original-polyline geometry and features, metric
quality >=0.18, Simpson lengths <=1.8 and prescribed first-height checks.
M=3 meshes: 12246/13091 points. M=4 meshes: 12216/13005 points.
All SA histories remain nonnegative; density and pressure are positive.
CLOSED-policy history residuals <=6.959e-13 in this pair. Raw whole-domain
integrals change slightly with open FARFIELD boundary resampling: audits
independently include its measured area change and verify near-constant
freestream states. This is not a claim of raw integral equality on changed
domains. Supplemental history receipts establish that both original donor
histories are saved at both events, despite generic sparse-history wording.

First-window private reconstruction min/mean/max:
M=3: 3.961/8.579/14.791 s; M=4: 3.430/8.829/21.485 s.
Initial working partition about 0.05 s; this does not measure a future
mid-remesh migration and ownership/cache rebuild. Dynamic rebalancing was
only discussed; no implementation or speedup claim is included here.

## Euler from a BL grid

M=3 completed both events: 7721/7569 points, quality minima
0.184179/0.328211, maximum lengths 1.798275/1.799184. No BL metric request.
Whole 117.812 s; CFD 95.2124, metric 4.08796, remesh 17.6747,
replacement 0.247041, adapted output 0.03458 seconds.
Full adaptation 22.044281 seconds, 23.153% of CFD. Independent actual frozen
sensor, geometry, both-history conservation/positivity and timing audits PASS.
The matching M=4 control also passed: 7720/7199 points, quality minima
0.222018/0.570416 and maximum lengths 1.772588/1.779675. Whole 172.066 s;
CFD 143.73 s and total adaptation 27.7778 s. First donor states and metrics
match M=3 exactly. The M=4 CPU-pressure peak was 36.33: these timings are not
a clean speedup control. Process-start evidence rejects the initial attribution
to a compiler observed after the run; both the rejected attribution and corrected
receipt are preserved. See rae_euler_subset_pair_v2.json. No worker recommendation.

## Remaining requirements

Both actual directions and subsequent remeshing after a saved window-boundary
restart are independently validated. The N4/M3 vortex restart reproduces donor
states, the next frozen metric, accepted mesh and resumed states bit exactly;
see vortex_restart_remesh_n4_m3_v1/restart_remesh_receipt.json. Mid-window
restart is not established. Repeat same-source comparisons, establish practical
memory/scaling limits and reproducible full-cost improvements; the self-contained
SGE campaign is prepared, not executed on a cluster. The goal remains active.
No native 3D, arbitrary-CAD, converged forces or general affordability claim.
