# Feature-aware/conical BL integration — whole milestone still FAIL

Working repository SU2_NativeIntegrated, branch codex/native-3d-bl.
The five original stages pass; hidden finite-wall accuracy remains unresolved.
No CFD timestep or production adapted BL mesh is claimed.

Implementation: retain existing positive polyhedron decomposition, split hidden
support by triangle face/edge/vertex planes, integrate supported pieces along
actual distance-resolved rays, and use the old sensor rule in certified outside
pieces. Only quadrature changes; source interpolation is original P1 and BL
composition remains at actual queries. Local-coordinate query generation and
bounded interior placement keep generated rounded points in the original cell;
no source extrapolation or sensor/tensor correction is admitted.

The strengthened hidden-wall test checks exact volume/moments and a separate
parallel-body density integral. A single right triangle with area A/perimeter P
has planar offset area A+P*r+pi*r^2. Integrating its radial density above z=.02
provides an independent oracle; the entire active support fits in the input cell.
The threshold remains5e-4 relative; caps remain65536points/16384geometry visits/
32depth/4096pieces. The latest fixed angular/radial rule uses65397points but
integrates2.4658690877 instead of2.4775664410 (about0.47% low). Whole aggregate FAIL.

Rejected steps: planes alone still reach point budget; initial conical source
rounding left the source cell; representable source queries resolve that; degree2
angular integration gives2.5268671273 (about2% high), degree5 gives2.4872367391
(about0.39% high), degree6 with lower radial order gives2.4658330488. Restoring
radial order can exceed budget; support-only integration/wider ray bands fit but
still fail accuracy. These are useful diagnosis, not qualification or speedup.

planes_only_failed.patch reconstructs the first failure from29c29f013e and was
hash-checked against its saved receipt. Current sources are in the branch and
current_experiment.patch. Every executed stage/aggregate/log/hash is retained.
Intermediate log/hash receipts remain historical; only snapshots explicitly
reconstructed/published provide complete source replay.

Next: replace fixed angular rules with error-controlled integration, preserving
all targets/caps and this stronger oracle; also pursue coordinated wall/transition
construction from LAYER_CONSTRUCTION_NEXT.md. Analytic authority, first-height,
composed gradation, actual RANS/MPI/transfer/restart/output and lifecycle cost
qualification remain full Goal3 requirements. No terminal owned job remains live.

Run after a current build:
python3 integration_evidence/native_3d_bl_v1/feature_partition_v1/run.py

The helper refuses stale builds and stores complete cases independently with
source/build/object/binary/helper/log pins. One sampled CPU/nice19/threads1,
180s per case. It is local serial correctness only.
