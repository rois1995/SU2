# Opt-in budget-aware BL policy: qualified checkpoint

Full Goal3 remains ACTIVE. This implements and qualifies a controlled native3D
slice, not general accepted M6 construction or affordable unsteady adaptation.
`summary.json` contains verified receipts, independent audits and raw phase rows.
`python3 report.py` rechecks them without running SU2. It intentionally refuses
current-state reuse after relevant source/build/binary changes; this evidence
belongs to checkpoint58214e08bd, not automatic qualification of later code.

## Behavior

Default `ADAP_BL_BUDGET= NO` retains strict behavior. YES requires explicitly chosen
`ADAP_BL_PROTECT_DISTANCE` (at least every first height) and
`ADAP_BL_OUTER_GROWTH_MAX` (at least every original growth). These are accuracy
permissions, not values an automatic budget optimizer can decide for the user.
Automatic selection currently supports native3D METRIC; 2D/MMG opt-in rejects.

One global lambda changes only normal spacing outside the protected band. It
anchors continuously to the original spacing there, preserving tangents, geometry,
first-height requirements, thickness/support, original fade and finer sensor
intersection. The scan has at most9 ordered profiles at1/8 spacing, with no assumed
monotonicity. It reports least sampled relaxation, not a continuous optimum.
An earlier uncertain profile stops selection, rather than forcing more relaxation.

Each trial uses the complete geometric sensor+BL composition and the original
unclipped sensor family. Owned contributions and uncertainty reduce collectively;
profile/version broadcasts agree. No additional whole-mesh gather. Original donor
support stays; profile-dependent wall samples/rules/workspaces are versioned and
invalidated. Profiles freeze through local edits, N/M migration and correction.
No adapted-node refit, clipped-donor scaling or geometry/height/q/J/edge/transfer
waiver. Explicit bounded stages handle normalization; clearly over-budget outcomes
at the original-family lower scale reject before publication.

An uncertain coarsest estimate now stops normalization provisionally at that scale.
The existing retained estimate cannot resolve a smaller correction. This status
requires reassessment of the same original field; it is not an exact root.

## What passed

`qualification_v7`: primitive4/256 and strict compatibility22/2056 assertions,
serial/MPI4 uncertain-budget controls (one trial at original coarsest scale), and
complete initialized RANS/SA current+BDF workflows in serial and N4/M2.
`qualification_v4` additionally retains the earlier MPI2 complete workflow.
V7 enabled checks include independent full-metric geometry/q/J/edge checks,
prescribed height, P1 sensor transport, all-field projection/conservation, atomic
limited-work rejection, SU2+CGNS reload and actual current+BDF restart with SA fields.
Curved/interacting/finer/fade/stale-cache behavior is covered at primitive level.

| Controlled budget | Lambda/version | Cells | Independent composed mass | Budget discrepancy |
|---|---|---|---|---|
|3|.875 /8|16|2.95498841845|-1.50039%|
|20|0 /1|240|19.9993054819|-0.00347259%|

Rank choices preserve profile/version/selected sensor. Valid topology and final
sampled estimates may differ. Observed estimate errors are covered by their
indicators here; that is not a bound. First-height and transported-sensor residuals
are zero in these fixtures; current+BDF integral defects <=2e-16, projection <=2.7e-16,
restart flow/BDF defects zero. These are initialized-state tests, not converged CFD.
Inspect `qualification_v7/enabled_serial/bl_budget_3_m1_accepted.su2`, its budget20
counterpart and the `_written_CGNS.cgns` files; original mesh files remain intact.

## Saved-state M6: selection is separate from construction

Explicit EXPERIMENTAL permissions: protected distance2e-6, maximum outer growth2,
original thickness. No claim that these permissions provide acceptable viscous
accuracy. Preprocess and ComputeMetric recover the actual unclipped original family
from the saved CFD state, without Solve/CFD advance. It is distinct from historical
RAW9/UPPER6 archives. The HMAX100 coarsest sensor is constant and background mass2.08846.

| Budget | Outcome | Evidence |
|---|---|---|
|100000|Estimated incompatible across9 sampled profiles; final260488.18 +/-10780.29|MPI2/4 trial costs/indicators agree to2.04e-13 relative|
|300000|Provisional integration-limited lambda.75/version7;308423.48 +/-12078.10|Original coarsest field, frozen, flow/SA/reference unchanged|
|1000|Estimated protected-demand conflict|Mandatory lower contribution17173.58 +/-6184.98 itself exceeds budget|

At100k the mandatory lower estimate alone does NOT exceed budget. It is the
permitted sampled profiles that conflict. All indicators are heuristic, not exact
infeasibility proofs. Complexity is not a tetrahedron count.

Held-out saved clouds with the same coarse endpoint and selected profile give
314146.21 +/-12152.84 and312539.01 +/-11851.88. They reuse native geometry samples
but independently reconstruct density and use separate seeds. Both lower heuristic
endpoints slightly exceed300k. This reinforces the need for reassessment; it does
not establish300k feasibility or provide a rigorous continuum reference.

No new M6 construction attempt or accepted M6 mesh is claimed. The existing rejected
construction and strict uncovered-source-segment diagnosis remain preserved.

## Work, cost and remaining bottlenecks

The previous300k run selected the same lambda then spent933 gradation sweeps,
35.20M visits and86.33s gradation, still returning an uncertain finer-scale estimate.
The uncertainty-stage change uses7 sweeps/429310 visits,0.431s gradation, zero final
sensor-gradation residuals. Metric time93.477->6.552s; launcher102.697->16.083s.
The provisional sensor scale CHANGES; this is removed unresolved trial work, not a
same-field arithmetic/remeshing speedup or lifecycle scaling result.

Current M6 clocks: cold geometry/donor preparation3.505s, reused background.989s,
correction.0243s, reductions.000116s, selection4.993s (overlapping/nested counters).
Retained integration maximum15,463,712 bytes/rank; original family25,906,568 bytes on
root. Across trials:46.72M background determinants,37282 correction queries,
31956 profile geometry refresh queries, zero trial donor searches. The successful MPI launcher records maximum RSS596236KiB, a process value,
not an aggregate MPI peak. Failed MPI aborts can omit child resource accounting;
their small launcher RSS/CPU numbers must not be used as whole-job measurements.

Controlled serial rank0 backend clocks (include repartition/migration/return):

| Budget | Metric build | Native backend | Reassessment within backend | Replace/rebuild/transfer |
|---|---|---|---|---|
|3|8.375s|.396s|.0951s|.00196s|
|20|2.681s|4.669s|.0448s|.00995s|

The first case includes a bounded correction; its replayed metric call is nested
in backend time. Geometry-return clocks are nested in replace. Full two-case native
launcher16.70s serial /17.29s N4M2 includes output, reload/restart and atomic-rejection
controls, not one isolated event. Campaign Python wrapper totals18.86/19.38s include
additional orchestration; independent Python audits are timed separately. Raw per-rank backend/detail/stage rows in summary retain
partition, migration, collective bytes/calls, memory and transfer sections. Output
is included in campaign elapsed time, not independently isolated per event.

Planar strict initialization remains expensive:~3.7M density calls/90 rules on the
six-cell budget3 input. M6 retained preparation and repeated background density are
still significant. These contended correctness pilots prove bounded/observable
work, not optimization, scaling or unsteady affordability.

## Preserved failures and scope limits

Initial build ambiguity, preliminary dimension0 validation, legacy diagnostic text,
forced-sampling planar budget rejection, missing mesh-bound marker fixture setup,
fractional integer-budget fixture and missing restarted-reference initialization
are retained with producer source/binary/config/input/log/resource receipts.
V2 sampling selected lambda.5 at3.1196 +/- .1458; same-field reassessment3.1626
+/- .0411 correctly rejected. Reusing existing strict planar integration, rather
than weakening the budget or mesh gates, enabled the accepted workflow. V5/v6
fixture failures are not production mesh failures. No failed campaign is relabeled
passing because a private intermediate mesh existed.

Open: calibrated global error and absolute-error work allocation, continuous optimum,
complete curved/interacting policy workflows, accepted M6 construction, viscous
accuracy, 2D automatic selection and complete-event performance. Composed-field
transport/gradation violations remain explicitly audited, not fixed or certified
by sensor gradation. No additional construction fallback or raised mesh ceiling.
