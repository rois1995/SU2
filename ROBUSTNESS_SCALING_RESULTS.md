# Integrated robustness and scaling results — in progress

Updated 2026-10-06. Goal active. Branch `codex/native-integrated`, sibling worktree
`/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated`. AdapNoExt remains at
`88828474db587652ee1b0c09010de838ddbcfd06` with its original `.gitignore` change.

Source reconciliation is complete for the pins in BRANCH_RECONCILIATION.md.
The practical envelope is still being measured; the following are verified results.

## Integrated gates

| Gate | MPI ranks | Result |
|---|---|---|
| Fresh primal MPI/MMG/CGNS build, assertions on | — | PASS |
| SU2/binary SU2/CGNS output, mixed order and marker names | 1,2,4 | 7 cases per rank, PASS |
| Native geometry/field/BL/adaptation/output focused matrix | 1,2,4 | 49 cases per rank, PASS |
| Native transaction memory controls | 1,2,4 | PASS |
| Default MMG remesh/transfer/resumed-flow controls | 1,2,4 | PASS |
| Independent saved mesh/target/CGNS output checks | 1,2,4 | 84 audits, PASS |
| Goal metric/custom sensors/two-pass/reference/adjoint transfer | 1,2,4 | 58 cases per rank, PASS |
| Independent constant-target pilot geometry/topology audits | 1,2,4 | 12 cases, PASS |
| Fresh AD build | — | PASS, executables archived with hashes |
| AD GoalSwap controls, corrected MPI halo seeds | 1,2,4 | 3 cases/26 assertions per rank, PASS |

Evidence: integration_evidence/integrated_output_controls_v6,
integrated_primal_controls_v6, integrated_primal_memory_v6, integrated_mmg_default_v6,
integrated_primal_controls_v6/independent_outputs_v6, integrated_auxiliary_v9,
and scaling_pilot_v6/independent_audit.json. Each primal runner archives its
actual executable and source/config hashes; original failed runs remain intact.

## Initial engine pilot

Constant frozen tensor, aspect ratio10, opposing walls with exact h0=.004,
adaptive physical boundary sampling, cyclic ownership. One fresh run per case;
times exclude the test-only audit gather and CFD/transfer/output. All12 cases
meet q>=.18, metric length<=1.8, wall-altitude error<=1e-8 and reference
deviation<=1e-10, and pass independent exact orientation/topology/area checks.

| Input cells | Ranks | Output cells | Adapt seconds | Selection seconds max | Collective seconds max | Cross-owner / commits | Conflicts | Peak RSS KiB/rank |
|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| 16 | 1 | 14 | 0.033381 | 0.000301 | 0.002133 | 0 / 42 | 0 | 20324 |
| 16 | 2 | 14 | 0.040145 | 0.000204 | 0.009440 | 11 / 24 | 129 | 20716 |
| 16 | 4 | 14 | 0.124596 | 0.000208 | 0.046673 | 17 / 24 | 423 | 20720 |
| 64 | 1 | 50 | 0.136422 | 0.002244 | 0.007004 | 0 / 168 | 0 | 20496 |
| 64 | 2 | 46 | 0.178915 | 0.001505 | 0.029264 | 99 / 155 | 270 | 20636 |
| 64 | 4 | 56 | 0.307765 | 0.001338 | 0.101928 | 154 / 197 | 656 | 20788 |
| 256 | 1 | 222 | 0.719293 | 0.021229 | 0.029651 | 0 / 700 | 0 | 20384 |
| 256 | 2 | 200 | 0.773803 | 0.014022 | 0.103090 | 678 / 829 | 407 | 20536 |
| 256 | 4 | 198 | 0.769291 | 0.009674 | 0.232830 | 626 / 661 | 1065 | 20936 |
| 1024 | 1 | 766 | 3.443461 | 0.099079 | 0.066021 | 0 / 2145 | 0 | 21508 |
| 1024 | 2 | 772 | 3.514976 | 0.073940 | 0.269569 | 2086 / 2646 | 282 | 21036 |
| 1024 | 4 | 764 | 3.326471 | 0.051029 | 0.639058 | 2898 / 3079 | 1277 | 21172 |

At1024 input cells, the observed1/2/4-rank times are3.44/3.52/3.33seconds.
This pilot shows little benefit from additional ranks at this size. It is not
a reliable speedup estimate: single samples, different output cell/transaction
counts, and adversarial ownership. In the four-rank1024-cell case,2898 of3079
commits cross ownership, with1277 conflicts. Repeated low-cut geometric
ownership controls will distinguish this effect from unavoidable collective cost.

Whole-process VmHWM includes startup and the replicated manufactured input.
It does not describe production CFD memory or prove the2MiB transaction-work
bound. Instrumented World counters do not include every MPI-library operation.

## Demonstrated constraints and pending limits

- Native supports static, single-zone, primal-double2D triangles. Native3D,
  CAD projection, mixed volumes, moving/time-domain meshes and native adjoint
  adaptation remain unsupported. These are capability limits, not benchmark failures.
- Geometry follows the immutable original marker polyline while its sampling
  adapts. The tests do not establish CAD conformance or3D prism-layer generation.
- Explicit defaults:2MiB transaction work,256 donor cells,128 discovery regions,
 64 cached candidates. Original boundary/reference and reader master boundary
 rows remain O(B). Larger-volume work and communication costs still require measurement.
- A three-cell/six-vertex mixed CFD fixture aborts at four ranks with an empty
 sparse pattern. Expanding its unchanged interleaving motif makes the writer
 regression pass. This is an observed too-small CFD partition limitation.
- MPI test-harness defects found and repaired: serial mesh counts, replicated
 SU2 boundary rows, omitted box coloring, rank-local symmetry/wall coverage,
 and colliding reference-file names. Saved failures are evidence of those
 defects, not native mesher construction failures.

Remaining campaign: escalate engine size with vertical geometric strips;
three repeated samples per rank for cyclic, horizontal contiguous and vertical
ownership; matched AR10/100/1000 and a separately proved incompatible AR25
wall/metric request; fresh actual NACA conservative transfer and resumed
viscous solves with thinner h0/higher complexity; opposing-wall audits; AD
warm/cold lifecycle and single-rank interruption propagation. See
ROBUSTNESS_SCALING_PROTOCOL.md and live JSON states in HANDOFF_Codex.md.

Do not call the goal complete or publish a large-rank/3D scaling claim from this pilot.

## MPI adjoint mesh-swap defect found during integration validation

Original GoalSwap v9 passed1 rank but failed2. Owned primal/adjoint transfer and
objective were exact, yet continuation residual discrepancy was3.2863 and final
adjoint relative difference.999103. Tight Jacobi solves reproduced the failure
with unchanged point ownership and recorded primal linear residual8.2855e-14,
ruling out the initial incomplete-ILU hypothesis. The transfer utility supplies
owner copies on halos, but the DA driver seeds every recorded primal output;
its recorded halo exchange then duplicates those contributions. Clearing only
halo adjoint seeds after SwapMesh retained owned warm starts and restored exact
continuation in the2-rank v13 diagnostic. Original ILU-settings regression at
1/2/4 passed as v14: all3 cases/26 assertions per rank; same-mesh transfer,
objective, per-iteration residuals and final adjoint differences are exactly0.
All measured point ownerships are unchanged in this continuation fixture.
This establishes the correction, not partition-independent iteration histories. Production correction is limited to DA mesh swap;
primal interpolation and its halo field semantics are unchanged.

Measurement scope clarification: World traffic/exchange-work/collective counters
include engine initialization and adaptation, while adapt_seconds excludes
initialization and selection_seconds_max measures adaptation selection only.
The maximum exchange-work counter can include the initial incidence-directory
transport, which uses no transaction ceiling; a value above2MiB is not itself
an admitted transaction violation. The probe enlarges a tiled rectangular strip
at fixed ny=4 and constant target, so boundary size grows with input cells. It
measures that workload, not a fixed-domain general2D/3D scaling law. Reference
construction and the SU2 import/transfer/CFD path are outside its timing.

## First low-cut size group — independently verified

At2048 input cells, all1/2/4-rank vertical-strip cases are complete and pass
independent exact geometry/topology/reference/target audits (large_t128).

| Ranks | Input cut edges | Output cells | Adapt seconds | Cross-owner / commits | Conflicts | Peak RSS KiB/rank |
|---:|---:|---:|---:|---:|---:|---:|
| 1 | 0 | 1532 | 9.453158 | 0 / 4220 | 0 | 22660 |
| 2 | 4 | 1548 | 6.403417 | 48 / 4360 | 3 | 22292 |
| 4 | 12 | 1512 | 4.797245 | 204 / 5316 | 28 | 21876 |

These are single samples with different adapted cell/transaction counts. They
show a useful low-cut control; repeated partition comparisons remain pending. Raw evidence:
integration_evidence/robustness_campaign_v8/large_t128/independent_audit.json.

The4096-cell vertical-strip group also passes1/2/4 independently (large_t256):

| Ranks | Input cut edges | Output cells | Adapt seconds | Cross-owner / commits | Conflicts | Peak RSS KiB/rank |
|---:|---:|---:|---:|---:|---:|---:|
| 1 | 0 | 2948 | 28.019265 | 0 / 8403 | 0 | 26232 |
| 2 | 4 | 2883 | 15.194881 | 58 / 7932 | 10 | 23792 |
| 4 | 12 | 2957 | 9.204581 | 175 / 8534 | 11 | 22984 |

No transaction-memory rejection occurred in these six larger cases. The first
failure/cost boundary is reported below; repeated comparisons remain pending.

The8192-cell vertical-strip group passes1/2/4 independently (large_t512):

| Ranks | Input cut edges | Output cells | Adapt seconds | Cross-owner / commits | Conflicts | Peak RSS KiB/rank |
|---:|---:|---:|---:|---:|---:|---:|
| 1 | 0 | 5808 | 98.935874 | 0 / 16714 | 0 | 31928 |
| 2 | 4 | 5834 | 52.893111 | 48 / 16622 | 6 | 27236 |
| 4 | 12 | 5766 | 28.137930 | 163 / 16012 | 11 | 25636 |

This is the largest independently verified rank-common strip size so far.
The16384-cell1-rank job exceeded the declared240-second cost budget, as reported
below. No construction failure is inferred from that timeout. At8192 cells the serial
selection time is7.158s and World collective time is.632s (init+adapt), compared
with98.936s adaptation. Those counters do not account for every cost, and do
not profile individual geometry/transaction routines. Source inspection shows
full component scans in PolylineReference::Parameter and ::Deviation and an
owned-cell scan for boundary CollisionVeto. These are concrete candidates for
profiling; their measured fraction is not yet established. No scheduler or
geometry validation change has been made to improve the benchmark.

The real-airfoil fixture uses10216 seed triangles and three short primal
iterations per cycle before producing each actual sensor/BL field. Its resumed
viscous run tests lifecycle/admissibility and transfer, not converged CFD accuracy.

## First observed cost boundary

large_t1024_p1 (16384 input triangles, vertical-strip layout) exceeded the
240-second whole-job budget; the supervisor returned124 after241.41s including
termination. No finished native_scaling.json was produced. This is a measured
budget limit for that serial strip workload, not proof of infeasibility or of
invalid partial topology. Larger2/4-rank limits remain unmeasured; follow-through
is prescribed in the protocol. Repeated comparisons use the8192-cell largest
complete common size. Encoded payload counts include self buckets, and World
collective counters omit some direct MPI calls/votes; do not interpret them as
network traffic or a complete communication-time decomposition.


## Repeated cyclic ownership — independently verified interim result

All nine layout1 repetitions at8192 input triangles pass independent exact
orientation/topology/perimeter/area and frozen metric/height/reference checks.
The auditor used immutable copies of only finished cases, leaving the live
campaign group and its scheduled final audit untouched. Evidence:
integration_evidence/partition_layout1_audit_v2/independent_audit.json.
Raw statistical snapshot: integration_evidence/partition_progress_summary_v1.json.

| Ranks | Samples | Adapt seconds min / median / max | Output cells each | Cross-owner / commits each | Conflicts each |
|---:|---:|---:|---:|---:|---:|
| 1 | 3 | 97.988 / 99.516 / 101.773 | 5808 | 0 / 16714 | 0 |
| 2 | 3 | 94.756 / 95.533 / 115.950 | 5804 | 12806 / 15699 | 346 |
| 4 | 3 | 52.242 / 52.513 / 53.654 | 5750 | 12172 / 12954 | 2377 |

Within each rank/layout, these repetitions have identical output counts and
transaction counts. Cyclic ownership is materially more expensive than the
single vertical low-cut sample at2/4 ranks (52.893/28.138s); repeated low-cut
controls are required before quantifying the comparison. Rank-dependent output
and transaction counts still preclude an identical-work speedup claim.
No memory admissions were rejected in these nine runs.

The failed partition_layout1_audit_v1 contains zero cases: the preliminary copy
command used a Python method unavailable in installed Python3.8. It never
examined a mesh. The corrected v2 copied and audited all nine cases successfully.
Do not classify v1 as a mesher failure.

Host observation during the next horizontal-strip repetition also found a
foreign two-rank periodic SU2_CFD_AD job running alongside our one-rank probe.
Our supervisors start each heavy job only after the observed machine is quiet;
this cannot prevent another user starting a job afterward. No foreign process
was stopped. All reported timings remain shared-workstation observations.

Measurement gap to close before completion: actual driver logs report
ReplaceMesh/solution-transfer times on the master rank, without an all-rank MAX.
Native phase logs report per-phase rank maxima, which cannot be summed into an
exact remesh wall-clock maximum. Keep those diagnostics distinct from the
engine probe's measured wall maximum. The final report still needs actual
remesh and transfer/repartition phase measurements meeting the protocol scope;
add only the necessary test instrumentation after live binary-pinned campaigns
finish, then run a separately labelled control rather than changing a binary
under an active campaign.
