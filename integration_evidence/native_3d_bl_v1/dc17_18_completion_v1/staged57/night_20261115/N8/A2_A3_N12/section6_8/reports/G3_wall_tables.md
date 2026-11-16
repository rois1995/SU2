# G3 corrected section6.8 measurement

G3 passed:384accepted splits, all384common edits identical to N1; observer-adjusted entrywork5113. Positive cells/sensors,first336P6c,zero upper refinements and original model/job/height/input gates passed. No accepted BL mesh is claimed. First fixture timeout/setup corrections are in validation/R3_G3_wrapper_timeout and R3_G3_bounded_command_schema.

| Normalized reason key | Count |
|---|---:|
| Native#D retained apex has incompatible height planes; different connectivity is required. | 318 |
| Native#D geometry-height incompatibility of the normal-row proposal at facet # cell # | 402 |
| Native#D joint point cannot see its retained skin. | 18 |
| Native #D cavity link has a nonmanifold edge. | 5 |
| Native #D cavity has a disconnected or unclosed link. | 5 |
| Native#D retained height/normal-foot constraints have no common fit. | 6 |
| Native#D retained height planes leave no normal-foot freedom. | 58 |
| Native#D wall refit needs complete free apex stars. | 18 |
| Native#D wall-foot fit needs visibility across an admitted interface. | 4 |

| Raw frame operator | Calls | Raw valid | Seconds |
|---|---:|---:|---:|
|RefitWallSplit|414|0|9.2593019989999998|
|ReconstructWallSplit|402|0|18.527632406000013|
|ReconstructSplitPatch|404|386|27.045679694999976|

Raw valid is the operator return, before guarded_geometry and later metric/publication checks. All834non-Guidance guard-refusal texts reconcile. Three new raw classes plus residual other_geometry reconstruct legacy other for this run; the old N9 count904 is a clock-dependent report baseline.

## B0 totals

| Key | Value |
|---|---:|
|rank|0|
|geometry_trials|3230|
|expected_geometry_trials|3230|
|geometry_rejected|1838|
|expected_geometry_rejected|1838|
|guidance_trials|1392|
|expected_guidance_trials|1392|
|geometric_change_guidance_trials|1392|
|priority_guidance|0|
|geometry_queue_guidance_trials|1197|
|size_queue_guidance_trials|195|
|guidance_rejected|1004|
|expected_guidance_rejected|1004|
|accepted|384|
|expected_accepted|384|
|metric_trials|388|
|expected_metric_trials|388|
|metric_validation_trials|388|
|metric_validation_rejected|4|
|metric_rejected|4|
|metric_vetoed|0|
|selected|3140|
|operator_entered|1006|
|pretrial|2134|
|choice_partition_sum|3140|
|expected_proposed|3140|
|expected_attempted|1006|
|planning_refusals|0|
|guarded_refusals|1838|
|unwrapped_geometry_trials|0|
|refusal_partition_sum|1838|
|counter_sums_match|1|
