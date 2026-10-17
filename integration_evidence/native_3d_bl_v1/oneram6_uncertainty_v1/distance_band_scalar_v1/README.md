# Scalar distance-band qualification (2026-10-17)

These are integration/field replays, not adapted grids. Actual pointwise original
sensor P1 plus native geometric BL/finer/fade is unchanged. Strict mesh edges keep
nearest-feature transitions; provisional scalar integration uses distance-band
crossings from the same finite-feature envelope. Query ceiling8192 unchanged.

From NativeIntegrated, with the matching source and freshly built test_driver:

```sh
python3 integration_evidence/native_3d_bl_v1/oneram6_uncertainty_v1/bands.py
```

Existing complete stages reuse only identical source/build/binary/input/helper
provenance. If changed, preserve this folder and choose a distinct campaign; do not
delete the proof to overwrite a prior result. Native CFD was not advanced.

inputs/cell622.dat and cell2331.dat are exact original-host tensor/coordinate
inputs, with the original complete wall and core. inputs/provenance.json ties them
to the complete original frozen-field manifest and wall fixture. No external mesh
or CFD rerun is needed for these individual replays. The separate three-cell MPI
normalization runner requires run.py export if the regenerable field_inputs shards
are absent; its original NPZ/mesh sources and checksums remain the authority.

summary.json has original88/157/329 controls and new622/2331 seeds/bounded estimates.
Independent references use fixed positive tensor-Gauss quadrature via the native
POINT law, without the native adaptive controller or band cuts. Both charts and
orders24/48 are retained. Cell2331 agrees;622 does not. Its additional96 checks
can be reproduced using the SAME helper as the recorded inline invocation:

```sh
cd integration_evidence/native_3d_bl_v1/oneram6_uncertainty_v1
python3 - <<'REFERENCE'
from bands import fixed_reference
from run import HERE
p=HERE/'distance_band_scalar_v1'
for apex in (0,3):
    fixed_reference(p/'cell622_reference'/('order96_apex%d'%apex),p/'inputs/cell622.dat',96,apex)
REFERENCE
```

The96 results are still inconsistent; extension.json must not be interpreted as
a certified reference or error bound. No further order escalation is proposed.
Successful reference stages retain scalar/moment/provenance receipts and remove
regenerable binary query/value scratch; invalid/failed stages must be preserved.

resource_usage.txt measures the complete serial validation helper job,66.11s elapsed,
49.78 user+8.97 system seconds,59788KiB maximum processRSS. The96 extension resource
receipt is33.50s/30.15+1.69 CPU seconds/215564KiB, zero swaps. These are validation
costs and PROCESS maxima, not adaptation throughput, summed host memory, whole-mesh
cache qualification or scaling. Only one heavy task ran at a time, core0/nice19.

Whole-field mass, uncertainty calibration, actual M6 construction and production
normalization remain open. See Handbook/GOAL3_CHORD_ENVELOPE.md from the repo root.
