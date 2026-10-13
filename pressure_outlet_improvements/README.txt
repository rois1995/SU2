Pressure-outlet validation evidence.

The source PR is independent of HLLC/moving-wall/supersonic-outlet fixes.
Cases use inviscid fixed-wall ducts and smooth converging-diverging nozzles.
Nozzle h(x)=0.1-0.02*sin(pi*x)^2, throat/inlet area ratio 0.8 and exit/throat ratio 1.25.
Air and MDM/Peng-Robinson, first-order HLLC, implicit FGMRES/ILU, CFL=20, two MPI ranks.
Nozzle density/velocity inlet, initial Mach=0.9 and outlet startup pressure=1000 Pa.
Final nozzle exits are supersonic, so 1000 Pa is not imposed on the final solution.

Reproduce a case with Python 3 and a normal MPI-enabled SU2_CFD build:
python3 run_cases.py --binary /absolute/path/to/SU2_CFD air_nozzle_100x8
For hybrid execution, use an OpenMP-enabled binary and add --threads 2.
The runner writes result.json and logs; restart/surface outputs can be generated from the case.

Cases named *_develop are for the develop baseline. Use that binary for those runs.
Cases named *_hybrid use two ranks and two threads.
Reservoir cases use prescribed inlet total pressure and temperature.
reservoir_baseline: pressure-outlet fix with the existing Peng-Robinson h/s inversion.
reservoir_tighter_hs: same outlet with an experimental tighter absolute entropy tolerance (isolation check).
reservoir_relative_hs: same outlet plus the proposed relative-volume stopping criterion.

reservoir_convergence.png/svg demonstrates the separate EOS precision effect, not an additional improvement supplied by the outlet PR alone.
peng_robinson_hs_precision.patch is the separate local EOS code and regression, supplied to reproduce that comparison.
The existing characteristic_outlet sibling folder contains the develop/outlet duct comparison plot and prior four-case comparison.
See summary.json for exact source/binary versions, results, verification scope and limitations.
