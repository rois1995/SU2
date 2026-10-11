# Native 3D bridge auditor self-check — 2026-10-11

This checkpoint prepares independent checks for the pending actual SU2 geometry/
reader/solution bridge. It does not establish a native 3D production driver or
successful CFD adaptation. The actual bridge campaign has not run yet.

`validation.json` PASS: a bounded serialized positive box fixture verifies the
original-P1 transported sensor, current affine flow and both BDF2 histories,
independent SU2 ASCII parsing, and independent HDF5 CGNS geometry/section/BC parsing.
Five deliberate corruptions are rejected: current state, transported tensor,
missing donor state, BC element range, and an external HDF5 link. The retained
HDF5 fixture is restored and closed. It is a synthetic checker fixture, not a
CGNS file certified through CGNS/SU2 or a simulation-ready adapted grid.

The original source is the pinned partition-controls unit-box fixture. Auditor
and imported helper hashes and exact self-check source are retained. Development
failures remain in geometry_audit_v1/v2: v1 used the wrong synthetic attribute
representation; v2 failed when writing provenance after the valid/adversary checks.
Neither failure involved native mesh generation or actual SU2 output.

The pending bridge tests will exercise actual owner/halo sensor import, weighted
M<=N adaptation and return, actual geometry/solver reconstruction, barycentric and
conservative solution/history transfer, strict independent embedding/metric/state
audits, actual SU2/CGNS writing and actual reader round trips. Independent raw
SU2/HDF5 checks additionally compare exact coordinates/connectivity/marker labels
and reject nonpositive output tetrahedra and external dependencies.

Actual library linking exposed stale pre-communicator helper ABI objects. A
268-step actual Common/strict/CFD-core incremental rebuild is underway in
geometry_build_v1, at nice19, -j1 and one numerical-library thread. Its live PID
must be checked, not inferred from active.json, before any restart. No additional
heavy job may be launched while it is live. All five failed standalone bridge
attempts are retained; no actual bridge test has passed or been claimed.

Full Goal 1, configured native3D driver integration, lifecycle rejection/accepted
publication, 2D regression and complete performance/RSS/overhead/scaling evidence
remain unfinished. No cluster launch is requested tonight.
