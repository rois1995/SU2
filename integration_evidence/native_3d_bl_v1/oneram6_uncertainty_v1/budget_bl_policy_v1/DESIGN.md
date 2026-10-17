# Opt-in budget-aware outer-normal BL policy

Connected implementation; see README and qualification receipts for tested scope.
Native3D auto-selection only; 2D/MMG keep existing behavior and reject the new opt-in explicitly.

ADAP_BL_BUDGET defaults NO. YES requires explicit positive physical protected
ADAP_BL_PROTECT_DISTANCE >= every h0 and ADAP_BL_OUTER_GROWTH_MAX >= every original
growth. No automatic physical-accuracy defaults. One shared lambda in[0,1] blends
original and maximum normal slopes only beyond the protected distance, anchored
continuously to original spacing there. Tangents/geometry/first height/support/
thickness/fade/finer sensor intersections stay unchanged. No scenario dispatch.

At most9 ordered lambda samples at1/8 spacing, each on the complete original
unclipped bounded/graded sensor-family coarsest endpoint and composed BL. No
monotonicity assumption; first uncertain candidate stops as integration_limited,
rather than skipping to more relaxation. Incompatible means estimated conflict
for sampled permitted profiles, not proof across the continuous interval. Report
mandatory protected/sensor density contribution separately with heuristic error.
The existing bounded original-family scale solver runs only for the chosen profile.
Freeze before all operations; no profile reselection during normalization correction.

Every trial broadcasts lambda/version, validates agreement, reduces uniquely owned
cost/absolute uncertainty and uses the existing bounded integration. No extra whole-
mesh gather. Existing donor support/proposal points survive profile changes; cached
wall samples are explicitly rebuilt and counted once per version, and strict cell
rules require that version. External integration workspaces and stale wall samples
reject mismatches. Profile identity is retained in the original metric family and
its config compatibility. Remesher refuses a missing/unfrozen selected profile.

When the coarsest estimate overlaps budget, enabled normalization retains that
original field provisionally with IntegrationLimited status and one normalization
trial. It does not grade finer trial sensors to chase an unresolved root. Reassessment
uses the same original donor family at an explicit stage; all acceptance gates stay.

The ordered scan gives the least sampled relaxation at resolution1/8. It cannot
prove a continuous minimum or exclude a narrow unsampled nonmonotone feasible
interval. Indicators remain heuristic; partial strict protected-band accuracy is
explicitly unknown (infinite), not inferred from full-field convergence.

Qualified controlled workflows and preserved failures are recorded in README.
Pending full-goal work includes global-error calibration/allocation, complete
curved/interacting policy workflows, accepted M6 construction and full-event
performance. No speedup claim follows from a cheap rejected budget or selected field.
