# B0 research program

Imported from `b0-repair` at c27a187a11578cddfeaa42fa384e25756329c24b.
This is a standalone MMG-based partition/repair experiment, not an enabled
SU2 remesher backend. Its repaired evidence protocol, immutable target audits,
state replay and failure-injection tools are retained for comparison work.
No production CMMGRemesher dump hook was imported.

The archived B0 assessment identified remaining interface damage, 3D growth,
coverage stagnation, MMG allocation overhead and weak scaling. E0 repairs the
measurement/protocol, not all those meshing defects. Do not treat this code as
a validated alternative to NATIVE_CAVITY or serial MMG. Existing Python job
scripts reference the original research case archive; supply local paths
before using them. The generator at this pinned revision already uses `first`
and `largest` piece selectors, correcting the preceding review finding.

Build explicitly (requires MPI, Eigen, MMG, SCOTCH and a built METIS archive):

```sh
python3 B0Spike/build_e0.py build-integrated B0Spike/bin
```

Build output uses `-j2`; run its jobs sequentially. `B0Spike/build.sh` accepts
the build directory and output directory as arguments. The program is not
part of the default solver or default test matrix.
