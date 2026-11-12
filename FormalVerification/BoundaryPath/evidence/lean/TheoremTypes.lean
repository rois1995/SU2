import SU2Boundary
set_option pp.universes true
#check SU2Boundary.projection_correct
#print axioms SU2Boundary.projection_correct
#check SU2Boundary.wall_source_contract
#print axioms SU2Boundary.wall_source_contract
#check SU2Boundary.far_reconstruction_correct
#print axioms SU2Boundary.far_reconstruction_correct
#check SU2Boundary.far_normal_velocity
#print axioms SU2Boundary.far_normal_velocity
#check SU2Boundary.far_tangential_velocity
#print axioms SU2Boundary.far_tangential_velocity
#check SU2Boundary.riemann_reconstruction
#print axioms SU2Boundary.riemann_reconstruction
#check SU2Boundary.dissipation_loop_correct
#print axioms SU2Boundary.dissipation_loop_correct
#check SU2Boundary.far_primitive_admissible
#print axioms SU2Boundary.far_primitive_admissible
#check SU2Boundary.source_conservative_correct
#print axioms SU2Boundary.source_conservative_correct
#check SU2Boundary.stored_flux_correct
#print axioms SU2Boundary.stored_flux_correct
#check SU2Boundary.roe_model_refinement
#print axioms SU2Boundary.roe_model_refinement
#check SU2Boundary.far_source_call_contract
#print axioms SU2Boundary.far_source_call_contract
#check SU2Boundary.boundary_refinement
#print axioms SU2Boundary.boundary_refinement
#check SU2Boundary.far_frame
#print axioms SU2Boundary.far_frame
#check SU2Boundary.wall_frame
#print axioms SU2Boundary.wall_frame
#check SU2Boundary.wall_three_constraints
#print axioms SU2Boundary.wall_three_constraints
#check SU2Boundary.wall_old_admissible
#print axioms SU2Boundary.wall_old_admissible
#check SU2Boundary.wall_mass_energy_frame
#print axioms SU2Boundary.wall_mass_energy_frame
#check SU2Boundary.projected_update_impermeable
#print axioms SU2Boundary.projected_update_impermeable
#check SU2Boundary.boundary_current_frame
#print axioms SU2Boundary.boundary_current_frame
#check SU2Boundary.unowned_far_frame
#print axioms SU2Boundary.unowned_far_frame
#check SU2Boundary.unowned_wall_frame
#print axioms SU2Boundary.unowned_wall_frame
#check SU2Boundary.far_add_positive
#print axioms SU2Boundary.far_add_positive
#check SU2Boundary.far_add_mutant_rejected
#print axioms SU2Boundary.far_add_mutant_rejected
#check SU2Boundary.wall_positive
#print axioms SU2Boundary.wall_positive
#check SU2Boundary.wall_old_mutant_rejected
#print axioms SU2Boundary.wall_old_mutant_rejected
#check SU2Boundary.wall_residual_mutant_rejected
#print axioms SU2Boundary.wall_residual_mutant_rejected
#check SU2Boundary.wall_truncation_mutant_rejected
#print axioms SU2Boundary.wall_truncation_mutant_rejected
#check SU2Boundary.missing_cell_rejected
#print axioms SU2Boundary.missing_cell_rejected
#check SU2Boundary.loop_ready
#print axioms SU2Boundary.loop_ready
#check SU2Boundary.loop_positive
#print axioms SU2Boundary.loop_positive
#check SU2Boundary.loop_refactor_positive
#print axioms SU2Boundary.loop_refactor_positive
#check SU2Boundary.point_mutant_rejected
#print axioms SU2Boundary.point_mutant_rejected
#check SU2Boundary.component_mutant_rejected
#print axioms SU2Boundary.component_mutant_rejected
#check SU2Boundary.timestep_positive
#print axioms SU2Boundary.timestep_positive
#check SU2Boundary.timestep_mutant_rejected
#print axioms SU2Boundary.timestep_mutant_rejected
#check SU2Boundary.reject_unknown_projection
#print axioms SU2Boundary.reject_unknown_projection
#check SU2Boundary.reject_extra_loop_condition
#print axioms SU2Boundary.reject_extra_loop_condition
#check SU2Boundary.reject_extra_getter_statement
#print axioms SU2Boundary.reject_extra_getter_statement
#check SU2Boundary.Generated.wallOld_original_parsed
#print axioms SU2Boundary.Generated.wallOld_original_parsed
#check SU2Boundary.Generated.wallOld_wrongSign_parsed
#print axioms SU2Boundary.Generated.wallOld_wrongSign_parsed
#check SU2Boundary.Generated.wallRes_original_parsed
#print axioms SU2Boundary.Generated.wallRes_original_parsed
#check SU2Boundary.Generated.wallRes_wrongSign_parsed
#print axioms SU2Boundary.Generated.wallRes_wrongSign_parsed
#check SU2Boundary.Generated.wallTrunc_original_parsed
#print axioms SU2Boundary.Generated.wallTrunc_original_parsed
#check SU2Boundary.Generated.wallTrunc_wrongSign_parsed
#print axioms SU2Boundary.Generated.wallTrunc_wrongSign_parsed
#check SU2Boundary.Generated.farAdd_original_parsed
#print axioms SU2Boundary.Generated.farAdd_original_parsed
#check SU2Boundary.Generated.farAdd_wrongSign_parsed
#print axioms SU2Boundary.Generated.farAdd_wrongSign_parsed
#check SU2Boundary.Generated.points_original_parsed
#print axioms SU2Boundary.Generated.points_original_parsed
#check SU2Boundary.Generated.points_refactored_parsed
#print axioms SU2Boundary.Generated.points_refactored_parsed
#check SU2Boundary.Generated.points_skipLast_parsed
#print axioms SU2Boundary.Generated.points_skipLast_parsed
#check SU2Boundary.Generated.components_original_parsed
#print axioms SU2Boundary.Generated.components_original_parsed
#check SU2Boundary.Generated.components_refactored_parsed
#print axioms SU2Boundary.Generated.components_refactored_parsed
#check SU2Boundary.Generated.components_skipLast_parsed
#print axioms SU2Boundary.Generated.components_skipLast_parsed
#check SU2Boundary.Generated.volume_original_parsed
#print axioms SU2Boundary.Generated.volume_original_parsed
#check SU2Boundary.Generated.volumeConst_original_parsed
#print axioms SU2Boundary.Generated.volumeConst_original_parsed
#check SU2Boundary.Generated.periodic_original_parsed
#print axioms SU2Boundary.Generated.periodic_original_parsed
#check SU2Boundary.Generated.periodicConst_original_parsed
#print axioms SU2Boundary.Generated.periodicConst_original_parsed
#check SU2Boundary.Generated.timestep_original_parsed
#print axioms SU2Boundary.Generated.timestep_original_parsed
#check SU2Boundary.Generated.timestep_wrongField_parsed
#print axioms SU2Boundary.Generated.timestep_wrongField_parsed
#check SU2Boundary.visits_coverage
#print axioms SU2Boundary.visits_coverage
#check SU2Boundary.visits_order
#print axioms SU2Boundary.visits_order
#check SU2Boundary.visits_no_duplicates
#print axioms SU2Boundary.visits_no_duplicates
#check SU2Boundary.offset_injective
#print axioms SU2Boundary.offset_injective
#check SU2Boundary.destination_injective
#print axioms SU2Boundary.destination_injective
#check SU2Boundary.iteration_bounds
#print axioms SU2Boundary.iteration_bounds
#check SU2Boundary.getter_index_bounds
#print axioms SU2Boundary.getter_index_bounds
#check SU2Boundary.selected_visits_complete
#print axioms SU2Boundary.selected_visits_complete
#check SU2Boundary.loop_refactor_preserves_order
#print axioms SU2Boundary.loop_refactor_preserves_order
#check SU2Boundary.getter_connection
#print axioms SU2Boundary.getter_connection
#check SU2Boundary.getter_const_overloads
#print axioms SU2Boundary.getter_const_overloads
#check SU2Boundary.original_loop_execution
#print axioms SU2Boundary.original_loop_execution
#check SU2Boundary.inputs_after_store
#print axioms SU2Boundary.inputs_after_store
#check SU2Boundary.execute_cell_correct
#print axioms SU2Boundary.execute_cell_correct
#check SU2Boundary.execute_cells_correct
#print axioms SU2Boundary.execute_cells_correct
#check SU2Boundary.writes_value
#print axioms SU2Boundary.writes_value
#check SU2Boundary.writes_frame
#print axioms SU2Boundary.writes_frame
#check SU2Boundary.complete_update_loop
#print axioms SU2Boundary.complete_update_loop
#check SU2Boundary.boundary_and_complete_update
#print axioms SU2Boundary.boundary_and_complete_update
#check SU2Boundary.assembled_boundary_complete_update
#print axioms SU2Boundary.assembled_boundary_complete_update
#check SU2Boundary.components_roundtrip
#print axioms SU2Boundary.components_roundtrip
#check SU2Boundary.channel_index
#print axioms SU2Boundary.channel_index
