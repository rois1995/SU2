# Goal 3 first implementation: geometric composed edges

This is a serial kernel checkpoint, not a qualified native3D BL remesh.
Production ADAP_BL remains rejected until prescribed-height construction works.

Implemented:

- Faceted-wall distance-band breakpoints in 2D/3D, using the existing wall ADT.
  Face/edge/vertex offset roots find thin finite-wall support even when endpoint
  and midpoint samples miss it. Original fade and growth bands are retained.
- Composed3D edge quadrature uses exact original/declared source intervals and
  actual-point sensor interpolation/composition. The sensor-only path stays
  analytic. Missing/malformed bands, work exhaustion and nonconvergence reject.
- Breakpoint scratch is included in field storage admission; geometry and
  composition timings are separately recorded inside edge integration.
- A flat-wall regression exposed cancellation in the existing3D eigensolve of
  tangent size plus tiny squared normal height. At h0=1e-6 the normal eigenvalue
  was1.0016903085788469e12 instead of1e12, and the independent chord integral
  was9155.919298046 instead of9148.1963264072. See preserved failed logs.
  The primal wall metric now inverts its tangent block independently and forms
  its logarithm before mixing normal/tangent scales. This preserves prescribed
  height and smoothstep/log fade. It does not overwrite sensor demands.
  Active-scalar callers retain their previous wall inversion; AD runtime is not
  qualified by these tests.

Final pinned serial checks pass19 BL cases/3486assertions and25 field/acceptance
cases. Full results and invocation pins are in validation.json.

Run from the repository root after building UnitTests/test_driver:

```sh
python3 integration_evidence/native_3d_bl_v1/edge_integration_v1/run.py
```

The runner saves each complete kernel stage independently, pinning relevant
sources, compile commands, objects, binary, itself and logs. A saved PASS is reused
only if all those hashes and its invocation match. Tests use one sampled CPU,
nice19, threads1 and a120s per-stage limit. No CFD timestep, production remesh,
MPI/scaling or cost-qualification campaign is included. Durations are correctness
run metadata, not performance evidence. No binary/build copy is archived.

Pending Goal3 work: thin-tetrahedral volume quadrature and shared complexity,
immutable analytic3D BL authority, joint first-row/transition construction and
feasibility/coverage/alignment diagnosis, separate composed gradation audit,
actual repeated Euler/BL/RANS/MPI/current+BDF/restart/output qualification and
complete adaptation lifecycle performance. Full scope is GOAL3_REQUIREMENTS.json
in the parent folder and NATIVE_3D_DEVELOPMENT_GOALS.md.
