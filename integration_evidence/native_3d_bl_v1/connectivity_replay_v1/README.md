# Coordinated wall repair and integration evidence

The retained-topology wall split and metric-directed complete dependencies resolve
the actual wall-seam blockage. A face-neighbor alone leaves an interior vertex
fixed; importing its complete star gives a35-cell private patch that passes the
original full-field refinement gates. No target or acceptance relaxation.

The complete stronger original-P1 field now independently passes construction and
removal on MPI1/2/4:649 BL cells→265/268/260 sensor-only cells. BL coordinates,
connectivity and physical markers are identical across ranks. Nine focused serial
integration controls pass1596 assertions. Earlier453-cell Lmax2.5437949 rejection,
malformed fixture extraction and the ineffective centroid-insertion experiment are
preserved; they are historical failures, not current accepted output.

From the repository root:

```sh
PYTHONDONTWRITEBYTECODE=1 python3 integration_evidence/native_3d_bl_v1/connectivity_replay_v1/run.py integration
PYTHONDONTWRITEBYTECODE=1 python3 integration_evidence/native_3d_bl_v1/connectivity_replay_v1/run.py visibility
PYTHONDONTWRITEBYTECODE=1 python3 integration_evidence/native_3d_bl_v1/wall_reconstruction_v1/run.py sensor
PYTHONDONTWRITEBYTECODE=1 python3 integration_evidence/native_3d_bl_v1/wall_reconstruction_v1/audit_sensor.py sensor
```

The same stronger-field collector/auditor accept `sensor_mpi2` and `sensor_mpi4`.
`visibility` is a saved-failure diagnostic; running it is not itself a certificate
of complete adaptation. `visibility_fixture.json` pins its original rejected mesh
and archived native receipt. The earlier `replay`/`audit.py` witness remains private
progress evidence with oversized interface edges, distinct from the complete result.

Collectors pin sources, build metadata, binaries, thin archive objects, commands
and outputs; preserve superseded stages; reuse only exact valid PASS. Accepted and
rejected SU2 grids remain inspectable. See the wall-reconstruction handbook for
coverage/alignment, curved/interacting geometry, physical accuracy and performance
limits. Goal3 remains incomplete.
