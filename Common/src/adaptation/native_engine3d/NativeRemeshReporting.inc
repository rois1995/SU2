// Serialize the existing native remesh diagnostics in their collective order.
namespace {

void WriteRemeshSummary(std::ostream& row, const World& parent, const RemeshStats& stats) {
  row << std::setprecision(17) << "NATIVE3D rank=" << parent.rank << " ranks=" << parent.size
      << " workers=" << stats.workers << " complete=" << stats.complete << " detailed=" << stats.detailed_timings
      << " total=" << stats.total_seconds << " other=" << stats.other_seconds << " cpu=" << stats.process_cpu_seconds
      << " idle_cpu=" << stats.idle_cpu_seconds;
  for (size_t j = 0; j < stats.names.size(); ++j) row << ' ' << stats.names[j] << '=' << stats.phases[j];
  row << " initial=" << stats.initial_cells << " final=" << stats.final_cells
      << " refine_accepted=" << stats.refine.accepted << " coarsen_accepted=" << stats.coarsen.accepted
      << " tracked_bytes=" << stats.communication_bytes << " tracked_calls=" << stats.communication_calls
      << " nested_tracked_collectives=" << stats.communication_seconds
      << " tracked_import_bytes=" << stats.imported.tracked_bytes
      << " tracked_import_calls=" << stats.imported.tracked_calls
      << " nested_tracked_import=" << stats.imported.tracked_collective_seconds
      << " tracked_parent_bytes=" << stats.parent_transport_bytes
      << " tracked_parent_calls=" << stats.parent_transport_calls
      << " nested_tracked_parent=" << stats.parent_transport_seconds
      << " tracked_worker_bytes=" << stats.worker_transport_bytes
      << " tracked_worker_calls=" << stats.worker_transport_calls
      << " nested_tracked_worker=" << stats.worker_transport_seconds << " native_bound=" << stats.maximum_native_bytes
      << " process_peak_rss=" << stats.process_peak_rss_bytes << " job_peak_rss=" << stats.job_peak_rss_bytes
      << " job_peak_rss_lower=" << stats.job_peak_rss_lower_bytes << " final_proof_faces=" << stats.final_proof_faces
      << " final_proof_surface_faces=" << stats.final_proof_surface_faces
      << " geometry_distinct_facets=" << stats.geometry_distinct_facets
      << " geometry_distinct_scope_keys=" << stats.geometry_distinct_scope_keys
      << " geometry_prepared_facet_mean_bytes=" << stats.geometry_prepared_facet_mean_bytes << '\n';
}

void WriteRemeshDetailPrefix(std::ostream& detail, const char* stage, const World& parent, const RemeshStats& stats) {
  detail << "NATIVE3D_DETAIL stage=" << stage << " rank=" << parent.rank << " ranks=" << parent.size
         << " workers=" << stats.workers << " complete=" << stats.complete;
}

void WriteRemeshBridgeDetail(std::ostream& detail, const char* stage, const World& parent, const RemeshStats& stats,
                             const BridgeStats& bridge) {
  WriteRemeshDetailPrefix(detail, stage, parent, stats);
  detail << " total=" << bridge.total_seconds << " points=" << bridge.points_seconds
         << " cells=" << bridge.cells_seconds << " faces=" << bridge.faces_seconds
         << " source=" << bridge.source_seconds << " query=" << bridge.query_seconds
         << " reader=" << bridge.reader_seconds << " reduction=" << bridge.reduction_seconds
         << " other=" << bridge.other_seconds << " requests=" << bridge.requests << " hits=" << bridge.hits
         << " misses=" << bridge.misses << " invalid_queries=" << bridge.invalid_queries
         << " failures=" << bridge.failures << " evictions=" << bridge.evictions
         << " compositions=" << bridge.compositions << " batches=" << bridge.batches
         << " retries=" << bridge.batch_retries << " source_point_queries=" << bridge.source_point_queries
         << " source_point_payloads=" << bridge.source_point_payloads
         << " source_point_seconds=" << bridge.source_point_seconds
         << " source_point_search=" << bridge.source_point_search_seconds
         << " source_point_interpolation=" << bridge.source_point_interpolation_seconds
         << " geometry_estimate=" << bridge.geometry_estimate_seconds
         << " predicted_geometry_operations=" << bridge.geometry_operations
         << " predicted_geometry_work=" << bridge.geometry_work
         << " geometry_weight_scale=" << bridge.geometry_weight_scale << " boundary_faces=" << bridge.geometry_faces
         << " cross_chart_faces=" << bridge.geometry_cross_chart_faces << " field_builds=" << bridge.field_builds
         << " nested_field_build=" << bridge.field_build_seconds << " nested_search=" << bridge.field_search_seconds
         << " nested_interpolation=" << bridge.interpolation_seconds
         << " nested_composition=" << bridge.composition_seconds << " tracked_bytes=" << bridge.tracked_bytes
         << " tracked_calls=" << bridge.tracked_calls
         << " nested_tracked_collectives=" << bridge.tracked_collective_seconds
         << " nested_source_envelope=" << bridge.source_envelope_seconds
         << " source_envelope_nodes=" << bridge.source_envelope_nodes
         << " source_envelope_cells=" << bridge.source_envelope_cells
         << " source_envelope_solves=" << bridge.source_envelope_solves
         << " source_envelope_arithmetic_work_sum=" << bridge.source_envelope_arithmetic_work
         << " source_envelope_arithmetic_limit_per_rank=" << bridge.source_envelope_arithmetic_work_limit
         << " source_envelope_contacts=" << bridge.source_envelope_contacts
         << " source_envelope_distances=" << bridge.source_envelope_distances
         << " nested_envelope_payload=" << bridge.source_envelope_payload_seconds
         << " envelope_replicated_bytes=" << bridge.source_envelope_payload_bytes
         << " parameter_return=" << bridge.parameter_return_seconds
         << " parameter_bytes=" << bridge.parameter_return_bytes << " parameter_points=" << bridge.parameter_points
         << " parameter_faces=" << bridge.parameter_faces << " native_bound=" << bridge.maximum_native_bytes << '\n';
}

void WriteRemeshSourceDetail(std::ostream& detail, const World& parent, const RemeshStats& stats) {
  WriteRemeshDetailPrefix(detail, "source", parent, stats);
  const auto& source = stats.source;
  detail << " build=" << source.build_seconds << " imports=" << source.import_seconds
         << " requests=" << source.owner_requests << " candidates=" << source.index_candidates
         << " payloads=" << source.payloads << " maximum_patch=" << source.maximum_patch
         << " nested_routing=" << source.routing_seconds << " nested_search=" << source.search_seconds
         << " nested_filter=" << source.filter_seconds << " nested_exchange=" << source.exchange_seconds << '\n';
}

void WriteRemeshPartitionDetail(std::ostream& detail, const World& parent, const RemeshStats& stats) {
  WriteRemeshDetailPrefix(detail, "partition", parent, stats);
  const auto& partition = stats.partition;
  auto at_rank = [&](const std::vector<uint64_t>& values) {
    return values.empty() ? uint64_t(0) : values.at(parent.rank);
  };
  detail << " total=" << partition.total_seconds << " graph=" << partition.graph_seconds
         << " parmetis=" << partition.partition_seconds << " migration=" << partition.migration_seconds
         << " other=" << partition.other_seconds << " nested_directory=" << partition.directory_seconds
         << " moved=" << partition.moved_cells << " arcs=" << partition.graph_arcs
         << " cells_before=" << at_rank(partition.cells_before) << " cells_after=" << at_rank(partition.cells_after)
         << " work_before=" << at_rank(partition.work_before) << " work_after=" << at_rank(partition.work_after)
         << " geometry_before=" << at_rank(partition.geometry_before)
         << " geometry_after=" << at_rank(partition.geometry_after) << " edgecut=" << partition.edgecut
         << " native_bound=" << partition.maximum_native_bytes << '\n';
}

void WriteEngineRoundWork(std::ostream& detail, const EngineStats& engine) {
  detail << " total=" << engine.total_seconds << " initialization=" << engine.initialization_seconds
         << " seed=" << engine.seed_seconds << " first_layer=" << engine.layer_seconds
         << " selection=" << engine.selection_seconds << " dependency=" << engine.dependency_seconds
         << " reservation=" << engine.reservation_seconds << " donor=" << engine.donor_seconds
         << " reconstruction=" << engine.reconstruction_seconds << " metric=" << engine.metric_seconds
         << " collision=" << engine.collision_seconds << " collision_builds=" << engine.collision_builds
         << " collision_candidates=" << engine.collision_candidates << " collision_checks=" << engine.collision_checks
         << " routing=" << engine.routing_seconds << " publication=" << engine.publication_seconds
         << " update=" << engine.update_seconds << " completion=" << engine.completion_seconds
         << " other=" << engine.other_seconds << " tracked_bytes=" << engine.tracked_bytes
         << " tracked_calls=" << engine.tracked_calls
         << " nested_tracked_collectives=" << engine.tracked_collective_seconds
         << " directory_elections=" << engine.directory_elections
         << " nested_directory_election=" << engine.directory_election_seconds
         << " process_cpu=" << engine.process_cpu_seconds
         << " private_reconstruction_cpu=" << engine.reconstruction_cpu_seconds
         << " private_metric_cpu=" << engine.metric_cpu_seconds
         << " private_measurement_cpu=" << engine.measurement_cpu_seconds
         << " planning_nested_s=" << engine.planning_seconds << " batch_considered=" << engine.batch_considered
         << " batch_deferred=" << engine.batch_deferred << " proposal_duplicates=" << engine.proposal_duplicates
         << " batch_rounds=" << engine.batch_rounds << " batch_candidates=" << engine.batch_candidates
         << " stop=" << EngineStopName(engine.stop) << " global_work=" << engine.global_work
         << " work_limit=" << engine.work_limit << " progress_windows=" << engine.progress_windows
         << " stagnant_windows=" << engine.stagnant_windows << " search_windows=" << engine.search_windows
         << " geometry_progress=" << engine.geometry_progress << " geometry_reactivated=" << engine.geometry_reactivated
         << " size_progress=" << engine.size_progress << " quality_progress=" << engine.quality_progress
         << " cell_removals=" << engine.cell_removals << " bidirectional=" << engine.bidirectional
         << " length_split_passes=" << engine.length_split_passes
         << " length_collapse_passes=" << engine.length_collapse_passes << " length_splits=" << engine.length_splits
         << " length_collapses=" << engine.length_collapses << " length_pass_deferred=" << engine.length_pass_deferred
         << " length_expansion_deferrals=" << engine.length_expansion_deferrals;
}

void WriteEngineOperatorWork(std::ostream& detail, const EngineStats& engine) {
  detail << " quality_stages=" << engine.quality_stages << " construction_returns=" << engine.construction_returns
         << " coarsen_checkpoints=" << engine.coarsen_checkpoints
         << " coarsen_committed_passes=" << engine.coarsen_committed_passes
         << " coarsen_rolled_back_passes=" << engine.coarsen_rolled_back_passes
         << " coarsen_retired_directions=" << engine.coarsen_retired_directions
         << " coarsen_restored_cells=" << engine.coarsen_restored_cells
         << " coarsen_rolled_back_collapses=" << engine.coarsen_rolled_back_collapses
         << " layer_shape_screens=" << engine.layer_shape_screens << " layer_shape_pruned=" << engine.layer_shape_pruned
         << " layer_batches=" << engine.layer_batches << " layer_maximum_batch=" << engine.layer_maximum_batch
         << " layer_donor_batches=" << engine.layer_donor_batches
         << " layer_donor_retries=" << engine.layer_donor_retries
         << " layer_metadata_nested_s=" << engine.layer_metadata_seconds
         << " layer_donor_nested_s=" << engine.layer_donor_seconds
         << " layer_geometry_nested_s=" << engine.layer_geometry_seconds
         << " layer_metric_nested_s=" << engine.layer_metric_seconds
         << " layer_publication_nested_s=" << engine.layer_publication_seconds
         << " layer_edge_evaluations=" << engine.layer_edge_evaluations
         << " layer_edge_reused=" << engine.layer_edge_reused << " layer_published=" << engine.layer_published
         << " height_checks=" << engine.height_checks << " height_rejected=" << engine.height_rejected
         << " footprint_rejected=" << engine.footprint_rejected
         << " wall_reconstruction_trials=" << engine.wall_reconstruction_trials
         << " wall_reconstruction_accepted=" << engine.wall_reconstruction_accepted
         << " wall_joint_trials=" << engine.wall_joint_trials << " wall_joint_moves=" << engine.wall_joint_moves
         << " wall_joint_removals=" << engine.wall_joint_removals
         << " wall_joint_accepted=" << engine.wall_joint_accepted << " wall_joint_flips=" << engine.wall_joint_flips
         << " wall_joint_vertex_moves=" << engine.wall_joint_vertex_moves
         << " wall_joint_coordinated=" << engine.wall_joint_coordinated
         << " wall_joint_solve_rejected=" << engine.wall_joint_solve_rejected
         << " wall_joint_move_geometry_rejected=" << engine.wall_joint_move_geometry_rejected
         << " wall_joint_move_height_rejected=" << engine.wall_joint_move_height_rejected
         << " wall_joint_move_loss_rejected=" << engine.wall_joint_move_loss_rejected
         << " oversized_stars=" << engine.oversized_stars << " optimization_trials=" << engine.optimization_trials
         << " optimization_validated=" << engine.optimization_validated
         << " atlas_arithmetic_work=" << engine.atlas_work << " atlas_peak_bytes=" << engine.atlas_peak_bytes
         << " optimization_moves=" << engine.optimization_moves
         << " optimization_insertions=" << engine.optimization_insertions
         << " optimization_collapses=" << engine.optimization_collapses
         << " optimization_swaps=" << engine.optimization_swaps
         << " placement_connectivity_trials=" << engine.placement_connectivity_trials
         << " placement_flips=" << engine.placement_flips << " placement_moves=" << engine.placement_moves
         << " placement_dependency_expansions=" << engine.placement_dependency_expansions
         << " placement_classified_vertices=" << engine.placement_classified_vertices
         << " placement_trials=" << engine.placement_trials << " placement_validated=" << engine.placement_validated
         << " wall_topology_trials=" << engine.wall_topology_trials
         << " wall_topology_accepted=" << engine.wall_topology_accepted << " geometry_trials=" << engine.geometry_trials
         << " geometry_rejected=" << engine.geometry_rejected << " metric_trials=" << engine.metric_trials
         << " metric_shape_rejected=" << engine.metric_shape_rejected
         << " metric_progress_rejected=" << engine.metric_progress_rejected
         << " metric_measurement_rejected=" << engine.metric_measurement_rejected;
}

void WriteEngineRejection(std::ostream& detail, const EngineStats& engine) {
  detail << " sample_metric_present=" << (engine.last_metric_rejection.failure != MetricFailure::None)
         << " sample_shape_measured=" << engine.last_metric_rejection.shape_measured << " sample_q=";
  if (engine.last_metric_rejection.shape_measured)
    detail << engine.last_metric_rejection.minimum_mean_ratio;
  else
    detail << "unavailable";
  detail << " sample_j=";
  if (engine.last_metric_rejection.shape_measured)
    detail << engine.last_metric_rejection.minimum_scaled_jacobian;
  else
    detail << "unavailable";
  detail << " sample_old_shape_cells=" << engine.last_metric_rejection.old_shape_cells
         << " sample_new_shape_cells=" << engine.last_metric_rejection.new_shape_cells
         << " sample_old_shape_debt=" << engine.last_metric_rejection.old_shape_debt
         << " sample_new_shape_debt=" << engine.last_metric_rejection.new_shape_debt
         << " sample_cell=" << engine.last_metric_rejection.weakest_cell
         << " sample_sliver=" << engine.last_metric_rejection.sliver_cell
         << " sample_feature=" << engine.last_rejected_feature.count
         << " sample_v0=" << engine.last_rejected_feature.vertices[0]
         << " sample_v1=" << engine.last_rejected_feature.vertices[1]
         << " sample_v2=" << engine.last_rejected_feature.vertices[2]
         << " sample_x=" << engine.last_rejected_cavity_centroid.x
         << " sample_y=" << engine.last_rejected_cavity_centroid.y
         << " sample_z=" << engine.last_rejected_cavity_centroid.z << " batch_reserved=" << engine.batch_reserved
         << " concurrent_rounds=" << engine.concurrent_rounds << " final_reservations=" << engine.final_reservations
         << " proposal_deferred=" << engine.proposal_deferred
         << " maximum_parallel_commits=" << engine.maximum_parallel_commits << " rounds=" << engine.rounds
         << " attempted=" << engine.attempted << " accepted=" << engine.accepted;
}

void WriteEngineTopologyWork(std::ostream& detail, const EngineStats& engine) {
  detail << " region_core_trials=" << engine.region_core_trials << " region_core_regions=" << engine.region_core_regions
         << " region_transition_fronts=" << engine.region_transition_fronts
         << " region_front_trials=" << engine.region_front_trials
         << " region_cap_nested_s=" << engine.region_cap_seconds
         << " region_sampling_s=" << engine.region_sampling_seconds
         << " region_first_band_s=" << engine.region_first_band_seconds
         << " region_transition_s=" << engine.region_transition_seconds
         << " region_junction_s=" << engine.region_junction_seconds << " region_core_s=" << engine.region_core_seconds
         << " region_validation_s=" << engine.region_validation_seconds
         << " region_field_points=" << engine.region_field_points
         << " region_sensor_hits=" << engine.region_field_sensor_hits
         << " region_interior_hits=" << engine.region_field_interior_hits
         << " region_field_edges=" << engine.region_field_edges << " region_edge_hits=" << engine.region_field_edge_hits
         << " region_field_imports=" << engine.region_field_imports
         << " region_maximum_resident=" << engine.region_field_maximum_resident << " rejected=" << engine.rejected
         << " conflicts=" << engine.conflicts << " requests=" << engine.field_requests << " hits=" << engine.field_hits
         << " misses=" << engine.field_misses << " invalid_queries=" << engine.field_invalid_queries
         << " failures=" << engine.field_failures << " evictions=" << engine.field_evictions
         << " compositions=" << engine.field_compositions << " edge_evaluations=" << engine.edge_evaluations
         << " collapse_geometry_checks=" << engine.collapse_geometry_checks
         << " collapse_filtered=" << engine.collapse_filtered
         << " collapse_orientation_checks=" << engine.collapse_orientation_checks
         << " collapse_orientation_filtered=" << engine.collapse_orientation_filtered
         << " collapse_correction_plans=" << engine.collapse_corrections
         << " collapse_correction_cells=" << engine.collapse_correction_cells
         << " collapse_correction_global_work=" << engine.collapse_correction_work
         << " nested_collapse_correction=" << engine.collapse_correction_seconds
         << " blocked_queue_skips=" << engine.blocked_queue_skips << " blocked_unmeasured=" << engine.blocked_unmeasured
         << " collapse_reactivated=" << engine.collapse_reactivated
         << " changed_star_scans=" << engine.changed_star_scans
         << " changed_vertices_sent=" << engine.changed_vertices_sent
         << " changed_vertices_received=" << engine.changed_vertices_received
         << " nested_change_notifications=" << engine.change_notification_seconds
         << " nested_collapse_orientation=" << engine.collapse_orientation_seconds
         << " nested_collapse_geometry=" << engine.collapse_geometry_seconds << " edge_reused=" << engine.edge_reused
         << " moved=" << engine.moved << " reconnected=" << engine.reconnected << " inserted=" << engine.inserted
         << " maximum_donor_patch=" << engine.donor_maximum_patch << " maximum_owned=" << engine.maximum_owned
         << " final_owned_cells=" << engine.final_owned_cells
         << " cached_measured_cells=" << engine.cached_measured_cells << " seed_batches=" << engine.seed_batches
         << " unmeasured_retired=" << engine.unmeasured_retired
         << " cached_invalid_cells=" << engine.cached_invalid_cells << " remaining_edges=" << engine.remaining_edges
         << " remaining_repairs=" << engine.remaining_repairs;
}

void WriteEngineFieldWork(std::ostream& detail, const EngineStats& engine) {
  detail << " donor_hint_tests=" << engine.donor_hint_tests << " donor_hint_hits=" << engine.donor_hint_hits
         << " private_edge_evaluations=" << engine.private_edge_evaluations
         << " private_edge_reused=" << engine.private_edge_reused
         << " private_edge_campaign_hits=" << engine.private_edge_campaign_hits
         << " nested_field_build=" << engine.field_build_seconds << " nested_field_query=" << engine.field_query_seconds
         << " nested_cache=" << engine.field_cache_seconds << " nested_search=" << engine.field_search_seconds
         << " nested_interpolation=" << engine.field_interpolation_seconds
         << " nested_composition=" << engine.field_composition_seconds
         << " nested_edge_geometry=" << engine.edge_geometry_seconds
         << " nested_edge_composition=" << engine.edge_composition_seconds << " nested_edge=" << engine.edge_seconds
         << " nested_edge_search=" << engine.edge_search_seconds << " nested_edge_trace=" << engine.edge_trace_seconds
         << " nested_edge_integral=" << engine.edge_integral_seconds << " donor_reused=" << engine.donor_reused
         << " edge_separated=" << engine.edge_separated << " metric_support_cells=" << engine.metric_support_cells
         << " metric_support_edges=" << engine.metric_support_edges
         << " metric_support_skipped=" << engine.metric_support_skipped
         << " metric_support_duplicates=" << engine.metric_support_duplicates
         << " collapse_constraint_filtered=" << engine.collapse_constraint_filtered
         << " donor_batches=" << engine.donor_batches << " donor_candidates=" << engine.donor_candidates
         << " donor_filtered=" << engine.donor_filtered << " donor_box_filtered=" << engine.donor_box_filtered
         << " donor_payloads=" << engine.donor_payloads << " nested_donor_search=" << engine.donor_search_seconds
         << " nested_donor_filter=" << engine.donor_filter_seconds << " batch_retries=" << engine.batch_retries
         << " query_retries=" << engine.query_import_retries
         << " private_query_retries=" << engine.private_query_retries << " native_bound=" << engine.maximum_work_bytes
         << '\n';
}

void WriteRemeshEngineDetail(std::ostream& detail, const char* stage, const World& parent, const RemeshStats& stats,
                             const EngineStats& engine) {
  WriteRemeshDetailPrefix(detail, stage, parent, stats);
  WriteEngineRoundWork(detail, engine);
  WriteEngineOperatorWork(detail, engine);
  WriteEngineRejection(detail, engine);
  WriteEngineTopologyWork(detail, engine);
  WriteEngineFieldWork(detail, engine);
}

void ReportRemeshDetails(const EngineOptions& control, const World& parent, const RemeshStats& stats,
                         const SU2NativeGeometry::AtlasBudget& target_event_budget) {
  std::ostringstream detail;
  detail << std::setprecision(17);
  WriteRemeshBridgeDetail(detail, "import", parent, stats, stats.imported);
  WriteRemeshBridgeDetail(detail, "weights", parent, stats, stats.weights);
  WriteRemeshBridgeDetail(detail, "return", parent, stats, stats.returned);
  WriteRemeshBridgeDetail(detail, "reassessment", parent, stats, stats.reassessment);
  WriteRemeshBridgeDetail(detail, "parameter_return", parent, stats, stats.parameter_return);
  WriteRemeshSourceDetail(detail, parent, stats);
  WriteRemeshPartitionDetail(detail, parent, stats);
  if (stats.construction) WriteRemeshEngineDetail(detail, "construction", parent, stats, stats.construction_stats);
  WriteRemeshEngineDetail(detail, "refine", parent, stats, stats.refine);
  WriteRemeshEngineDetail(detail, "coarsen", parent, stats, stats.coarsen);
  if (control.geometry_workspace_bytes)
    detail << " target_geometry_work=" << target_event_budget.work
           << " target_geometry_reserved_peak_bytes=" << target_event_budget.peak_bytes
           << " target_geometry_scratch=" << control.geometry_workspace_bytes;
  SU2Native3D::ProfileRow3D(detail.str(), parent.comm);
}

void ReportRemesh(const CConfig& config, const EngineOptions& control, const World& parent, RemeshStats& stats,
                  const std::string& reason, const SU2NativeGeometry::AtlasBudget& target_event_budget) {
  const auto report_started = parent.seconds();
  std::ostringstream row;
  WriteRemeshSummary(row, parent, stats);
  const auto text = row.str();
  const auto all = CPassiveComm::Allgatherv(std::vector<char>(text.begin(), text.end()), nullptr, parent.comm);
  if (!parent.rank) {
    std::cout.write(all.data(), all.size());
    if (!stats.complete)
      std::cout << "Native3D rejected event: " << reason << "; accepted CFD mesh/solution retained.\n";
  }
  if (control.timings) ReportRemeshDetails(control, parent, stats, target_event_budget);
  if (parent.rank == 0 && stats.normalization_stages) {
    std::cout << "Native3D frozen stages: count=" << stats.normalization_stages
              << " shared_work=" << stats.cumulative_work << " correction_trials=" << stats.normalization.trials
              << " correction=" << stats.normalization.changed
              << " correction_status=" << unsigned(stats.normalization.status)
              << " correction_density_queries=" << stats.normalization.density_queries
              << " correction_geometry_queries=" << stats.normalization.geometry_queries
              << " correction_seconds=" << stats.normalization.seconds << " reason=" << stats.normalization.reason
              << "\n";
  }
  if (parent.rank == 0 && stats.complexity.estimated)
    std::cout << std::setprecision(17) << "Native3D candidate complexity: value=" << stats.complexity.value
              << " uncertainty_indicator=" << stats.complexity.uncertainty
              << " requested_budget=" << config.GetAdap_Complexity() << " relative_budget_discrepancy="
              << (stats.complexity.value / SU2_TYPE::GetValue(config.GetAdap_Complexity()) - 1)
              << " standard_error=" << stats.complexity.standard_error
              << " block_standard_error=" << stats.complexity.block_standard_error
              << " samples=" << stats.complexity.samples
              << " current_nodal_field=1 proven_bound=0 normalization_correction=" << stats.normalization.changed
              << "\n";
  else if (parent.rank == 0 && stats.complete)
    std::cout << "Native3D candidate complexity unresolved: " << stats.complexity.reason << "\n";
  stats.report_seconds = parent.seconds() - report_started;
}

}  // namespace
