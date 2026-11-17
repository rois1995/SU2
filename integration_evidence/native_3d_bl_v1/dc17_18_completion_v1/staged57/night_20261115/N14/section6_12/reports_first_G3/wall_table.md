# Wall-stage observations

Report only; accepted mesh and result gates are separate.

Missing counter fields stay n/a. Parent coverage/remote attribution limits are explicit in WALL_SPLITS rows.

| case / rank | accepted | rounds | measured | geometry_trials | geometry_rejected | guidance_trials | guidance_rejected | guidance_seconds | upper_seconds | geometry_bad | size_bad | geometry_debt | size_debt | maximum_work_bytes | upper_refinements |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| N9_baseline / 0 | 457 | 4676 | 37234 | 3726 | 2084 | 1642 | 1180 | 176.90471694900029 | 232.48702961099946 | 2428 | 4237 | 0.00011979696876187805 | 5656.7291689432232 | n/a | 0 |
| N11_baseline / 0 | 459 | 4870 | 37673 | 3406 | 1886 | 1520 | 1056 | 164.71403814899995 | 231.28303656399942 | 2653 | 4609 | 0.00012947533365975256 | 5936.6983113070364 | n/a | 0 |
| N14_first / 0 | 1113 | 6055 | 41456 | 1377 | 140 | 506 | 118 | 26.040122075000021 | 255.40731787600052 | 3170 | 5972 | 0.00012879410631569341 | 4785.4595043388136 | n/a | 0 |

## Wall counter rows

| case | marker | fields |
|---|---|---|
| N9_baseline | NATIVE3D_WALL_REFUSAL | rank=0 class=SplitEdge reason=orientation count=0 |
| N9_baseline | NATIVE3D_WALL_REFUSAL | rank=0 class=SplitEdge reason=interface count=0 |
| N9_baseline | NATIVE3D_WALL_REFUSAL | rank=0 class=SplitEdge reason=surface_containment count=0 |
| N9_baseline | NATIVE3D_WALL_REFUSAL | rank=0 class=SplitEdge reason=feature count=0 |
| N9_baseline | NATIVE3D_WALL_REFUSAL | rank=0 class=SplitEdge reason=marker_junction count=0 |
| N9_baseline | NATIVE3D_WALL_REFUSAL | rank=0 class=SplitEdge reason=incomplete_star count=0 |
| N9_baseline | NATIVE3D_WALL_REFUSAL | rank=0 class=SplitEdge reason=other count=0 |
| N9_baseline | NATIVE3D_WALL_REFUSAL | rank=0 class=InsertEdgeCavity reason=orientation count=0 |
| N9_baseline | NATIVE3D_WALL_REFUSAL | rank=0 class=InsertEdgeCavity reason=interface count=0 |
| N9_baseline | NATIVE3D_WALL_REFUSAL | rank=0 class=InsertEdgeCavity reason=surface_containment count=0 |
| N9_baseline | NATIVE3D_WALL_REFUSAL | rank=0 class=InsertEdgeCavity reason=feature count=0 |
| N9_baseline | NATIVE3D_WALL_REFUSAL | rank=0 class=InsertEdgeCavity reason=marker_junction count=0 |
| N9_baseline | NATIVE3D_WALL_REFUSAL | rank=0 class=InsertEdgeCavity reason=incomplete_star count=0 |
| N9_baseline | NATIVE3D_WALL_REFUSAL | rank=0 class=InsertEdgeCavity reason=other count=0 |
| N9_baseline | NATIVE3D_WALL_REFUSAL | rank=0 class=other_geometry reason=orientation count=18 |
| N9_baseline | NATIVE3D_WALL_REFUSAL | rank=0 class=other_geometry reason=interface count=4 |
| N9_baseline | NATIVE3D_WALL_REFUSAL | rank=0 class=other_geometry reason=surface_containment count=0 |
| N9_baseline | NATIVE3D_WALL_REFUSAL | rank=0 class=other_geometry reason=feature count=0 |
| N9_baseline | NATIVE3D_WALL_REFUSAL | rank=0 class=other_geometry reason=marker_junction count=0 |
| N9_baseline | NATIVE3D_WALL_REFUSAL | rank=0 class=other_geometry reason=incomplete_star count=25 |
| N9_baseline | NATIVE3D_WALL_REFUSAL | rank=0 class=other_geometry reason=other count=857 |
| N9_baseline | NATIVE3D_WALL_REASON | rank=0 text=Native3D retained apex has incompatible height planes; different connectivity is required. count=345 |
| N9_baseline | NATIVE3D_WALL_REASON | rank=0 text=Native3D geometry-height incompatibility of the normal-row proposal at facet 48509 cell 28524296 count=1 |
| N9_baseline | NATIVE3D_WALL_REASON | rank=0 text=Native3D geometry-height incompatibility of the normal-row proposal at facet 48412 cell 28543732 count=1 |
| N9_baseline | NATIVE3D_WALL_REASON | rank=0 text=Native3D geometry-height incompatibility of the normal-row proposal at facet 48870 cell 28545800 count=1 |
| N9_baseline | NATIVE3D_WALL_REASON | rank=0 text=Native3D geometry-height incompatibility of the normal-row proposal at facet 48397 cell 28546788 count=1 |
| N9_baseline | NATIVE3D_WALL_REASON | rank=0 text=Native3D geometry-height incompatibility of the normal-row proposal at facet 47559 cell 28578523 count=1 |
| N9_baseline | NATIVE3D_WALL_REASON | rank=0 text=Native3D geometry-height incompatibility of the normal-row proposal at facet 47559 cell 28579568 count=1 |
| N9_baseline | NATIVE3D_WALL_REASON | rank=0 text=Native3D geometry-height incompatibility of the normal-row proposal at facet 47564 cell 28603115 count=1 |
| N9_baseline | NATIVE3D_WALL_REASON | rank=0 text=Native3D geometry-height incompatibility of the normal-row proposal at facet 47564 cell 28604143 count=1 |
| N9_baseline | NATIVE3D_WALL_REASON | rank=0 text=Native3D geometry-height incompatibility of the normal-row proposal at facet 48297 cell 28620512 count=1 |
| N9_baseline | NATIVE3D_WALL_REASON | rank=0 text=Native3D geometry-height incompatibility of the normal-row proposal at facet 47419 cell 28687094 count=1 |
| N9_baseline | NATIVE3D_WALL_REASON | rank=0 text=Native3D geometry-height incompatibility of the normal-row proposal at facet 48884 cell 28731106 count=1 |
| N9_baseline | NATIVE3D_WALL_REASON | rank=0 text=Native3D geometry-height incompatibility of the normal-row proposal at facet 48885 cell 28924658 count=1 |
| N9_baseline | NATIVE3D_WALL_REASON | rank=0 text=Native3D geometry-height incompatibility of the normal-row proposal at facet 48360 cell 29061873 count=1 |
| N9_baseline | NATIVE3D_WALL_REASON | rank=0 text=Native3D geometry-height incompatibility of the normal-row proposal at facet 47638 cell 29186793 count=1 |
| N9_baseline | NATIVE3D_WALL_REASON | rank=0 text=Native3D geometry-height incompatibility of the normal-row proposal at facet 47827 cell 29187853 count=1 |
| N9_baseline | NATIVE3D_WALL_REASON | rank=0 text=<more> count=544 |
| N9_baseline | NATIVE3D_WALL_REFUSAL | rank=0 class=PreservePassingFirstHeights count=0 |
| N9_baseline | NATIVE3D_WALL_REFUSAL | rank=0 class=height_check count=0 |
| N9_baseline | NATIVE3D_WALL_REFUSAL | rank=0 class=boundary_replacement_ready count=0 |
| N9_baseline | NATIVE3D_WALL_REFUSAL | rank=0 class=guidance_guard count=1180 |
| N9_baseline | NATIVE3D_WALL_REFUSAL | rank=0 class=unwrapped_geometry_trial count=0 |
| N9_baseline | NATIVE3D_WALL_GUIDANCE | rank=0 origin=size cause=incomplete count=10 |
| N9_baseline | NATIVE3D_WALL_GUIDANCE | rank=0 origin=size cause=nonfinite count=0 |
| N9_baseline | NATIVE3D_WALL_GUIDANCE | rank=0 origin=size cause=no_progress count=0 |
| N9_baseline | NATIVE3D_WALL_GUIDANCE | rank=0 origin=size cause=after_above count=0 |
| N9_baseline | NATIVE3D_WALL_GUIDANCE | rank=0 origin=size cause=exception count=0 |
| N9_baseline | NATIVE3D_WALL_GUIDANCE | rank=0 origin=size cause=other count=0 |
| N9_baseline | NATIVE3D_WALL_GUIDANCE | rank=0 origin=geometry cause=incomplete count=1167 |
| N9_baseline | NATIVE3D_WALL_GUIDANCE | rank=0 origin=geometry cause=nonfinite count=0 |
| N9_baseline | NATIVE3D_WALL_GUIDANCE | rank=0 origin=geometry cause=no_progress count=3 |
| N9_baseline | NATIVE3D_WALL_GUIDANCE | rank=0 origin=geometry cause=after_above count=0 |
| N9_baseline | NATIVE3D_WALL_GUIDANCE | rank=0 origin=geometry cause=exception count=0 |
| N9_baseline | NATIVE3D_WALL_GUIDANCE | rank=0 origin=geometry cause=other count=0 |
| N9_baseline | NATIVE3D_WALL_PRETRIAL | rank=0 disposition=other count=0 |
| N9_baseline | NATIVE3D_WALL_PRETRIAL | rank=0 disposition=oversized count=0 |
| N9_baseline | NATIVE3D_WALL_PRETRIAL | rank=0 disposition=conflict count=0 |
| N9_baseline | NATIVE3D_WALL_PRETRIAL | rank=0 disposition=deferred count=3455 |
| N9_baseline | NATIVE3D_WALL_PRETRIAL | rank=0 disposition=split_position count=0 |
| N9_baseline | NATIVE3D_WALL_COUNTS | rank=0 geometry_trials=3726 expected_geometry_trials=3726 geometry_rejected=2084 expected_geometry_rejected=2084 guidance_trials=1642 expected_guidance_trials=1642 geometric_change_guidance_trials=1642 priority_guidance=0 geometry_queue_guidance_trials=1425 size_queue_guidance_trials=217 guidance_rejected=1180 expected_guidance_rejected=1180 accepted=457 expected_accepted=457 metric_trials=462 expected_metric_trials=462 metric_validation_trials=462 metric_validation_rejected=5 metric_rejected=5 metric_vetoed=0 selected=4676 operator_entered=1221 pretrial=3455 choice_partition_sum=4676 expected_proposed=4676 expected_attempted=1221 planning_refusals=0 guarded_refusals=2084 unwrapped_geometry_trials=0 refusal_partition_sum=2084 counter_sums_match=1 |
| N11_baseline | NATIVE3D_WALL_LAW | rank=0 boundary=exit faces=50314 edge_incidences=150942 need_incidences=20014 unsupported_incidences=0 max_ratio=4924.9832181650645 seconds=74.060795342999995 |
| N11_baseline | NATIVE3D_WALL_REFUSAL | rank=0 class=SplitEdge reason=orientation count=0 |
| N11_baseline | NATIVE3D_WALL_REFUSAL | rank=0 class=SplitEdge reason=interface count=0 |
| N11_baseline | NATIVE3D_WALL_REFUSAL | rank=0 class=SplitEdge reason=surface_containment count=0 |
| N11_baseline | NATIVE3D_WALL_REFUSAL | rank=0 class=SplitEdge reason=feature count=0 |
| N11_baseline | NATIVE3D_WALL_REFUSAL | rank=0 class=SplitEdge reason=marker_junction count=0 |
| N11_baseline | NATIVE3D_WALL_REFUSAL | rank=0 class=SplitEdge reason=incomplete_star count=0 |
| N11_baseline | NATIVE3D_WALL_REFUSAL | rank=0 class=SplitEdge reason=other count=0 |
| N11_baseline | NATIVE3D_WALL_REFUSAL | rank=0 class=InsertEdgeCavity reason=orientation count=0 |
| N11_baseline | NATIVE3D_WALL_REFUSAL | rank=0 class=InsertEdgeCavity reason=interface count=0 |
| N11_baseline | NATIVE3D_WALL_REFUSAL | rank=0 class=InsertEdgeCavity reason=surface_containment count=0 |
| N11_baseline | NATIVE3D_WALL_REFUSAL | rank=0 class=InsertEdgeCavity reason=feature count=0 |
| N11_baseline | NATIVE3D_WALL_REFUSAL | rank=0 class=InsertEdgeCavity reason=marker_junction count=0 |
| N11_baseline | NATIVE3D_WALL_REFUSAL | rank=0 class=InsertEdgeCavity reason=incomplete_star count=0 |
| N11_baseline | NATIVE3D_WALL_REFUSAL | rank=0 class=InsertEdgeCavity reason=other count=0 |
| N11_baseline | NATIVE3D_WALL_REFUSAL | rank=0 class=other_geometry reason=orientation count=0 |
| N11_baseline | NATIVE3D_WALL_REFUSAL | rank=0 class=other_geometry reason=interface count=0 |
| N11_baseline | NATIVE3D_WALL_REFUSAL | rank=0 class=other_geometry reason=surface_containment count=0 |
| N11_baseline | NATIVE3D_WALL_REFUSAL | rank=0 class=other_geometry reason=feature count=0 |
| N11_baseline | NATIVE3D_WALL_REFUSAL | rank=0 class=other_geometry reason=marker_junction count=0 |
| N11_baseline | NATIVE3D_WALL_REFUSAL | rank=0 class=other_geometry reason=incomplete_star count=0 |
| N11_baseline | NATIVE3D_WALL_REFUSAL | rank=0 class=other_geometry reason=other count=0 |
| N11_baseline | NATIVE3D_WALL_REFUSAL | rank=0 class=RefitWallSplit reason=orientation count=0 |
| N11_baseline | NATIVE3D_WALL_REFUSAL | rank=0 class=RefitWallSplit reason=interface count=4 |
| N11_baseline | NATIVE3D_WALL_REFUSAL | rank=0 class=RefitWallSplit reason=surface_containment count=0 |
| N11_baseline | NATIVE3D_WALL_REFUSAL | rank=0 class=RefitWallSplit reason=feature count=0 |
| N11_baseline | NATIVE3D_WALL_REFUSAL | rank=0 class=RefitWallSplit reason=marker_junction count=0 |
| N11_baseline | NATIVE3D_WALL_REFUSAL | rank=0 class=RefitWallSplit reason=incomplete_star count=25 |
| N11_baseline | NATIVE3D_WALL_REFUSAL | rank=0 class=RefitWallSplit reason=other count=383 |
| N11_baseline | NATIVE3D_WALL_REFUSAL | rank=0 class=ReconstructWallSplit reason=orientation count=0 |
| N11_baseline | NATIVE3D_WALL_REFUSAL | rank=0 class=ReconstructWallSplit reason=interface count=0 |
| N11_baseline | NATIVE3D_WALL_REFUSAL | rank=0 class=ReconstructWallSplit reason=surface_containment count=0 |
| N11_baseline | NATIVE3D_WALL_REFUSAL | rank=0 class=ReconstructWallSplit reason=feature count=0 |
| N11_baseline | NATIVE3D_WALL_REFUSAL | rank=0 class=ReconstructWallSplit reason=marker_junction count=0 |
| N11_baseline | NATIVE3D_WALL_REFUSAL | rank=0 class=ReconstructWallSplit reason=incomplete_star count=0 |
| N11_baseline | NATIVE3D_WALL_REFUSAL | rank=0 class=ReconstructWallSplit reason=other count=400 |
| N11_baseline | NATIVE3D_WALL_REFUSAL | rank=0 class=ReconstructSplitPatch reason=orientation count=18 |
| N11_baseline | NATIVE3D_WALL_REFUSAL | rank=0 class=ReconstructSplitPatch reason=interface count=0 |
| N11_baseline | NATIVE3D_WALL_REFUSAL | rank=0 class=ReconstructSplitPatch reason=surface_containment count=0 |
| N11_baseline | NATIVE3D_WALL_REFUSAL | rank=0 class=ReconstructSplitPatch reason=feature count=0 |
| N11_baseline | NATIVE3D_WALL_REFUSAL | rank=0 class=ReconstructSplitPatch reason=marker_junction count=0 |
| N11_baseline | NATIVE3D_WALL_REFUSAL | rank=0 class=ReconstructSplitPatch reason=incomplete_star count=0 |
| N11_baseline | NATIVE3D_WALL_REFUSAL | rank=0 class=ReconstructSplitPatch reason=other count=0 |
| N11_baseline | NATIVE3D_WALL_REASON | rank=0 text=Native#D retained apex has incompatible height planes; different connectivity is required. count=312 |
| N11_baseline | NATIVE3D_WALL_REASON | rank=0 text=Native#D geometry-height incompatibility of the normal-row proposal at facet # cell # count=400 |
| N11_baseline | NATIVE3D_WALL_REASON | rank=0 text=Native#D joint point cannot see its retained skin. count=18 |
| N11_baseline | NATIVE3D_WALL_REASON | rank=0 text=Native #D cavity link has a nonmanifold edge. count=5 |
| N11_baseline | NATIVE3D_WALL_REASON | rank=0 text=Native #D cavity has a disconnected or unclosed link. count=5 |
| N11_baseline | NATIVE3D_WALL_REASON | rank=0 text=Native#D retained height/normal-foot constraints have no common fit. count=6 |
| N11_baseline | NATIVE3D_WALL_REASON | rank=0 text=Native#D retained height planes leave no normal-foot freedom. count=55 |
| N11_baseline | NATIVE3D_WALL_REASON | rank=0 text=Native#D wall refit needs complete free apex stars. count=25 |
| N11_baseline | NATIVE3D_WALL_REASON | rank=0 text=Native#D wall-foot fit needs visibility across an admitted interface. count=4 |
| N11_baseline | NATIVE3D_WALL_FRAME | rank=0 operation=RefitWallSplit calls=412 valid=0 seconds=8.7185220129999976 |
| N11_baseline | NATIVE3D_WALL_FRAME | rank=0 operation=ReconstructWallSplit calls=400 valid=0 seconds=17.969101995999992 |
| N11_baseline | NATIVE3D_WALL_FRAME | rank=0 operation=ReconstructSplitPatch calls=402 valid=384 seconds=26.305389337999987 |
| N11_baseline | NATIVE3D_WALL_SOURCE | rank=0 source=primary validated=1136 guidance_rejected=672 approved=459 |
| N11_baseline | NATIVE3D_WALL_SOURCE | rank=0 source=RefitWallSplit validated=0 guidance_rejected=0 approved=0 |
| N11_baseline | NATIVE3D_WALL_SOURCE | rank=0 source=ReconstructWallSplit validated=0 guidance_rejected=0 approved=0 |
| N11_baseline | NATIVE3D_WALL_SOURCE | rank=0 source=ReconstructSplitPatch validated=384 guidance_rejected=384 approved=0 |
| N11_baseline | NATIVE3D_WALL_LAW_SELECTION | rank=0 law_weighted_faces=2914 law_fallback_faces=0 accepted_geometry_splits=255 accepted_size_splits=204 |
| N11_baseline | NATIVE3D_WALL_REFUSAL | rank=0 class=PreservePassingFirstHeights count=0 |
| N11_baseline | NATIVE3D_WALL_REFUSAL | rank=0 class=height_check count=0 |
| N11_baseline | NATIVE3D_WALL_REFUSAL | rank=0 class=boundary_replacement_ready count=0 |
| N11_baseline | NATIVE3D_WALL_REFUSAL | rank=0 class=guidance_guard count=1056 |
| N11_baseline | NATIVE3D_WALL_REFUSAL | rank=0 class=unwrapped_geometry_trial count=0 |
| N11_baseline | NATIVE3D_WALL_GUIDANCE | rank=0 origin=size cause=incomplete count=10 |
| N11_baseline | NATIVE3D_WALL_GUIDANCE | rank=0 origin=size cause=nonfinite count=0 |
| N11_baseline | NATIVE3D_WALL_GUIDANCE | rank=0 origin=size cause=no_progress count=0 |
| N11_baseline | NATIVE3D_WALL_GUIDANCE | rank=0 origin=size cause=after_above count=0 |
| N11_baseline | NATIVE3D_WALL_GUIDANCE | rank=0 origin=size cause=exception count=0 |
| N11_baseline | NATIVE3D_WALL_GUIDANCE | rank=0 origin=size cause=other count=0 |
| N11_baseline | NATIVE3D_WALL_GUIDANCE | rank=0 origin=geometry cause=incomplete count=1043 |
| N11_baseline | NATIVE3D_WALL_GUIDANCE | rank=0 origin=geometry cause=nonfinite count=0 |
| N11_baseline | NATIVE3D_WALL_GUIDANCE | rank=0 origin=geometry cause=no_progress count=3 |
| N11_baseline | NATIVE3D_WALL_GUIDANCE | rank=0 origin=geometry cause=after_above count=0 |
| N11_baseline | NATIVE3D_WALL_GUIDANCE | rank=0 origin=geometry cause=exception count=0 |
| N11_baseline | NATIVE3D_WALL_GUIDANCE | rank=0 origin=geometry cause=other count=0 |
| N11_baseline | NATIVE3D_WALL_PRETRIAL | rank=0 disposition=other count=0 |
| N11_baseline | NATIVE3D_WALL_PRETRIAL | rank=0 disposition=oversized count=0 |
| N11_baseline | NATIVE3D_WALL_PRETRIAL | rank=0 disposition=conflict count=0 |
| N11_baseline | NATIVE3D_WALL_PRETRIAL | rank=0 disposition=deferred count=3734 |
| N11_baseline | NATIVE3D_WALL_PRETRIAL | rank=0 disposition=split_position count=0 |
| N11_baseline | NATIVE3D_WALL_COUNTS | rank=0 geometry_trials=3406 expected_geometry_trials=3406 geometry_rejected=1886 expected_geometry_rejected=1886 guidance_trials=1520 expected_guidance_trials=1520 geometric_change_guidance_trials=1520 priority_guidance=0 geometry_queue_guidance_trials=1301 size_queue_guidance_trials=219 guidance_rejected=1056 expected_guidance_rejected=1056 accepted=459 expected_accepted=459 metric_trials=464 expected_metric_trials=464 metric_validation_trials=464 metric_validation_rejected=5 metric_rejected=5 metric_vetoed=0 selected=4870 operator_entered=1136 pretrial=3734 choice_partition_sum=4870 expected_proposed=4870 expected_attempted=1136 planning_refusals=0 guarded_refusals=1886 unwrapped_geometry_trials=0 refusal_partition_sum=1886 counter_sums_match=1 |
| N14_first | NATIVE3D_WALL_LAW | rank=0 boundary=exit faces=51622 edge_incidences=154866 need_incidences=21667 unsupported_incidences=0 max_ratio=4924.9832181650645 seconds=75.947918439000006 |
| N14_first | NATIVE3D_WALL_REFUSAL | rank=0 class=SplitEdge reason=orientation count=0 |
| N14_first | NATIVE3D_WALL_REFUSAL | rank=0 class=SplitEdge reason=interface count=0 |
| N14_first | NATIVE3D_WALL_REFUSAL | rank=0 class=SplitEdge reason=surface_containment count=0 |
| N14_first | NATIVE3D_WALL_REFUSAL | rank=0 class=SplitEdge reason=feature count=0 |
| N14_first | NATIVE3D_WALL_REFUSAL | rank=0 class=SplitEdge reason=marker_junction count=0 |
| N14_first | NATIVE3D_WALL_REFUSAL | rank=0 class=SplitEdge reason=incomplete_star count=0 |
| N14_first | NATIVE3D_WALL_REFUSAL | rank=0 class=SplitEdge reason=other count=0 |
| N14_first | NATIVE3D_WALL_REFUSAL | rank=0 class=InsertEdgeCavity reason=orientation count=0 |
| N14_first | NATIVE3D_WALL_REFUSAL | rank=0 class=InsertEdgeCavity reason=interface count=0 |
| N14_first | NATIVE3D_WALL_REFUSAL | rank=0 class=InsertEdgeCavity reason=surface_containment count=0 |
| N14_first | NATIVE3D_WALL_REFUSAL | rank=0 class=InsertEdgeCavity reason=feature count=0 |
| N14_first | NATIVE3D_WALL_REFUSAL | rank=0 class=InsertEdgeCavity reason=marker_junction count=0 |
| N14_first | NATIVE3D_WALL_REFUSAL | rank=0 class=InsertEdgeCavity reason=incomplete_star count=0 |
| N14_first | NATIVE3D_WALL_REFUSAL | rank=0 class=InsertEdgeCavity reason=other count=0 |
| N14_first | NATIVE3D_WALL_REFUSAL | rank=0 class=other_geometry reason=orientation count=0 |
| N14_first | NATIVE3D_WALL_REFUSAL | rank=0 class=other_geometry reason=interface count=0 |
| N14_first | NATIVE3D_WALL_REFUSAL | rank=0 class=other_geometry reason=surface_containment count=0 |
| N14_first | NATIVE3D_WALL_REFUSAL | rank=0 class=other_geometry reason=feature count=0 |
| N14_first | NATIVE3D_WALL_REFUSAL | rank=0 class=other_geometry reason=marker_junction count=0 |
| N14_first | NATIVE3D_WALL_REFUSAL | rank=0 class=other_geometry reason=incomplete_star count=0 |
| N14_first | NATIVE3D_WALL_REFUSAL | rank=0 class=other_geometry reason=other count=0 |
| N14_first | NATIVE3D_WALL_REFUSAL | rank=0 class=RefitWallSplit reason=orientation count=0 |
| N14_first | NATIVE3D_WALL_REFUSAL | rank=0 class=RefitWallSplit reason=interface count=0 |
| N14_first | NATIVE3D_WALL_REFUSAL | rank=0 class=RefitWallSplit reason=surface_containment count=0 |
| N14_first | NATIVE3D_WALL_REFUSAL | rank=0 class=RefitWallSplit reason=feature count=0 |
| N14_first | NATIVE3D_WALL_REFUSAL | rank=0 class=RefitWallSplit reason=marker_junction count=0 |
| N14_first | NATIVE3D_WALL_REFUSAL | rank=0 class=RefitWallSplit reason=incomplete_star count=3 |
| N14_first | NATIVE3D_WALL_REFUSAL | rank=0 class=RefitWallSplit reason=other count=8 |
| N14_first | NATIVE3D_WALL_REFUSAL | rank=0 class=ReconstructWallSplit reason=orientation count=0 |
| N14_first | NATIVE3D_WALL_REFUSAL | rank=0 class=ReconstructWallSplit reason=interface count=0 |
| N14_first | NATIVE3D_WALL_REFUSAL | rank=0 class=ReconstructWallSplit reason=surface_containment count=0 |
| N14_first | NATIVE3D_WALL_REFUSAL | rank=0 class=ReconstructWallSplit reason=feature count=0 |
| N14_first | NATIVE3D_WALL_REFUSAL | rank=0 class=ReconstructWallSplit reason=marker_junction count=0 |
| N14_first | NATIVE3D_WALL_REFUSAL | rank=0 class=ReconstructWallSplit reason=incomplete_star count=0 |
| N14_first | NATIVE3D_WALL_REFUSAL | rank=0 class=ReconstructWallSplit reason=other count=11 |
| N14_first | NATIVE3D_WALL_REFUSAL | rank=0 class=ReconstructSplitPatch reason=orientation count=0 |
| N14_first | NATIVE3D_WALL_REFUSAL | rank=0 class=ReconstructSplitPatch reason=interface count=0 |
| N14_first | NATIVE3D_WALL_REFUSAL | rank=0 class=ReconstructSplitPatch reason=surface_containment count=0 |
| N14_first | NATIVE3D_WALL_REFUSAL | rank=0 class=ReconstructSplitPatch reason=feature count=0 |
| N14_first | NATIVE3D_WALL_REFUSAL | rank=0 class=ReconstructSplitPatch reason=marker_junction count=0 |
| N14_first | NATIVE3D_WALL_REFUSAL | rank=0 class=ReconstructSplitPatch reason=incomplete_star count=0 |
| N14_first | NATIVE3D_WALL_REFUSAL | rank=0 class=ReconstructSplitPatch reason=other count=0 |
| N14_first | NATIVE3D_WALL_REASON | rank=0 text=Native#D retained apex has incompatible height planes; different connectivity is required. count=8 |
| N14_first | NATIVE3D_WALL_REASON | rank=0 text=Native#D geometry-height incompatibility of the normal-row proposal at facet # cell # count=11 |
| N14_first | NATIVE3D_WALL_REASON | rank=0 text=Native#D wall refit needs complete free apex stars. count=3 |
| N14_first | NATIVE3D_WALL_FRAME | rank=0 operation=RefitWallSplit calls=11 valid=0 seconds=0.123898383 |
| N14_first | NATIVE3D_WALL_FRAME | rank=0 operation=ReconstructWallSplit calls=11 valid=0 seconds=0.46649285899999998 |
| N14_first | NATIVE3D_WALL_FRAME | rank=0 operation=ReconstructSplitPatch calls=12 valid=12 seconds=0.48681929800000001 |
| N14_first | NATIVE3D_WALL_SOURCE | rank=0 source=primary validated=1225 guidance_rejected=106 approved=1113 |
| N14_first | NATIVE3D_WALL_SOURCE | rank=0 source=RefitWallSplit validated=0 guidance_rejected=0 approved=0 |
| N14_first | NATIVE3D_WALL_SOURCE | rank=0 source=ReconstructWallSplit validated=0 guidance_rejected=0 approved=0 |
| N14_first | NATIVE3D_WALL_SOURCE | rank=0 source=ReconstructSplitPatch validated=12 guidance_rejected=12 approved=0 |
| N14_first | NATIVE3D_WALL_LAW_SELECTION | rank=0 law_weighted_faces=4407 law_fallback_faces=0 accepted_geometry_splits=764 accepted_size_splits=349 |
| N14_first | NATIVE3D_WALL_SPLITS | rank=0 wall_splits_required_trials=731 wall_splits_required_accepted=731 wall_splits_checked_trials=132 wall_splits_checked_guidance_rejected=99 wall_splits_checked_accepted=33 new_faces=2916 new_faces_at_or_below_tolerance=1319 new_faces_above_parent_plus_tolerance=0 new_faces_unknown_parent=528 children_unmatched=0 children_unmeasured=8 attribution=locally_registered_primary_faces observation_failures=0 |
| N14_first | NATIVE3D_WALL_REFUSAL | rank=0 class=PreservePassingFirstHeights count=0 |
| N14_first | NATIVE3D_WALL_REFUSAL | rank=0 class=height_check count=0 |
| N14_first | NATIVE3D_WALL_REFUSAL | rank=0 class=boundary_replacement_ready count=0 |
| N14_first | NATIVE3D_WALL_REFUSAL | rank=0 class=guidance_guard count=118 |
| N14_first | NATIVE3D_WALL_REFUSAL | rank=0 class=unwrapped_geometry_trial count=0 |
| N14_first | NATIVE3D_WALL_GUIDANCE | rank=0 origin=size cause=incomplete count=7 |
| N14_first | NATIVE3D_WALL_GUIDANCE | rank=0 origin=size cause=nonfinite count=0 |
| N14_first | NATIVE3D_WALL_GUIDANCE | rank=0 origin=size cause=no_progress count=0 |
| N14_first | NATIVE3D_WALL_GUIDANCE | rank=0 origin=size cause=after_above count=0 |
| N14_first | NATIVE3D_WALL_GUIDANCE | rank=0 origin=size cause=exception count=0 |
| N14_first | NATIVE3D_WALL_GUIDANCE | rank=0 origin=size cause=other count=0 |
| N14_first | NATIVE3D_WALL_GUIDANCE | rank=0 origin=geometry cause=incomplete count=97 |
| N14_first | NATIVE3D_WALL_GUIDANCE | rank=0 origin=geometry cause=nonfinite count=0 |
| N14_first | NATIVE3D_WALL_GUIDANCE | rank=0 origin=geometry cause=no_progress count=14 |
| N14_first | NATIVE3D_WALL_GUIDANCE | rank=0 origin=geometry cause=after_above count=0 |
| N14_first | NATIVE3D_WALL_GUIDANCE | rank=0 origin=geometry cause=exception count=0 |
| N14_first | NATIVE3D_WALL_GUIDANCE | rank=0 origin=geometry cause=other count=0 |
| N14_first | NATIVE3D_WALL_PRETRIAL | rank=0 disposition=other count=0 |
| N14_first | NATIVE3D_WALL_PRETRIAL | rank=0 disposition=oversized count=0 |
| N14_first | NATIVE3D_WALL_PRETRIAL | rank=0 disposition=conflict count=0 |
| N14_first | NATIVE3D_WALL_PRETRIAL | rank=0 disposition=deferred count=4830 |
| N14_first | NATIVE3D_WALL_PRETRIAL | rank=0 disposition=split_position count=0 |
| N14_first | NATIVE3D_WALL_COUNTS | rank=0 geometry_trials=1377 expected_geometry_trials=1377 geometry_rejected=140 expected_geometry_rejected=140 guidance_trials=506 expected_guidance_trials=506 geometric_change_guidance_trials=506 priority_guidance=0 geometry_queue_guidance_trials=144 size_queue_guidance_trials=362 guidance_rejected=118 expected_guidance_rejected=118 accepted=1113 expected_accepted=1113 metric_trials=1119 expected_metric_trials=1119 metric_validation_trials=1119 metric_validation_rejected=6 metric_rejected=6 metric_vetoed=0 selected=6055 operator_entered=1225 pretrial=4830 choice_partition_sum=6055 expected_proposed=6055 expected_attempted=1225 planning_refusals=0 guarded_refusals=140 unwrapped_geometry_trials=0 refusal_partition_sum=140 counter_sums_match=1 |
| N14_first | NATIVE3D_WALL_LANE_CENSUS | rank=0 commits=0 class_s=0 class_w1=0 class_w2m=0 class_w2o=0 w1_kind0=0 w1_kind1=0 w1_kind2=0 w2m_kind1=0 w1_link_local=0 w2m_link_local=0 w1_first_proof=0 w1_link_local_first_proof=0 seconds_s=0 seconds_w1=0 seconds_w2m=0 seconds_w2o=0 seconds_w1_link_local=0 seconds_w1_link_local_first_proof=0 seconds_w2m_link_local=0 |
| N14_first | NATIVE3D_WALL_PREPARE | rank=0 stage=preparation calls=0 seconds=0 edits=0 faces=0 |
| N14_first | NATIVE3D_WALL_PREPARE | rank=0 stage=metric calls=0 seconds=0 edits=0 faces=0 |
| N14_first | NATIVE3D_WALL_PREPARE | rank=0 stage=construction calls=0 seconds=0 edits=0 faces=0 |
| N14_first | NATIVE3D_WALL_PREPARE | rank=0 stage=surface calls=1113 seconds=1.3670429710000005 edits=1113 faces=6678 |
| N14_first | NATIVE3D_WALL_PREPARE | rank=0 stage=quality calls=0 seconds=0 edits=0 faces=0 |
| N14_first | NATIVE3D_WALL_PREPARE | rank=0 stage=final_proof calls=0 seconds=0 edits=0 faces=0 |
| N14_first | NATIVE3D_WALL_PREPARE | rank=0 stage=transfer calls=0 seconds=0 edits=0 faces=0 |
| N14_first | NATIVE3D_WALL_PREPARE | rank=0 stage=output calls=0 seconds=0 edits=0 faces=0 |
