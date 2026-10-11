# Native3D larger practical-limit campaign preparation

Working repository: /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated.
Branch: codex/native-3d-core; predecessor6db495a38bc01a2f13e9e51881a8802d110be3b0.
Status: orchestration CHECKED; C++ fixture and numerical adaptation NOT VALIDATED.
Published implementation/evidence: 6565e5575e5b4e00f4f938434d05040227a1d4a7; remote branch SHA verified.

The existing envelope fixture and local runner now admit dyadic n8. This wedge
has405 points,1536 tetrahedra and512 original surface triangles, above the former
global256-facet cap. Scale sequence64/16/64 follows the existing n*n rule, with
unchanged aspect1.5, spatial rotation/variation0.125, original-P1 sensor field,
planar junction, quality0.20/sliver0.05/length1.8/default512 rounds/noise0.
No production operator, sensor, BL, MMG or2D implementation changes here.

The closed SGE package retains its default seven serialized jobs. Optional
submit.py --limits adds one final four-slot campaign, gated by same-bundle
correctness and N4 lifecycle. It runs N1M1,N2M2,N4M2,N4M4 sequentially and stops
at the first failed native test or independent audit. Successful cases retain
three actual refinement/coarsening/refinement events, geometry/metric/current+BDF
states and SU2/CGNS reload audits. These remain proposed runs, not results.

Parameters and relevant envelope/thread environment are now written before MPI
launch for all envelope runs. Failed-case stages/logs/raw files remain available
without compression; subsequent cases in the limits job are not launched.
Successful large JSON is verified-gzip packed as before. Result analysis reports
single-pilot elapsed time, accepted edits/rate, rank max/mean, requested bound and
lifetime RSS, without three-repeat medians or cross-partition fixed-work claims.
Failed campaigns are refused by the summary; inspect their retained failure
reason and raw case folder.

package_checks_v4 retains an initial lightweight check failure: its synthetic
bundle lacked a copied recipe file, so result/package was never created.
The fixture was corrected to contain a frozen recipe, matching actual preparation.
package_checks_v5 passes all11 fake-file/mock-process/mock-qsub/synthetic-summary
checks in0.899s; Python and SGE shell syntax and diff checks pass. It verifies the
four N/M case order, unchanged rounds and thread count, independent per-event
single-pilot summaries, failure provenance/no-compression/stop behavior, and the
optional eighth job's scheduling/result gates. Source/artifact hashes are retained.
No numerical C++ build/test, MPI/CFD run, bundle freeze or real qsub was performed.

Host preflight: eight CPUs, load12.51/12.38/12.87, sampled idle0-1%, foreign FSI and
conservation workers active; local heavy jobs deferred. Root filesystem about1.1GiB
free;4TB workspace77GiB. Current native-prefixed /tmp scratch totals9,306,112 bytes.
The9.3GiB /tmp/su2-metric-build belongs to the other agent's metric-robustness
worktree;2.2GiB wall-fixes build belongs to another session. No cleanup or foreign
process changes. Check scratch used an explicitly owned4TB subfolder, now empty.

Next remains contention-gated configured reference_admission_regression_v1
(full68-object driver,69 selected cases per MPI1/2/4 rank and serialMMG), the frozen
two n4 fixed-work/actual persisted-restart cases and independent qualification.
Only after qualification freeze a current closed cluster bundle; launch when
the user requests. Last validated production remains1fafe9e83c. C++ reference
admission draft d0d3fdaa remains UNVALIDATED. Larger size/rank results, complete
traffic/cache/privateCPU/wait/memory calibration and detail overhead remain open.
FullGoal1 stays ACTIVE; no complete3DBL/curved/unsteady/PDE/affordability claim.
