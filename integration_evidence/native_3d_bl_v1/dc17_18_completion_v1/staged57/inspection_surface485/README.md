# Latest provisional M6 surface

Open `wing_surface.vtu` in ParaView and choose **Surface With Edges**. `all_boundaries.vtu` additionally includes farfield and symmetry; threshold cell array `marker` (0 FARFIELD, 1 SYMMETRY, 2 WING3D).

These are actual physical boundary triangles of the saved build485 mesh. Test486 published no change. The mesh is provisional, not accepted, and no BL first row has been established.

`transported_sensor_tensor` contains nine row-major components of the current transported 3D nodal sensor tensor. It is **not** the composed flow/BL target or a separate tangential surface metric. `last_published_root_vertex` marks the latest moved root point. Reference facet and adjacent volume-cell IDs support tracing individual faces. Global q/J and geometry certification were not available and are omitted.

`surface_preview.png` is an orthographic wireframe preview; use VTU for exact coordinates and interactive inspection. `provenance.json` records source hashes, counts and successful readback of every exported point/cell array. Native checkpoint files and complete composed target remain unchanged.
