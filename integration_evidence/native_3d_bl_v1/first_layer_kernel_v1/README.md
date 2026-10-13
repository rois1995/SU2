# Coordinated first-height kernel — serial geometry PASS

This is a private geometric kernel, not production CFD/BL adaptation.
ConstructFirstLayer reconnects an admitted cavity around a reserved background
point, then carves separate face-centroid normal wall children and their transition.
It reuses native coning, fixed-interface topology and immutable geometry checks.
Rejected proposals leave output unchanged. All node/cell IDs are caller-reserved.

CheckFirstHeight separately reports wall coverage, altitude error, tangent/normal
alignment, worst facet/cell and apex location. Empty height policies release the
constraint. Failure reasons distinguish insufficient clearance of the admitted
volume from an obstructing background; neither is a global infeasibility proof.
No sensor overwrite, hard normal reset, geometry or metric threshold change.

Four cases/944assertions PASS: construction plus repeated height changes, exact
pairwise conformity/no overlap, atomic rejection, tilt diagnosis, adjacent walls
and distinct local clearance failures. Independent Python checks pass positive
volume, unchanged boundary/original coordinates, unique wall coverage, altitude
and alignment. Inputs and output geometry are saved through the existing exporter.

Inspectable grids:
- first_height_initial_kernel.su2:6tetrahedra, Euler-type toy cube.
- first_height_constructed_kernel.su2:18tetrahedra, first altitude0.05.
- first_height_updated_kernel.su2:18tetrahedra, rebuilt first altitude0.025.

They have wall/far markers and are geometry demonstrations only. Metric/shape/
sliver/length contracts, global remote clearance, complete MPI dependencies and
atomic publication, solution/BDF/turbulence transfer, actual RANS/CGNS/restart and
performance remain engine/driver qualification work. Production BL remains guarded.
The separate hidden-wall density accuracy failure is still unresolved.

Reproduce after a current build:
python3 integration_evidence/native_3d_bl_v1/first_layer_kernel_v1/run.py
python3 integration_evidence/native_3d_bl_v1/first_layer_kernel_v1/export_and_check.py

The reused runner saves independent complete cases with source/build/object/binary/
helper/log provenance and refuses stale builds. One sampled CPU/nice19/threads1,
180s per case. No CFD timestep/cluster/performance campaign.
