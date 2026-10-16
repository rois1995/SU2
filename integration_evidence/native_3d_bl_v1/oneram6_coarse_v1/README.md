# ONERA M6 Coarse.su2 — native RANS adaptation test

**Result: FAIL_BEFORE_REMESH. No candidate or adapted mesh exists.**

The supplied303359-tetrahedron/61330-point grid runs120 RANS/SA iterations with
physical WING3D no-slip, true SYMMETRY and FARFIELD, Mach0.8395/AOA3.06/Re11.72e6.
The tested native target requests first height2e-6/growth1.2/thickness.01, QLS
MACH/PRESSURE sensors/noise0, complexity100000 and unchanged acceptance gates.
Four CFD/adaptation ranks,300s native-entry and1200s whole-process limits.

- Initial attempt: completed120 CFD iterations, then hit a missing QLS WLS-fallback
  workspace assertion with Green–Gauss primal gradients. Shared allocation fixed.
  Existing analytic regression now exercises this combination:1 case/32260 assertions PASS.
- `retry_workspace_fix/`: identical mesh/configuration, completed120 CFD iterations;
  all122660 MPI point/sensor QLS fits succeeded without WLS fallbacks (1.42196s max rank).
  Complexity integration then exhausted65536 points on global cell13. It did not
  reach the remesher, mesh acceptance or solution transfer.
- `flow_checkpoint/`: matching standalone120-iteration flow exported successfully.
  Every shared printed history column agrees exactly with both attempts. Compact
  Float64 restart coordinates equal the input exactly; all fields finite, density
  and ideal-gas pressure positive. This is an unconverged starting-flow checkpoint.

Open `flow_checkpoint/flow.vtu` for the volume grid/flow and
`flow_checkpoint/surface_flow.vtu` for the surface. The reusable compact solution is
`flow_checkpoint/restart_flow.dat`; the matching grid is `Coarse.su2`. Working-directory input links
resolve to the preserved mesh within this case, not to the user's original.

Open `failure_cell_13.vtu` for the46-cell neighborhood. Select `FailureCell=1`
to highlight the reported original cell; `OriginalCellID`/`OriginalPointID` retain
source indices. Its centroid is (1.10496480,1.19815318,0.00539511), on the upper
outer wing near the tip. Its wall altitude is.00224586883, versus2e-6 requested.
This diagnostic is input geometry, not a rejected remeshing proposal.

`result.json` contains the assessment; `artifacts.json` pins saved files.
Per-attempt `provenance.json` records source/build/binary/config/input identities;
logs, histories, configurations, source differences and pre/post workspace sources
are retained. The original mesh is unmodified.

Recheck saved evidence from the repository root:

```sh
OMP_NUM_THREADS=1 OPENBLAS_NUM_THREADS=1 PYTHONDONTWRITEBYTECODE=1 \
  python3 integration_evidence/native_3d_bl_v1/oneram6_coarse_v1/summarize.py
```

This verifies the recorded current solver/source identities; rebuilding or editing
a relevant source correctly invalidates that identity check. Regenerate the small
patch with `locate_failure.py` beside it. Preserve existing folders before any new
run; both run.cfg files describe their inputs. No cluster task or extra adaptation
is queued. The next issue is geometry-resolved thin-BL volume integration on this
actual wing, before assessing exterior-wing mesh construction. No work-limit,
tolerance, geometry, first-height or metric relaxation was used.
