# Metric integration and temporal feature prediction

The metric checkpoint `5cb87667c20f7c590fab00aa373c7c056a2daca1` merges the native performance branch (production baseline `e6995fbff565955a5677ffcbcc66af9acf96a8f1`, assessment parent `d8fc473a00ad9d76a2f959b5038d8cf5e9c312e0`) with metric-robustness `62850446143f5ac8de433213b68e51c2116bc674`. The incoming findings and v3 accepted/rejected experiments are preserved. Incoming receipts are references; `validation.json` records tests of the combined executable.

## Metric changes reviewed

Stable metric intersection preserves the generalized maximum. Adaptation WLS ordering and primal compensated normalization improve MPI reproducibility. Supported QR wall enrichment and mirror-even symmetry retain rank/conditioning checks and lower-order fallbacks. GG P1 volume-star recovery is selected by adaptation callers; ordinary CFD gradients retain their defaults. Invalid, incomplete or mixed stars retain whole-star fallback. Cell geometry work is shared within a GG pass without a permanent cache or added communication. Restart names no longer use the overflowing fixed buffer.

The steady adaptation loop consumes Postprocess's completed metric once; explicit public remeshing still refreshes it. Native complexity solves retain safeguarded brackets and endpoint refresh while reusing accepted fields and the previous scale. Active nodal sensor gradation retains synchronous old tensors, halo communication, global-ID ordering, retries of clipped corrections and global demand-based stopping. The round cap is a safeguard, not a convergence certificate.

Sensor-only immutable donor P1 interpolation and geometric BL evaluation at every actual query, including private and remote candidates, remain intact. Complexity and remeshing use the same geometric composition and thin-region integration. Finer sensor demands, coupling, original-wall reference and fade remain. Noise stays zero. No experimental hard-normal reset or propagation of composed wall tensors was introduced. The RAE auditor conflict preserves current CSV/transport/Euler capabilities and audits against the original reference wall.

Nodal sensor transport, between-node P1 transport and composed sensor-plus-BL transport are distinct properties. The 18 frozen RAE fields pass their nodal transport and MPI checks. Their composed-field audit remains diagnostic and records violations (maximum ratio approximately 99 in one control). This integration does not declare combined-field gradation solved or relax mesh/transport contracts.

## Hessian cadence

Steady Postprocess runs after an iteration block, and unsteady Postprocess after a physical step, rather than every inner CFD iteration. WINDOW_AVERAGE defines the mean of absolute Hessians over every physical step. Existing MMG FIXED_POINT uses this mean for each resolved window and an end-window convergence diagnostic. Skipping those samples or taking a Hessian of the mean solution would change the target. Full bounded/graded metric construction occurs at the window end.

PREDICT exits before derivative construction except at its configured snapshot steps. The new `ADAP_PREDICT_SNAPSHOTS` selects the count (default two). `ADAP_PREDICT_SEPARATION` selects fixed cadence backward from the window's final step; zero computes the largest cadence that fits. A restart uses the remaining scheduled samples without shifting cadence. Three or more samples permit acceleration; two retain constant motion, one retains the final metric without motion. Terminal-window metrics remain available to configured output even when no subsequent replacement occurs.

## Temporal feature model

The history fits feature motion, rather than extrapolating metric tensor entries. Each earlier invariant is matched to the common final frame. For constant feature acceleration, interval-average velocities sample temporal midpoints; centered least squares estimates final velocity and acceleration. `ADAP_PREDICT_TEMPORAL_FILTER` applies a dimensionless ridge penalty to acceleration (default one, zero disables the penalty). The mean is preserved while acceleration is shrunk by `1/(1+filter)`.

The fit is checked against the original offset-corrected saved invariants. Acceleration is retained only when it improves on the best constant candidate (latest pair, mean motion or zero). This is an in-sample safeguard, not a guarantee of future prediction accuracy. When acceleration is active, each future endpoint is traced back independently through the time-dependent field; gradient congruence preserves positive definiteness before metric intersection and bounds. Zero acceleration follows the previous autonomous path.

Extra samples add storage, derivative construction and root optical-flow solves. Accelerating trajectories also cost more than constant trajectories. The legacy two-snapshot path is retained; no speedup is implied by increasing the count. Motion units are displacement per CFD step and acceleration per CFD step squared.

Native support now admits PREDICT alongside WINDOW_AVERAGE for its existing static primal 2D triangle, first-/second-order dual-time path. FIXED_POINT remains guarded for native; its existing compatibility smoke uses MMG.

## Validation and limits

The metric checkpoint has 41 successful stages: MPI1/2/4 suites, GG/WLS/QR frozen RAE fields at complexities 12000 and 60000, AD syntax checks, actual steady RAE remeshing, frozen Euler-to-BL / BL-to-Euler, native Euler CGNS and RANS window-average smokes, and MMG PREDICT/FIXED_POINT compatibility controls. Independent audits check original geometry, quality/length/height, saved target fields, positivity and both BDF histories where available. MMG fixed-point discarded-window transfer is not independently certified by the retained output.

The apparent MPI4 stall was reproduced under a two-CPU affinity mask. Traces showed matching collective prefixes and ongoing progress; moving the same processes to four available CPUs allowed completion. Subsequent complete MPI4 suites passed without a production communicator change. Failed attempts and the affinity correction are retained in `mpi_timeout_resolution.json` and referenced evidence. This is a correctness diagnosis, not a performance result or general proof against deadlocks.

The complete 302-step prediction ABI rebuild passed (`build_predict_v1/evidence.json`). The first local serial metric suite was stopped at the user's request before completion; no new prediction CFD cases ran. Its interrupted log and original controller are preserved. `predict_history_v1/validation.json` explicitly records STOPPED_BY_USER, not a numerical failure. Runtime validation of the new history/filter and native PREDICT support remains pending the SGE job in `../native_cluster_metric_comparison_v1/CORRECTNESS_README.md`. The metric checkpoint does not substitute for these tests.

The original sixteen downloaded cluster cases passed independent audits (`../native_cluster_analysis_v1/ASSESSMENT.md`). Overlapping jobs, one repeat and shared hardware make those descriptive evidence rather than isolated performance controls. No local performance campaign was run. The next comparison uses separate complete baseline/candidate checkouts, unchanged inputs, matched builds, chained SGE holds and reversed version order in even repetitions, with the usual shared-node allocation. See `../native_cluster_metric_comparison_v1/README.md`. Its current workload uses WINDOW_AVERAGE and does not measure the additional prediction-history cost.

Native 3D remeshing, native FIXED_POINT, general mixed/periodic QR, differentiated runtime/adjoints, full hybrid scaling and converged aerodynamic accuracy remain outside this validation. Shared-node timings remain approximate. Complete adaptation costs include working partition/migration, returned CFD partition, transfer and output; nested timings and independent rank maxima must not be added.
