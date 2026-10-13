# Coupled BL construction/removal pilot

Six-tetrahedron cube, height0.1, growth1.2, thickness0.3, frozen identity P1 sensor.
The distinct-marker and ordinary wall/far-marker engine pilots both reject the
remaining corner edge. Full engine construction and Euler release are incomplete.

`validated_witness_v1/` preserves a native-passing private joint repair of the
ordinary-marker saved failure, with an independent exact/analytic whole-mesh audit
and inspectable `grouped_joint_witness.su2`. This validates one reconstruction;
the engine has not yet discovered/published it, and no CFD solution is attached.

From the repository root, after building the test driver:
`python3 integration_evidence/native_3d_bl_v1/coupled_engine_v1/run.py balanced_analytic joint_witness`
Then run `audit_witness.py` in this folder through Python. Selected stages write
`validation_focused.json`; the original full-campaign rejection stays preserved.
No CFD, transfer, RANS, curved/interacting-wall BL or performance qualification.
