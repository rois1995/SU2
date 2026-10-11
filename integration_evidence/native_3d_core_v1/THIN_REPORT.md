# Affine thin-domain driver checkpoint

Repository: `/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated`.
Branch: `codex/native-3d-core`; predecessor `58b6b40fb046f130be4eef1fa03c4b9104763f47`.
Implementation/evidence checkpoint: `df5bcf48e758c9845ffea6bcf0dc7e17ab2dc355`, pushed and verified against the full remote branch SHA.

Six qualified cases complete 18 actual native backend/driver refinement, coarsening and refinement events. Production archives and every thin archive member are identical to the pushed direction checkpoint. Only the manufactured fixture, runner and independent auditor changed. Full Goal 1 remains active.

## Frozen target and checks

The slab is the affine image of the direction-changing reference box: `x=A*x_ref`, `A=diag(1,1,h)`, `M_phys=A^-T*M_ref*A^-1`. Heights are exact powers of two. Evaluate the unchanged reference sensor first, then scale finished tensor entries with `ldexp`; freeze original sensor-only P1 per event. Reference aspect1.5, strength1+0.125x, azimuth45–90deg and elevation0–30deg, target scales4/1/4. This is a coordinate/tensor-conditioning test, not BL construction or a new regularized target.

Physical original-P1 metrics still determine all accepted-cell checks: q>=0.20, scaled Jacobian>=0.05, every source-resolved edge<=1.8, unchanged production/default512 round allowance and Hessian noise0. Exact independent checks cover orientation, incidence, surface/vertex links, no positive-volume overlaps, domain volume and facet/marker coverage. Current and BDF2 fields are affine in physical coordinates.

Transported-tensor errors are reported in both physical and reference-box units. `A^T*M_phys*A` makes the existing absolute1e-13 residual meaningful across coordinate units; no change to physical mesh guards. All measured physical and reference transport/readback residuals are zero. Each thin case rejects seven deliberately corrupted evidence variants, including a1% normal tensor error and excess tensor components. Default unit-box behavior keeps the existing audit path.

| Case folder | Height | CFD/workers | Cells after three events | Parent seconds, three events |
|---|---:|---:|---|---|
| thin_control_h0_n1_v3 | 1 | 1/1 | 86/14/85 | 0.5885 / 0.6383 / 0.4869 |
| thin_h10_n1_v1 | 2^-10 | 1/1 | 86/14/85 | 0.5708 / 0.6292 / 0.3638 |
| thin_h10_n2_m2_v1 | 2^-10 | 2/2 | 86/13/91 | 0.5729 / 0.7285 / 0.5257 |
| thin_h10_n4_m2_v1 | 2^-10 | 4/2 | 86/12/79 | 0.9289 / 0.6552 / 0.3762 |
| thin_h10_n4_m4_v1 | 2^-10 | 4/4 | 86/8/73 | 1.0966 / 1.1958 / 1.0516 |
| thin_h20_n1_v1 | 2^-20 | 1/1 | 86/14/85 | 0.6105 / 0.6336 / 0.3705 |

Across all cases: minimum q=0.345974711, minimum scaled Jacobian=0.150717018, maximum metric edge=1.7993801038; maximum current/BDF2 relative residual=3.86033e-16.

Actual SU2 and CGNS files are independently parsed and loaded by SU2; loaded nodes carry the original frozen P1 sensor. As in the direction checkpoint, `_reload.json` contains the relinearized nodal field while `_reload_target.json` retains the original donor target for strict geometry/metric checks. This does not prove admissibility of the relinearized interior field or persistence of the complete frozen donor field in mesh files. Restart remains open.

## Performance scope

The442 rows in `thin_rank_phases.csv` separate exclusive lifecycle phases from nested native details; each closes against its own parent. `THIN_CHECKPOINT_VALIDATION.json` gives accepted edit counts, useful edits per parent second, max/mean parent times, native requested-storage bounds and lifetime process RSS per event. Metric preparation, observer diagnostics, remeshing and replacement are timed; output and independent auditing are separate. These are manufactured metric/transfer cycles, not CFD affordability measurements.

Maximum requested native bound is 8.0 MiB; maximum lifetime RSS is 29.73 MiB. Requested storage is not allocator/vendor/RSS calibration. Parent max/mean near one is not evidence of balanced private CPU or low MPI waiting.

One nice19/thread1 pilot per case ran sequentially on a shared eight-core host with four unrelated FSI ranks. N/M choices produce different meshes and work counts. The timing table is diagnostic evidence; it establishes neither scaling nor speedup. Repeated timings, instrumentation overhead, exact MPI traffic/wait/private CPU and vendor/allocator calibration remain required. No further local timing campaign is implied.

## Controls, regressions and retained failures

`thin_control_h0_n1_v3` reproduces13 raw geometry/source/current+BDF2/SU2CGNS nodal/frozen-readback files byte-for-byte from `direction_a15_n2_n1_v2`.

`thin_control_h0_n1_v1` passed independent mesh/state checks but failed the stronger exact baseline comparison: the extra multiply-by-one in a test tensor expression changed compiler association under the configured fast-math flags. The inspected yz entry differed by4.44e-16. It is superseded, retained with `THIN_CONTROL_V1_DIAGNOSIS.json`. No production fix or acceptance relaxation was made. The original frozen matrix is retained as `THIN_MATRIX_INITIAL.json`.

`thin_control_h0_n1_v2` failed compilation because the temporary fixture used array indexing on the named-member Tensor struct. It ran no numerical case. Its full failed receipt and source snapshot are retained, along with `THIN_MATRIX_CONTROL_V2.json`. V3 uses xz/yz/zz members and passes the exact control.

`thin_regression_v1` passes64 selected native mesh/dependency/transaction/budget/2D+3D bridge tests at each MPI1/2/4, using the actual configured68-object test driver. MMG2D/3D round trips run serially (48 assertions in two tests) because the existing direct-MMG fixture is master-only. All selectors match; all source/object/archive/binary pins remain stable. Production identities match `direction_regression_v1`. Existing324 independent automatic frames use the unchanged production identities and were not rerun for these fixture-only edits.

## Inspectable files and reproduction

Every qualified folder listed above contains `envelope_input.su2`, `envelope_mesh_00001.su2` through `00003`, matching immutable `.native_ref` sidecars, `envelope_s0.vtu`/`s1.vtu`/`s2.vtu`, final `envelope_s2_written_SU2.su2` and `envelope_s2_written_CGNS.cgns`, original/accepted JSON geometry and sensor sources, and `_states.json` current/BDF2 fields. Thin physical grids may require a display-only z scale in ParaView; the stored coordinates remain physical.

Representative final grid: `thin_h20_n1_v1/envelope_s2.vtu`; its SU2 counterpart is `thin_h20_n1_v1/envelope_mesh_00003.su2`. These are manufactured meshes, not solved RANS or Euler cases.

Recheck the retained evidence without running SU2:

```sh
PYTHONDONTWRITEBYTECODE=1 python3 integration_evidence/native_3d_core_v1/analyze_thin.py
```

The case command is recorded in each validation receipt; the new test-only option is `--height-power {0,10,20}`. Binaries are local build products, excluded from Git; reuse requires matching actual objects, complete libraries and all thin members, with verified-copy fallback for hard links.

Next Goal1 work: wedge and persistent manufactured-state restart, larger size/first-failure limits, complete communication/private CPU/memory profiling, and closed SGE detail OFF/ON, repeated and progressive scaling runs. No curved geometry, fullBL3D, full unsteady3D or affordable-CFD claim. No cluster launch requested overnight.
