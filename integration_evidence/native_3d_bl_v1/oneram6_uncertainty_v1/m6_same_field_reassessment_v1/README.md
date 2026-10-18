# Same-original-field M6 reassessment on the saved finer partition

Full Goal3 remains ACTIVE/incomplete. Production code and all mesh/metric/geometry/
height/transfer gates are unchanged. This tests the user's hypothesis that actual
refinement may ease complexity estimation, using the existing production
`ReassessComplexity` and `CNativeComplexity3D` facilities, with no new integration
mechanism, CFD advance, Hessian recovery, normalization or donor refit.

## Result and decision

| Integration proposal | Samples | Estimate | Heuristic indicator | Native assessment seconds |
|---|---:|---:|---:|---:|
| Original303359 tetrahedra |8192|312124.65|51830.47|4.779|
| Saved private541027 tetrahedra |8192|285516.06|31476.26|8.154|
| Existing wall-focused proposal, seed9919 |32768|298571.52|5765.29|12.727 including cold preparation|

The wall proposal uses four times as many samples, so its tighter indicator is
not an equal-work comparison of the proposal kernels.

The finer partition reduces the indicator39.27% (ratio.6073), but costs70.64% more
in this one contended correctness replay. Deviations from the different-proposal
estimate are+13553.13 and-13055.46. That small difference does not establish better
accuracy: the comparison estimate is itself uncertain. Both lie within the sum of
their heuristic indicators. All four ranks agree exactly on each global estimate,
indicator, sample count and selected profile. This is MPI4 agreement, not a new
MPI1/2 invariance campaign or calibrated coverage probability.

The hypothesis is therefore PARTIALLY supported for this private finer mesh:
variance indication improves; accuracy and cheaper work are unproven. Actual
adaptation is not an assured quadrature cure. Keep the existing wall-localized
sensor-plus-BL decomposition for difficult initialization; do not add another
subdivision/limit ladder or tighten normalization beneath the uncertainty.
No production change or new accuracy default is justified by this experiment.

The wall-focused estimate is0.4762% below the requested300k budget, with~1.93%
heuristic uncertainty. It retains~287870 BL correction plus sensor midpoint~10701,
so BL accounts for~96.4% of the estimate. No missing BL mass is identified by this
finite comparison, but rare unsampled mass and background uncertainty are not
proven bounded. A metric complexity is not an exact tetrahedron count.

## Immutable field and scope

All assessments use the SAME original selected owner-UPPER6 sensor file
`selected_sensor.bin`, immutable original303359 tetrahedral donor geometry,
actual-query geometric BL, parameter.875, core.003215747252591252 and original
support/fade. The independent wall proposal mirrors those exact six components
and checks every imported donor value before assessment. It does not use the
historical different RAW9 field as a reference or rescale clipped donors.
Original donor fingerprints and profile versions remain unchanged. The candidate
mesh supplies sampling probabilities only, never replacement metric samples.

The saved finer mesh is REJECTED by complete construction. Its exact terminal
owner pieces come from
[the preserved M6 state](../cavity_operator_replay_v1/evolved_volume_v1/README.md).
The exact-facet crease obstruction is unchanged. This is not an accepted mesh,
production stage correction, flow transfer, restart or full-goal success.

Independent finite domain screens pass:541027unique cells, positive long-double
volumes (minimum3.07e-15), total volume agrees with coarse to1.64e-19 relative,
face incidence at most2, and all48954open faces have unique physical bindings.
These are useful integration-domain screens, not a global no-overlap, geometry,
quality or full-metric acceptance certificate.

## Work and resource accounting

One MPI4 comparison reuses one original donor index (build~1.02s on rank0); coarse
then finer assessment use311551 and549219 uniquely owned original-point queries
respectively: one centroid per cell plus8192 samples. Source interpolation and
actual-point composition remain authoritative. Field rebuild counters during
assessment are zero. Per-rank phase timings/query counts/communication/byte models
are in `coarse_rank*.json`, `fine_rank*.json`, `setup_rank*.json`.

The separate wall proposal uses32768disjoint-seed draws. Rank0 cold preparation
12.511s, assessment.189s, including background.163s/correction.00175s and recorded
reduction time. It retains~28.1MB locally and records59.55M support candidates.
These clocks are nested/local; do not add them to form a global critical path.

Launcher times24.712s for the two assessments and23.530s for the wall proposal
include driver initialization/restart/import/finalization. GNU-time maximum RSS
is550160/595364KiB respectively; these are process high-water observations, not
simultaneous aggregate MPI peaks. Largest modeled native bytes~811.6MB/rank,
CFD/allocator/MPI storage separate. Swaps0. One heavy job at a time, nice19,
threads1, affinity0--3, local MPI4. No clean scaling/event-affordability conclusion.
Each candidate phase has90s boundary-checked work time; outer launch240s. Neither
limit was reached. Complete independent stage receipts survive interruption.

## Reproduction and preservation

`run.py` and `wall_reference_v1/run.py` preserve attempts and refuse overwriting
saved receipts. Source/build/archive/binary/config/sensor/reference/input hashes and
executed source capsules are retained. `audit.py` verifies source provenance,
rank reductions and the independent partition screens. `verify.py` checks sealed
results without native/CFD reruns. Restorable raw owner inputs and compressed
executables stay local, excluded from Git; exact compressed owner originals are
already tracked in the linked state. Use `restore_inputs.py` before a deliberate
new versioned replay. Symlinks resolve inside this repository.

More cell subdivision had already shown nonmonotone accuracy/cost on the saved
88/157/329 difficult-cell references. This result supplies the missing actual-M6
private-refinement comparison; it does not invalidate their preserved failures or
complete the still-open global-error calibration/allocation requirement.
