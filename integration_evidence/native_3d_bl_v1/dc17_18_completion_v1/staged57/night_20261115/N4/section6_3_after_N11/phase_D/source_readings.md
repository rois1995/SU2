# Phase D source readings

D follows the whole-cut source check and the passed build570 phase C. Work order 2 supersedes the older reference readings in that check.

1. D1 streamed is set from ready after ApplyPrimaryOperator votes and returns false when the vote fails. The only caller of PrepareMeasurement is below that successful return. Keep the empty DonorPatch for the existing admission calculation. Keep ReserveOptimization(false), Maximum(repair_budget > repair_work) and ReleaseOptimization in order. The MetricNormalization scope, FieldCounts and reconstruction timer subtractions live outside the dead branch and remain at the same site.
2. The sole SlotCommit call is below the private repair path's continue or break. Remove the false repairing alternatives there. Keep the live repair_approved and moved routes and all collective checks on the remaining path. The private batch commit carries the live repair state.
3. D2 must preserve the order and lifetime of ContextValue, budget/window, timer, cavity and setup state across exits.
4. D1 uses the explicit G1/G2/G3/GQ schedule. D2 uses G1 and G2 under C §5, plus the integrated G3 and GQ where the extracted round and cavity code requires them. G4 and the shared-code checks qualify the integrated D state. D3 uses the accepted six G5 payload references and the required isolated restart.
