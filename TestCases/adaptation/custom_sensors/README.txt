Small ADAP_CUSTOM_SENSORS cases (all filenames in generated configs are relative).
Run from any directory:
  python3 run_cases.py --binary /path/build/SU2_CFD/src/SU2_CFD \
    --baseline /path/build_mmg/SU2_CFD/src/SU2_CFD --output /tmp/sensor_cases

The runner waits whenever load exceeds 7, constrains runs to two CPUs, generates a
small triangle mesh (NACA uses QuickStart's mesh), writes logs and results.json,
checks selected-only output and finite data, compares the legacy adaptation loop
bitwise, and verifies unsteady history and restart after mesh replacement.
Use --cases name... to select cases, --ranks N for MPI runs.

The unsteady cascade deliberately selects PRESSURE + 0*(PRESSURE*PRESSURE), while
also evaluating a velocity gradient: this makes identical metrics/remeshes possible
and isolates changes to solution/history from changes in the selected metric.
The three *_nonlinear cases select PRESSURE*PRESSURE and check instantaneous
values and bitwise restart continuation after mesh replacement. Analytic nonlinear
sensor accuracy and convergence are also checked in the unit tests.

Unit filters: [CustomSensors] (grammar, binding, fields, symmetry, periodicity,
AD/DD differentiation) and [CustomSensorsMPI] (values/gradients/Hessians/metric,
by global point ID against a serial computation on each rank).
