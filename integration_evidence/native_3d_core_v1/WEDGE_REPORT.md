# Oblique wedge adaptation checkpoint

Repository: `/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated`.
Branch: `codex/native-3d-core`; predecessor `8acadb1bf67bba237cf245acc4f2d7774cf3c65f`.

Four wedge cases pass three actual native backend/driver refinement, coarsening and refinement events each. A fifth box control reproduces13 raw geometry/source/state/SU2CGNS nodal/frozen-readback files byte-for-byte. Production archives and all thin members match the published thin-domain checkpoint. Full Goal1 remains active.

## Domain and frozen contract

The domain is the triangular prism `x>=0`, `y>=0`, `x+y<=1`, `0<=z<=1`, with exact volume1/2. A structured triangular base is extruded; sorted base IDs choose a consistent diagonal on each shared prism face, creating three positively oriented tetrahedra per prism. The n2 input has18 points,24 tetrahedra and32 boundary triangles. It is a true conforming wedge subdivision; no crossing cube elements were filtered out.

Six immutable marker patches cover x=0, y=0, x+y=1, z=1 and two bottom patches meeting at x=.5. Surface connectivity can refine/coarsen; original facet geometry and feature/marker junctions remain exact. Independent rational-coordinate checks establish domain containment, positive volumes, no positive-volume overlaps, conforming incidence/links, complete finite-facet coverage and preservation of the oblique plane.

The unchanged prescribed sensor has reference aspect1.5, azimuth45–90deg/elevation0–30deg and strength1+.125x. Freeze its original sensor-only P1 target per event, with target scales4/1/4. Physical metric gates remain q>=.20, scaled Jacobian>=.05 and every original-source-resolved edge<=1.8; production/default512 rounds and Hessian noise0 stay unchanged. No sensor smoothing, clipping or geometry/quality relaxation was introduced.

| Case folder | CFD/workers | Accepted tetrahedra | Oblique triangles: input/refine/coarsen/refine | Parent seconds, three events |
|---|---:|---|---|---|
| wedge_n2_n1_v1 | 1/1 | 46/9/38 | 8/16/4/10 | 0.1414 / 0.2225 / 0.1623 |
| wedge_n2_n2_m2_v1 | 2/2 | 46/9/36 | 8/16/4/10 | 0.1544 / 0.2583 / 0.1759 |
| wedge_n2_n4_m2_v1 | 4/2 | 46/14/53 | 8/16/6/14 | 0.2278 / 0.2832 / 0.1748 |
| wedge_n2_n4_m4_v1 | 4/4 | 46/14/46 | 8/16/6/14 | 0.1410 / 0.1921 / 0.2471 |

Wedge-only bounds: minimum q=0.321433436, minimum scaled Jacobian=0.152512886, maximum metric edge=1.7930625853. Current/BDF2 relative residual<=3.84415e-16; transported and reloaded nodal original-P1 sensor residuals are zero.

Current/BDF2 checks use actual transferred affine synthetic states in physical coordinates. The solver does no CFD iterations in this fixture. SU2 ASCII and HDF5 CGNS files are independently parsed for topology, coordinates, positive tetrahedra, markers and closed dependencies, then loaded through SU2. `_reload.json` records loaded nodal sensors; `_reload_target.json` retains the frozen original donor field for metric/geometry checks. This does not establish admissibility of the relinearized interior source or a persistent complete-sensor restart.

Each wedge audit rejects six corrupted evidence variants, including an off-domain oblique vertex and a changed oblique marker name. The analyzer also proves that calling the legacy seven-marker default on a wedge still rejects it. The optional marker parameter is explicit; default box checks were not relaxed.

## Production and regression evidence

`wedge_regression_v1` passes64 selected native/2D+3D bridge tests on MPI1/2/4, using the actual configured68-object test driver and complete production archives. The actual MMG2D/3D round trips pass serially (48 assertions in two tests); their existing direct-MMG fixture is master-only. No unmatched selectors. Sources, objects, libraries, thin members and binaries remain stable across every campaign.

The source changes are the test-only wedge builder/envelope option, runner and independent geometry/marker audit extensions. The factory, arithmetic/operators, metric preparation/BL composition, donor interpolation/partition/transfer, 2D and MMG production paths are untouched. Archive identity against `thin_regression_v1` is checked. Default independent signature helpers additionally validate all six retained thin/box final SU2CGNS output pairs, including default-marker mismatch negatives. Historical evidence is unchanged.

## Cost and remaining work

The408 rows in `wedge_rank_phases.csv` distinguish exclusive lifecycle phases from nested native details and close per rank. The validation summary records parent and remesh maxima, accepted edits and useful edits per parent second, parent max/mean ratios, requested native buffer bounds and lifetime RSS. Metric preparation, diagnostic observation, remeshing and replacement are accounted; mesh output and external auditing remain separately timed.

Maximum requested native bound across the matrix is 8.0 MiB; maximum lifetime RSS is 28.96 MiB. These are different quantities; allocator/vendor/ParMETIS calibration remains open.

One sequential nice19/thread1 pilot per case ran on a shared eight-core host with four unrelated FSI ranks. Different N/M produce different coarsening/reconstruction trajectories. Timings are diagnostic samples, not fixed-work speedups or scaling evidence. Parent max/mean ratios do not measure private CPU balance or MPI waiting. Complete traffic/wait/private CPU profiling, detail overhead, repeated timings and progressive cluster scaling are pending.

Next Goal1 work is persistent manufactured-state restart, larger-domain/first-failure limits, remaining profiling and closed SGE campaigns. The fixture supports dyadic n4 generation, but this checkpoint qualifies n2 only. No physical CFD, curved geometry, 3DBL generation, full unsteady3D or affordability claim. No overnight cluster launch request.

## Grids and reproduction

Each wedge folder in the table contains `envelope_input.su2`, accepted `envelope_mesh_00001.su2` through `00003` and immutable `.native_ref` sidecars, `envelope_s0.vtu`/`s1.vtu`/`s2.vtu`, final `envelope_s2_written_SU2.su2` and `envelope_s2_written_CGNS.cgns`, original/accepted mesh and sensor JSON, current/BDF2 `_states.json`, logs/audits/pins/source snapshots. Example final grid: `wedge_n2_n4_m2_v1/envelope_s2.vtu` or `wedge_n2_n4_m2_v1/envelope_mesh_00003.su2`.

Recheck retained evidence without CFD:

```sh
PYTHONDONTWRITEBYTECODE=1 python3 integration_evidence/native_3d_core_v1/analyze_wedge.py
```

The frozen matrix is `WEDGE_MATRIX.json`; exact launch commands and stage costs are in each validation receipt. The new option `--wedge` is test-only and requires aligned markers and n2/n4. Local binaries/objects/pyc are excluded from Git; exported dependencies are closed.
