# Adaptive integration checkpoint — incomplete

Latest `validation.json` is FAIL: seven stages pass and hidden support rejects.
Actual complexity and unchanged sensor-only donor tensors pass on MPI1/2/4.
This is metric-construction correctness, not full BL remeshing or CFD qualification.

The callback evaluates the actual trial's bounded/graded original-P1 sensor and
geometric BL with its frozen core. Equal donor tensors/core reuse a rule within the
immutable wall/cell geometry lifetime. New timing and rebuild/reuse counters expose
this part of metric work. No full performance or scaling claim is supported.

The experimental rule uses positive vertex-based tetrahedral decomposition from
the existing clipper's Moments, distance-band clipping on locally affine probes,
largest-error angular subdivision and radial order/subdivision checks. Degree5
angular points are retained and compared with degree6; radial3/5 points are checked
with5/7. Summed estimated angular and radial error controls refinement. Metric,
sensor interpolation/finer demands/fade and independent5e-4 accuracy gates remain
unchanged, along with65536returned points,16384refinement work and depth32.
Neither sampled affine checks nor embedded error estimates are general rigorous
geometry/integration certificates.

Current hidden-support failure has retained65411 points,896generated panels and
105initial cones; estimated error0.002544. The printed density2.471511 is PARTIAL:
the selected parent is removed while children are still being built. It cannot
establish accuracy against the independent parallel-body oracle2.4775664410.
The stronger isotropic100 sensor oracle is in the same test but has not been
reached after this initial rejection. Production BL guard stays intact.

Saved experiments distinguish local/global angular allocation, radial refinement,
affine routing, output/comparison admission, vertex-based decomposition, explicit
phase diagnosis, volume-weighted error, comparison-order and local-order attempts.
Ten preceding source states are preserved as `adaptive_*_failed.patch` plus base
commit, patch/source hashes and historical validation/logs. Run
`python3 integration_evidence/native_3d_bl_v1/feature_partition_v1/verify_experiment_sources.py`
to reconstruct them in disposable /tmp files; source_reconstruction.log passes all
ten. This validates source replay only; old binaries are not archived.

Run `python3 integration_evidence/native_3d_bl_v1/feature_partition_v1/run.py`
after the existing test target is current. Eight independently saved stages verify
full provenance and per-rank test records. Thread count1, nice19 and sequential
MPI stages limit local contention. Reuse requires matching binary/object/source/
helper/command/environment/log hashes. No CFD timestep or cluster submission.

Next work needs a substantive geometric integration change, not cap/order sweeps
or acceptance using the known answer. Full construction/removal, curved walls,
composed gradation, transfer/RANS/restart/output and lifecycle cost remain pending.
