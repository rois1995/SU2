# Stronger anisotropy envelope — 2026-10-11

Repository `/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated`, branch
`codex/native-3d-core`. Production implementation is unchanged from donor-reuse
checkpoint `a406a1281a7d7bd16d45c88070737c82b1347897`; branch tip before these cases
was verified remote `157750a75bece663540c36e1ee6d68fbed07ce06`.

`ROBUSTNESS_EXPANSION_MATRIX.json` froze the two initial cases before execution.
Both use the actual native factory default512, q>=0.20/J>=0.05/L<=1.8,
ADAP_HESSIAN_NOISE=0, adaptable planar boundaries and aligned marker patches.
The initial mesh has48 tetrahedra. The target has a 2:1 aspect ratio, a constant
45-degree eigenframe, and scalar strength varying by1+0.125x. Its three scales
are4/1/4; each event freezes its actual sensor-only original P1 source. This is
stronger than the preceding1.5:1 case, but does not test changing eigenvectors.

Both cases PASS three actual refine/coarsen/refine replacement events, independent
exact positivity/embedding/incidence/facet/marker/volume and frozen metric audits,
original-P1/current/BDF2 transfer, actual SU2/CGNS write/readback and unchanged
reference sidecars. MPI4/M2 includes accepted return to allfour CFD ranks.
`ROBUSTNESS_STRENGTH_VALIDATION.json` verifies compact artifact hashes, no source/
object/archive drift, matching configured production archives/thin members, the
fixed recipe and exclusive engine/import/partition closure. Each case's retained
independent_audit.json also contains parent/rank closure and two negative controls.

| Folder | N/M | Accepted tetrahedra | Minimum q / J | Maximum metric edge |
|---|---|---|---|---|
| robustness_a2_n2_n1_v1 | 1/1 | 128 / 17 / 79 | 0.246842 / 0.0870495 | 1.798437 |
| robustness_a2_n2_n4_m2_v1 | 4/2 | 128 / 16 / 109 | 0.266667 / 0.134421 | 1.785889 |

Transported tensor error is zero, maximum state relative error3.81218e-16.
These are different partition trajectories; byte identity across them is not
required or established. Accepted VTU grids are envelope_s0.vtu through
envelope_s2.vtu in each folder; corresponding original source/state JSON,
SU2/immutable reference sidecars and final CGNS are retained alongside.

| Case | Parent maxima at three events, seconds | Useful accepted edits per parent second |
|---|---|---|
| N1/M1 | 0.815206 / 0.970013 / 0.328394 | 77.28 / 40.21 / 182.71 |
| N4/M2 | 0.978312 / 1.442031 / 0.376798 | 64.40 / 26.35 / 138.00 |

These are one nice19/thread1 diagnostic per case on a machine also running four
unrelated FSI ranks. Parent accounting includes manufactured metric preparation,
diagnostic observation, remesh and replacement. Requested output and independent
audits are separately timed. Nested timers are not added twice; phase maxima are
not summed to invent elapsed time. Maximum native admitted bound8MiB and lifetime
RSS31096832B are distinct and do not calibrate complete CFD/ParMETIS memory.
No scaling or repeat-qualified performance improvement is claimed.

The initial post-run analyzer mistakenly required identical library dictionaries
for the full68-object driver and the six-object actual lifecycle runtime. The
full driver additionally records libm.a/libdl.a/librt.a; every runtime production
archive and all thin members already matched. The analyzer now uses the existing
donor-analysis rule: each runtime archive must match the full configured driver,
with exact thin-member identities and shared source pins. The original analyzer
and failure record are retained. No numerical run, receipt or guard was changed.

Next frozen fixtures must vary metric direction in space, exercise thin/wedge
geometry and actual adapted restart, and expand size to locate practical limits.
Complete MPI wait/traffic/memory calibration and closed SGE instrumentation-
overhead/repeated/scaling campaigns remain open. Full Goal1 remains ACTIVE.
No CFD iterations, curved geometry, robust BL generation, unsteady3D or CFD
affordability qualification is claimed. No overnight cluster launch is requested.
Foreign work, AdapNoExt, submodules and ClusterResults remain untouched.
