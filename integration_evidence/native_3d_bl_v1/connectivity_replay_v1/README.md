# Coordinated private repair and integration evidence

Run from the NativeIntegrated repository with the configured test/solver build:

```sh
PYTHONDONTWRITEBYTECODE=1 python3 integration_evidence/native_3d_bl_v1/connectivity_replay_v1/run.py integration
PYTHONDONTWRITEBYTECODE=1 python3 integration_evidence/native_3d_bl_v1/connectivity_replay_v1/audit.py integration
PYTHONDONTWRITEBYTECODE=1 python3 integration_evidence/native_3d_bl_v1/wall_reconstruction_v1/run.py sensor
PYTHONDONTWRITEBYTECODE=1 python3 integration_evidence/native_3d_bl_v1/wall_reconstruction_v1/audit_sensor.py
```

The first stage passes seven focused controls; its private witness passes shape,
geometry, height and footprint checks but retains oversized interface edges.
The required full stronger-field stage currently FAILS at the unchanged4096-work
ceiling. It accepts3 coordinated wall reconstructions and returns453 rejected
cells; independent audit confirms Lmax2.5437949>1.8. It does not reach removal or
CFD replacement. Keep this failure visible; do not turn it into a passing smoke test.

Collectors pin sources, build metadata, binaries, thin archive objects, configs,
commands and outputs, preserve superseded stages, and reuse only exact valid PASS.
The independent audit covers the private witness separately from the full event.
Artifacts remain inspectable as SU2 grids; rejected files carry `_rejected` in
their names. No new MPI, physical RANS accuracy or accepted-adaptation performance
qualification is claimed. See Handbook/GOAL3_WALL_RECONSTRUCTION.md for limits.
