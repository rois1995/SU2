# Spatially changing metric directions — 2026-10-11

Repository `/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated`, branch
`codex/native-3d-core`, predecessor `8b64fe7dba33b62612065009981f1cae660952e1`.
Implementation/evidence checkpoint `ed79f20caf9708809722db5d0b871c71a495000f` pushed to origin/codex/native-3d-core; full remote SHA verified. Full Goal1 remains active.

## Fixture and frozen target

This checkpoint adds a test-only `--direction 1` option to the actual native3D
manufactured driver. At source vertices, with aspect a=1.5 and scale4/1/4:

    q = scale*(1+0.125*x)
    phi = pi*(0.25+0.25*x), theta = pi*y/6
    u = (cos(phi)*cos(theta), sin(phi)*cos(theta), sin(theta))
    M = q*(I+(a*a-1)*u*u^T)

Azimuth changes45–90degrees and elevation0–30degrees across the box. Nonzero xz
and yz coupling exercises a genuinely3D changing principal direction. Every
adaptation freezes sensor-only original P1 on its actual current source mesh.
New-point queries evaluate that frozen field, not the analytic formula above.
The analytic formula is used only to prescribe each event's original sensors.

The initial n2 box has48tetrahedra and adaptable planar boundary triangles with
aligned immutable marker junctions. Production factory default512 and frozen
q>=0.20/J>=0.05/L<=1.8, noise0, source/geometry/atomic gates are unchanged.
Direction0 executes the original sensor arithmetic. The final default-direction
control preserves all11 accepted/source/state/readback files byte-for-byte against
the pushed aspect2 predecessor. No production adaptation code changes.

## Qualification

`DIRECTION_MATRIX.json` was frozen before campaigns and retains the diagnostic
addendum after the initial reload failure. `DIRECTION_CHECKPOINT_VALIDATION.json`
verifies five actual three-event driver cases, production/source/object/archive/
binary identities, compact artifact hashes and408 closed exclusive/nested rows
in direction_rank_phases.csv. Four direction-changing cases pass exact embedding,
positive volumes, incidence, finite-facet/marker coverage, no overlaps/gaps,
original-field quality/sliver/length, current/BDF2 transfer and actual SU2/CGNS
output/readback. A fifth legacy control proves default compatibility.

| Folder | N/M | Accepted tetrahedra at three events | Minimum q / J |
|---|---|---|---|
| direction_a15_n2_n1_v2 | 1/1 | 86 / 14 / 85 | 0.380398 / 0.150717 |
| direction_a15_n2_n2_m2_v2 | 2/2 | 86 / 13 / 91 | 0.345975 / 0.163200 |
| direction_a15_n2_n4_m2_v1 | 4/2 | 86 / 12 / 79 | 0.376202 / 0.163200 |
| direction_a15_n2_n4_m4_v1 | 4/4 | 86 / 8 / 73 | 0.378010 / 0.164225 |

Maximum metric-edge length1.799380104, maximum state relative error3.84415e-16,
transported sensor tensor error0. Loaded SU2/CGNS nodal tensor error0 for each
case. Four negative controls per direction case reject current/state corruption
and missing/tampered reload nodal sources. Reference sidecars and marker geometry
remain unchanged. Different partition trajectories are independently qualified;
byte identity across them is neither required nor claimed.

Final `direction_regression_v1` compiles the complete configured68-object test
driver against actual archives and passes64 selected native tests at MPI1/2/4,
including all arithmetic/P1/segment/thin/filter/insertion/repair/budget/default,
native2D bridge and existing native3D bridge controls. The latter exercise all
callers of the changed output helper, barycentric/conservative/current+BDF2
transfer, M=N/M<N and actual output/readback. Actual MMG2D/3D round trips pass
separately at MPI1 (48assertions); unmatched selectors remain forbidden.
Existing324-frame automatic audits retain identical production identities;
this fixture-only checkpoint does not rerun that independent campaign.

## The reload failure and its correction

Initial direction_a15_n2_n2_m2_v1 completes allthree adaptations, but its
independent reload check fails: accepted final Lmax1.7993801038 versus reload
Lmax1.8146257462. The old helper reevaluates the analytic nonlinear sensor at new
mesh nodes. Its resulting P1 field differs from the original frozen field used
during adaptation. Affine fields in earlier cases had concealed this mismatch.
The initial serial direction case happened to pass that different-target check;
it is retained as superseded evidence, not the final target-preservation test.

For direction-changing output checks, the corrected helper reloads actual
transported nodal metrics from accepted CReaderSlices. It saves two records:
`*_reload.json` describes the imported nodal field on the accepted mesh;
`*_reload_target.json` carries the unchanged original frozen donor source.
The independent audit checks every unique reloaded node's tensor against original
P1, validates complete nodal-source topology/coverage/SPD, and checks output
geometry/residuals against the same frozen target as the accepted mesh. The guard
is unchanged. Default direction0 output behavior and its11datafiles stay intact.

Nodal transport does not prove that relinearizing those tensors on new cells
preserves the old field throughout their interiors. We do not claim the new
nodal field is independently admissible, or that SU2/CGNS mesh files persist the
full frozen donor field. The original source is retained in memory for these
output checks. This is not a complete solver restart/lifecycle certificate.
The failed receipt, exact inputs, outputs, source snapshots and diagnosis remain
in direction_a15_n2_n2_m2_v1 and DIRECTION_RELOAD_DIAGNOSIS.json; no old receipt or
metric threshold was rewritten. Both rebuilt serial/MPI2v2 cases qualify. Allnine actual adaptation/source/state
files from failed MPI2v1 are byte-identical to passed MPI2v2, proving that the
reload correction did not change the accepted adaptation trajectory.

## Performance scope

| Direction case | Parent maxima, seconds | Useful accepted edits per parent second |
|---|---|---|
| N1/M1 | 0.509785 / 0.549472 / 0.447675 | 107.89 / 43.68 / 125.09 |
| N2/M2 | 0.534728 / 0.620855 / 0.398623 | 102.86 / 40.27 / 137.98 |
| N4/M2 | 0.542462 / 0.616560 / 0.331290 | 101.39 / 40.55 / 150.93 |
| N4/M4 | 1.297876 / 1.148694 / 0.698907 | 42.38 / 22.63 / 72.97 |

Parent accounting includes manufactured metric preparation, diagnostic observation,
remesh and replacement. Requested output and independent audits are separate.
Engine/import/partition phases close; nested detail is not summed twice. Each
parent's max/mean is near1 because ranks finish matching collective work; this
does not establish balanced private CPU or absence of MPI waits. Timing pilots
are single nice19/thread1 local runs under four unrelated FSI ranks, and
partitions have different meshes/work. No scaling or speedup claim is made.
Maximum admitted native bound8MiB and lifetime RSS31313920B are distinct, not
calibrated full solver/ParMETIS/allocator memory measurements.

Inspectable VTU grids are envelope_s0.vtu through envelope_s2.vtu in the four
case folders above; original/accepted states, SU2/reference sidecars and final
CGNS are retained beside them. Legacy control folder:
direction_control_a2_n2_n1_v2. Executables and objects are not published.

Next: thin/wedge geometry, actual adapted restart and larger practical size/rank
limits, complete MPI wait/traffic/private CPU/memory calibration, instrumentation
overhead and closedSGE repeated/scaling campaigns. FullGoal1 is not complete.
No curved geometry, robust3DBL, actual CFD iterations/unsteady3D or operational
CFD affordability claim is made. No overnight cluster launch is requested.
AdapNoExt, foreign work, submodules and ClusterResults remain untouched.
