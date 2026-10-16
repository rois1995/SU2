# Three-element airfoil test input

User-supplied source: /media/rausa/4TB/Validation/ThreeElementAirfoil/30P30N/Meshes.
The file named2D contains a single extruded hex layer. `prepare.py` uses the existing
SU2 CGNS/HDF5 library to extract its unchanged z=0 cap and triangulate the quads.
`preparation.json` records hashes and positive/manifold/marker/area/topology checks.
The output `30P30N_coarse_tri.su2` is an INITIAL mesh, not an adapted result.
No CFD steps, adaptation, flow configuration or accepted workflow is supplied here.

Boundary001 is the exterior loop; Boundary004/005/006 are the three airfoil loops.
Names and coordinates are retained from the source. The CGNS export labels every
boundary BCWall, including exterior/caps; those labels must not define the CFD BCs.
The opposite extrusion cap differs slightly; its projection is diagnostic only.
The rejected exact-pairing assumption is retained in rejected_exact_cap_pairing.log.

Run `python3 prepare.py` here to reproduce with the existing build-integrated-v2
CGNS/HDF5 libraries. It requires the original CGNS path recorded above. Temporary
raw data is removed after validation. The full3D coarse grid has8.63M hexes and is
outside the current tetrahedral native backend input contract.
