# Preserved first fixture failure

qualification_v1/serial and diagnostic_serial_v1 are terminal failures of the new
fixture. The original executed-source capsule, binary/config/provenance and logs
are retained. Initial assertion0accepted versus1 did not diagnose concurrency:
all engine counters are zero and reason is "Native3D frozen composition/band
policies differ or composed integration is missing." The new fixture omitted the
required edge_breakpoints callback. The production guard correctly rejected it.

Fix fixture metadata using the existing {0,1} callback for its smooth field, and
choose sensorI with a small composed z variation: its first body-diagonal split
has admissible children, making a one-epoch publication assertion meaningful.
No production source/guard/mesh contract changes. New complete checks go to
qualification_v2. MPI had not started; no M6 run was launched from the failure.


qualification_v2/serial and diagnostic_serial_v2 are also preserved. The callback
fix admitted the field, but sensorI with small variation makes the cube's longest
edge~1.74 below the actual1.8 limit: the engine correctly completed with zero edits.
The separate independent composed-field FrozenField also needed the same band
callback. Both mistakes are in the new fixture. Use1.21I and a constant1.03125
xx composition; explicit setup assertions place the body diagonal above1.8 and
all face diagonals below it. The midpoint is exactly representable in each shifted
component. Pass the same {0,1} callback to the independent complete-field check.
No production change. Fresh complete checks use qualification_v3.
