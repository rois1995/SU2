# Coupled BL construction/removal pilot

Coarse source: six-tetrahedron cube, height0.1, growth1.2, thickness0.3, identity
original P1 sensors. Both distinct-marker and ordinary wall/far-marker cold engine
construction remain rejected. Full Goal3 and production support are incomplete.

`validated_witness_v1/` preserves the first accepted private joint replacement.
`validated_engine_replay_v1/` contains the actual engine repair of its saved100-cell
input using immutable six-cell donors, and complete Euler release on MPI1/2/4.
Independent geometry/full metric/first-height audits PASS. Inspect saved_BL_input,
constructed_BL and released_euler SU2 files there; no CFD solutions are attached.

From the repository root after building the test driver:
`python3 integration_evidence/native_3d_bl_v1/coupled_engine_v1/run.py corner_engine_mpi1 corner_engine_mpi2 corner_engine_mpi4`
Audit each using `audit.py corner_engine_mpi1` (then2/4). Selected stages write
validation_focused.json; the full campaign failure stays preserved. Source,
fixtures, build/binary, inputs, helper, command and output hashes accompany evidence.
The first-engine rejected89-cell snapshot and subsequent rejected logs remain.

No CFD, transfer, RANS, curved/interacting-wall BL or performance qualification.
