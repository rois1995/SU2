Mesh-adaptation CAPABILITY regressions: small runs of the in-SU2 adaptation loop (ADAP_LOOP= YES, MMG), judged by
mesh and metric gates only. Flow convergence is NOT a gate (the runs are a few iterations / time steps).

  python3 run_capability.py --binary BUILD/SU2_CFD/src/SU2_CFD --output OUT [--ranks N] [--cases NAME|TAG ...]
  python3 run_capability.py ... --ranks 1; ... --ranks 2      (np 2 compares with the np 1 run in OUT)
  python3 capcheck.py --selftest                              (checker self test, also run by the runner)
  python3 ../../adaptation_regression.py --binary ...         (serial set, the entry point next to the other scripts)

Needs numpy, SU2_CFD built with MMG (-Denable-mmg=true) and mpirun for --ranks > 1. The runner waits while the load
is above 7, pins itself and SU2 to 2 CPUs (4 ranks are oversubscribed on them), kills a run after --timeout s (300).
All meshes are generated (meshgen.py) or taken from QuickStart; no data files. The linear preconditioner is JACOBI
(common.cfg): ILU is block-wise per rank and makes np 1 and np 2 solve different flows before the first remesh.
Output: OUT/<case>_np<N>/ (mesh.su2,
run.cfg, run.log, mesh_out*.su2, restart and VTU files), OUT/results.json, OUT/summary.txt (one column per gate).
WINDOW_AVERAGE and FIXED_POINT also run an independent control after the main run, in a fresh
OUT/<case>_np<N>_window/ with the same rank count and run_su2 settings. The control uses the first window's mesh:
mesh.su2 for WINDOW_AVERAGE; mesh_out_00000.su2 for FIXED_POINT (the accepted mesh, whose solve starts from the
initial condition). Its case config has ADAP_LOOP= NO, TIME_ITER= ADAP_FREQ (steps 0..e, e = ADAP_FREQ-1),
HESSIAN in VOLUME_OUTPUT, OUTPUT_WRT_FREQ= (1, 1), and no restart input options (RESTART_SOL, RESTART_ITER,
SOLUTION_FILENAME). Without the loop, Postprocess calls ComputeMetric every step, so control flow_<k>.vtu holds
instantaneous Hessians at every step, including e. Summary solver times exclude control and restart runs.
Not in serial_regression.py: the CI binaries have no MMG.

Cases (tags: 2d 3d steady unsteady fixed free bl custom mpi4)
  naca_free           2D Euler NACA0012, 2 steady cycles (3000, 4000), ADAP_SURFACE= YES, sensors MACH + PRESSURE
                      KNOWN FAILURE: the free airfoil deviates 3.5e-3 .. 5.4e-3 from the input near the leading
                      edge whatever ADAP_HAUSD (4e-4 .. 1e-2; limit 2 hausd + sagitta = 4.1e-3): MMG2D's Hausdorff
                      bound is not effective there; PASS or XFAIL with the flow solution (ILU: 5.3e-3, JACOBI:
                      3.5e-3); expected gate: free_boundary_geometry
  naca_fixed          same, 1 cycle, ADAP_SURFACE= NO (boundary bitwise), metric checked against numpy (2 sensors)
  plate_metric        2D laminar flat plate (symmetry + wall), BL METRIC on the wall (h0 5e-5), free patches
  plate_metric_fixed  same with ADAP_SURFACE= NO
  bump2d_twopass      2D laminar channel over a curved bump wall, ADAP_BL_METHOD= TWO_PASS, 2 cycles
  vortex_wa           2D inviscid vortex, ADAP_UNSTEADY_METRIC= WINDOW_AVERAGE, custom sensor, free patches, restart
  vortex_predict      same, PREDICT, custom sensor sqrt(|u|^2), fixed patches, restart
  vortex_fp           same, FIXED_POINT (ADAP_FP_ITER 1), PRESSURE, free patches, restart
  bump3d_free         3D Euler channel with a 3D bump (tetrahedra), free patches, metric checked against numpy
  bump3d_fixed        same with ADAP_SURFACE= NO            needs the MMG #333 fix (hangs without it)
  bump3d_bl           3D BL METRIC on the bump, free patches
                      KNOWN FAILURE: MMG3D does not follow the 3D BL metric (metric edges in [0.71, 1.41] ~50 %,
                      quality 1st percentile ~0.04, ~4x more points than the metric complexity); BL heights are fine;
                      expected gates: metric_edges, metric_quality, points_vs_complexity
  bump3d_fixed_bl     3D BL METRIC on two walls meeting at a ridge, ADAP_SURFACE= NO
                      needs the MMG #333 fix; the BL ridge is the mechanism the current #333 patch breaks (refined
                      patch pending): bl_ridge_cell_height requires eligible ridge faces
                      KNOWN FAILURE with the current #333 patch: lower wall 75 % and its ridge faces 55 % of the area
                      in [0.5, 2] h0 (cells too tall); expected gates: bl_cell_height, bl_ridge_cell_height
  bump3d_wa           3D WINDOW_AVERAGE, ADAP_SURFACE= NO, custom sensor, restart   needs the MMG #333 fix
np 1 and 2: all cases; np 4: tag mpi4 (vortex_wa, vortex_predict, bump3d_free).

Gates (per written mesh; worst over the meshes of a case). Files: steady cycle k writes restart_flow_adap_0000k.dat
(non-compact, double) with the metric that makes mesh_out_adap_0000(k+1); unsteady: flow_<e>.vtu of the window end e
(Float32) holds the metric of mesh_out_<e+1>; FIXED_POINT meshes come from discarded solves (no metric file).
  run                  exit 0, "Exit Success", no timeout, MMG status SUCCESS for every remesh (TWO_PASS: pass A
                       accepted every time), no "MMG could not fully adapt" / "MMG made the metric coarser" warning,
                       number of meshes as expected for the mode
  log_counters         printed complexity == target (non-BL); WINDOW_AVERAGE / FIXED_POINT: samples per window ==
                       ADAP_FREQ; PREDICT: a feature speed > 0 every window; FIXED_POINT: solves per window ==
                       ADAP_FP_ITER + 1
  positive_volumes     every element has a positive signed volume
  conformity           every element face in <= 2 elements; every boundary face in exactly one marker; no marker face
                       that is interior, not an element face, or repeated
  points_elements      no unused or duplicate points (1e-12 of the size), no duplicate elements
  markers              same marker names as the input, none empty
  metric_file          readable metric file on the donor mesh with all Metric_* fields; mandatory for steady,
                       WINDOW_AVERAGE and PREDICT; absence allowed only for FIXED_POINT (discarded solve)
  metric_field         metric file: finite, SPD, sizes in [ADAP_HMIN, ADAP_HMAX] (1 % slack), aspect ratio <= ADAP_ARMAX
                       BL metric: sizes in [0.5 min(ADAP_HMIN, smallest ADAP_BL_FIRST_HEIGHT), ADAP_HMAX], same slack;
                       aspect ratio reported only (the BL intersection follows the global bounds)
  metric_nontrivial    95th / 5th percentile of sqrt(det M) >= 4
  points_vs_complexity output points / target: 2D [0.7, 2.0], 3D [0.7, 3.0] (BL: / complexity of the metric file;
                       bump2d_twopass 2D [0.7, 3.0])
  metric_complexity    sum sqrt(det M) |K|/(d+1) of the metric file == target: 1e-6 (double), 2e-3 (Float32); BL report
  metric_edges         output edges in the metric P1-interpolated from the donor mesh (length = mean of the two
                       endpoint lengths, an approximation of MMG's own): mean in [0.8, 1.3], fraction in [0.71, 1.41]
                       >= 60 %. Report only for fixed patches (MMG got a floored boundary metric) and TWO_PASS
  metric_quality       mean-ratio quality in the element's mean metric: 1st percentile >= 0.1 (same scope)
  metric_reference     steady naca_fixed / bump3d_*, unsteady WINDOW_AVERAGE / FIXED_POINT: the written metric equals
                       a numpy re-implementation of CSolver::ComputeMetric from the written Hessians (Lp metric per
                       sensor, scaling, intersection, global factor with bounds; no corner / BL metric): 1e-5
                       (double), 1e-4 (Float32); with 2 sensors the variants without a sensor or without the
                       per-sensor scaling must differ by > 1e-3. Scope: Hessian -> metric, not the sensors
  window_average       WINDOW_AVERAGE / FIXED_POINT (accepted solve): n H_end - sum of |H| of the earlier steps of the
                       window is finite and PSD within 1e-5 relative tolerance, for every sensor (H_end is the mean
                       |H|). Its volume-weighted Frobenius norm / that of the previous instantaneous |H| must be in
                       [1/3, 3] (vortex_wa measures 0.52: its vortex decays fast); missing evidence or zero norms fail.
                       Applied to every window; PSD and the norm band alone cannot detect stale or omitted samples
  window_control_run   independent first-window control: exit 0 and "Exit Success"; missing first-window mesh fails
  window_average_exact WINDOW_AVERAGE / FIXED_POINT, first window only: for every sensor, the loop's reconstructed
                       final sample n H_end - sum_{k<e} |H_k| equals |H_e| from the independent control (absolute
                       eigenvalues), within a volume-weighted relative Frobenius error <= 1e-4 (Float32 VTUs).
                       Also reports and gates the largest earlier-step error of the signed loop vs control Hessians
                       at <= 1e-4: a trajectory mismatch fails clearly. Weights are the first mesh's median-dual
                       volumes; each error is sqrt(sum V_i ||A_i-B_i||_F^2 / sum V_i ||B_i||_F^2), with the control
                       as B. Missing control files/Hessians, nonfinite evidence or points off the first mesh fail.
                       Selftest: the correct mean passes; duplicating the penultimate sample, omitting the final
                       sample with n-1 normalization, and using the last step only each fail this exact gate
  predict_lookahead    PREDICT: the density centre of the written (predicted) metric is ahead of the instantaneous
                       metric of the window end (numpy, from its Hessians) by >= 0.25 x speed x horizon along the flow,
                       and its density at the current feature is >= 0.25 of the instantaneous one
  fixed_boundary_bitwise  ADAP_SURFACE= NO: every marker's faces (as coordinate tuples) and the boundary points are
                       bitwise those of the input (17-digit output)
  free_boundary_geometry  ADAP_SURFACE= YES, against the ORIGINAL input (drift over cycles counts): flat patches: every
                       point and face centroid on the input line / plane within 1e-12 of the size and inside the patch
                       box (points on a joint of two patches: 2 ADAP_HAUSD, MMG bows ridges as curves); curved
                       patches: distance output -> input and input -> output <= 2 ADAP_HAUSD + input sagitta
  analytic_surface     report: distance to the analytic bump (2D / 3D)
  corners              input corners (2D: points of >= 2 markers or turns > ADAP_ANGLE; 3D: points of >= 3 markers)
                       kept within 1e-12 of the size with the same markers; no new corner
  seams                3D: points of 2 markers on the input seams (1e-12 of the size fixed, 2 ADAP_HAUSD free)
  bl_coverage          each BL marker: positive eligible face area and eligible / total area >= 50 %; faces touching
                       non-BL markers excluded, eligible wall and BL ridge faces counted
  bl_cell_height       wall-adjacent cell height d |K| / |face| in [0.5, 2] h0 for >= 80 % of the eligible wall area,
                       evaluated separately per BL marker
  bl_ridge_cell_height same for eligible faces at a ridge of two BL walls, >= 60 % per marker;
                       bump3d_fixed_bl also requires nonempty eligible ridge evidence
  restart_bitwise      unsteady: restart from mesh_out_<ADAP_FREQ>.su2 with the rewritten restart files of the two
                       steps before it; the following restart files (across the next remesh, which the restart run
                       must do) are bitwise those of the uninterrupted run
  mpi_vs_np1           np > 1: points of the last mesh within 5 % of np 1 (mesh hashes reported; the np 1 run of the
                       case must be in OUT)
  mpi_metric_vs_np1    np > 1: metric of the first remesh (same input mesh) within 1e-8 (double) / 1e-5 (Float32) of
                       the largest eigenvalue of np 1
XFAIL requires run=PASS and a nonempty set of failing gates wholly contained in the case's expected gates above.
Any unrelated failure (including execution, metric files, restart or MPI) is FAIL; no failures is PASS.
XFAIL does not fail the run. Expected gate sets are written as sorted lists, with a separate reason in results.json.
