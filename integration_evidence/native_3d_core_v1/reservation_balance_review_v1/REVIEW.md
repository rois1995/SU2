# Partition balance versus reservation bias — 2026-10-12

Repository: /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated.
Branch: codex/native-3d-transport-accounting; starting checkpoint
c407d8b9a40e8aa6816b779968482d206a51cdea. Production C++ is unchanged.
Goal 1 remains active; Goal 2 is not started.

## Saved evidence

Three source-pinned successful box-prefilter N4/M2 cases are reparsed:
box_filter_n4_n4_m2_v1, box_filter_a2_direction_n4_n4_m2_v1 and
box_filter_thin_h20_n4_n4_m2_v1. All original validation/artifact/audit hashes
are verified. These are the earlier c1bdef2a binary's actual logs, not new runs
of the current diagnostic binary. Current partition/directory/donor/remesher
source hashes match that qualified build; diagnostic-only engine changes remain
separately documented in COMPLETION_DIAGNOSTIC_VALIDATION.md.

The specialized reviewer verifies each rank's explicit ENVELOPE cycle sequence
0/1/2 and partition/refine/coarsen stage order before associating the three stage
ordinals. Generic profile event_index still denotes its within-stage ordinal,
not a universally shared cycle identifier. Cell/work totals before and after
partition agree, and independent audits already certify the numerical results.

The strongest example is aspect2's second-event coarsening. Its preceding
refinement makes no edits, so the initial partition load remains the starting
coarsening load. These recorded counts describe work, not calibrated CPU cost:

| Measure | Worker 0 | Worker 1 |
|---|---:|---:|
| Partition cells | 197 | 205 |
| Partition proxy weight | 959 | 979 |
| ADT donor candidates examined by source owner | 649111 | 644747 |
| Private attempts | 130 | 64 |
| Accepted / rejected attempts | 55 / 75 | 46 / 18 |
| Reservation vetoes | 0 | 92 |
| Received donor payloads at requester | 39323 | 15163 |
| Fraction of reserved-or-vetoed proposals vetoed | 0 | 0.589744 |
| Accepted fraction of private attempts | 0.423077 | 0.71875 |

Proxy max/mean is 1.01032 among the two active workers; including the two idle CFD
ranks would instead produce 2.02064. Owner candidate max/mean is about 1.00337.
Thus this example is not evidence that ParMETIS failed to balance the supplied
weights. It does show substantial unequal access to private reconstruction work.

Directory::Reserve selects the smallest caller rank for every shared vertex.
The lowest active rank cannot lose a conflicting reservation under this order;
higher ranks can repeatedly lose interface candidates. Here worker 1 loses 92 of 156
rounds. The standard and H20 cases also show worker 0 with no conflicts and worker 1
with 87 conflicts in their second coarsening. Boundary/marker constraints and
candidate feasibility still matter; equal weights need not imply equal proposals.

Donor owner candidate scans and requester payloads are different counters:
CNativeDonor3D.cpp increments index_candidates during owner ADT search and payloads
on received donor replies. Both must be retained. Similar candidate counts do not
prove similar predicate costs: exact fallback, touching geometry and filtering
paths differ. Nor does a conflict necessarily consume equal time in every stage.
No pure MPI waiting, calibrated rank CPU imbalance or net speedup follows from
these contended correctness logs.

## Reusable profiling improvement

profile_summary.summarize now adds:

- partition_active_load: cell/weight distributions over M active remeshing ranks,
  retaining the original N-rank distributions separately. Idle CFD ranks cannot
  masquerade as a partition imbalance. Unexecuted all-zero partitions do not get
  a spurious active-balance claim; inactive retained cells/work reject.
- proposal_outcomes: per active worker, reservation-veto fraction, accepted
  fraction of attempts, and unfinished private attempts. Counters must be finite,
  integral, nonnegative and bounded by attempts/rounds. Completed engines require
  all attempted proposals to have accepted/rejected outcomes. Failed-stage veto
  counts may include admission refusals; these fractions are not MPI wait.

No additional C++ timers/messages/configs or geometry/metric/state changes are
introduced. Generic event stages remain separate. The unchanged descriptive
scope string preserves older frozen runtime summaries. Comparison analysis
rechecks every recorded value against raw logs; it permits only absence of these
two new derived profile keys in legacy summaries and outputs recomputed fields.
Changed original counts, samples, missing core measures or wrong present derived
ratios still reject. This is compatibility with old derived schemas, not a
relaxation of accepted-work or source/artifact checks.

The lightweight core 34 tests and comparison 15 tests pass. Negative controls cover
idle-rank contamination, counter bounds, incomplete attempts, legacy schema,
modified accepted work/conflicts and tampered derived ratios. Two intermediate
comparison-check failures involved simplified mock schemas (empty/missing
profiles); sources/stderr are preserved in failed_packaging_v1/v2. Numerical
meshes and simulations did not fail. The final checker explicitly rejects
malformed runtime dictionaries and retains complete runtime equality.

analyze.py reparses all three actual logs through the updated profile machinery,
including full phase/transport closure, and retains 18 operation events in
analysis.json. Commands/tools/input receipts/outputs are pinned in validation.json.
No SU2/MPI/cluster or local performance run occurred; all work is terminal.

## Next decision

Measure this reservation imbalance on repeated cluster runs before tuning weights
or choosing mid-remesh repartitioning. A deterministic rotating reservation order
is a plausible bounded experiment, because the existing lowest-rank preference
is structural. It must preserve a globally consistent order, nonconflicting
complete dependencies and atomic publication; changed meshes need fresh full
independent certification, not the fixed-work certificate of the donor optimization.
Fairer access need not reduce round count or total adaptation time.

Do not change production reservation policy in this checkpoint. The already
prepared fixed-work donor comparison remains usable with the current source.
Changing policy now would mix scheduling/mesh-trajectory effects with the donor
performance question. Fresh bundles include the new analysis; old frozen bundles
remain immutable. No cluster submission is made tonight.

Repeated total-cost/scaling/overhead/rank/weight/wait/traffic/memory calibration,
practical limits and the default 512 completion bound still gate full Goal 1.
This review supplies a more specific balance hypothesis, not a calibrated cost
model, completed performance gate, curved geometry, production 3D BL or affordability.
