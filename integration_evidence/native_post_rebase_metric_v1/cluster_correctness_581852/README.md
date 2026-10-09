# Cluster correctness job 581852 assessment

The metric and native suites pass on MPI1/2/4, including the corrected orientation reference, multi-frame acceleration/filtering and MPI prediction transfer tests. All five expected-rejection guards pass. The native default-two N4/M3 and four-frame donor-fixture N1/M1 CFD cases pass their independent audits, reproduced locally from saved output without running a solver.

The next case, four_n1m1, completes both remeshes but stops at step 11 with `Maximum time reached (MAX_TIME = 1s)`. Its configuration requests 15 steps at 0.1s and omits MAX_TIME. CConfig defaults to 1.0; CSinglezoneDriver stops on either physical time or iteration count. Expected snapshots 12, 13 and 14 are therefore missing. The original aggregate FAIL is correct and is preserved, even though every executed solver subprocess exits successfully.

The two replacements in that interrupted case were audited using a temporary copy with TIME_ITER=12 to select the available last restart. Both satisfy q >= 0.18, L <= 1.8, original-connectivity donor metric evaluation, reference sidecars, positive density/pressure, and conservation of both BDF states (maximum relative defect 2.77e-15). This is a partial-run audit, not certification of the requested 15-step test. The copied configuration hash is recorded; the original config and downloaded files remain unchanged.

All logged sensor and composed nodal transport diagnostics in the three executed Euler cases report zero directed edges above 1+1e-5. This does not establish combined sensor-plus-BL gradation. These short correctness runs provide no performance comparison or converged flow-accuracy evidence.

The fix sets MAX_TIME=100.0 in all seven fixtures, allowing TIME_ITER to govern their requested duration, and adds the actual/expected steps and stop-limit hint to the assertion diagnostic. Production code and frozen performance pins are unchanged. The remaining N2/M1, CGNS N4/M3, straight-wall SA RANS and partial-restart cases were not reached. Pull, reprepare the checkpoint and rerun the same sequential SGE gate. Require aggregate PASS before proceeding to performance comparisons.

Raw validation, checkpoint, failure log/config, launcher traceback, completed-case audits, and the explicitly scoped partial audit accompany assessment.json. Full downloaded grids and snapshots remain in ClusterResults/predict_correctness_581852/.
