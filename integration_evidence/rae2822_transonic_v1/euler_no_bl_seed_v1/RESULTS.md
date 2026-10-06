# RAE2822 Euler on user-provided no-BL seed

Native adaptation completed twice on four MPI ranks, solver exit0,28.372s.
Mesh counts:6952 ->8667 ->12788triangles;3592 ->4466 ->6538points.
All three solves met the configured density residual criterion(log10RMS<=-8).
Native quality/length gates reported qmin .435839/.530950,Lmax1.63736/1.66163;
zero shape/length/height/reference residuals in both remeshes.
Independent topology, exact mesh/restart coordinates, Float32 VTU scalar pairing,
admissible conserved flow and immutable original-polyline checks PASS.
AIRFOIL sampling192 ->198 ->203edges; max reference deviation<1e-6.

CL/CD: .807091/.006535 ->1.136641/.024719 ->.896210/.009459.
These changes are substantial: this is NOT grid-converged aerodynamic validation.
Convergence refers to the configured density criterion, not all residual fields.

Open mesh_and_mach.png and surface_cp.png for quick views. Full ParaView files
flow_adap_0000N.vtu contain mesh+flow; their matching grids are input.su2 forN=0,
mesh_adap_0000N.su2 forN=1,2, with solution_adap_0000N.dat double restarts.
inspection.json contains detailed independent checks and cycle histories.
