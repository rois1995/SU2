# A2 state-only preparation (not applied)

Input: the accepted F3b prefix through resolved geometry, with the original private
by-value EngineOptions copy and external World/accepted mesh/reference untouched.
Output: the same live RAII objects at the same original sequence points, now held
by EngineSetup. No collective/logic extraction belongs to A2.

Interleaved declarations prevent a constructor from eagerly building all RAII
members: SetShapeAdmission, the Session ownership accounting, start clock, atlas
budget fields and shared geometry reservation must retain their order. Use stable
in-place storage with construction at each original declaration. EngineSetup's
copy/move operations must be deleted. The admission guard is first and restored
last; geometry lease/atlas/private state are released before RestoreBudgetWindow;
Session is finalized at its old relative point. A temporary aggregate guard must
not restore the budget early when emplaced: construction must be in place, with no
extra guard destruction. Do not change the early rejected return into finish().

The owning EngineSetup object is declared at the original first guard position.
CavityScope and loop timers remain separate locals under A4; no eager construction.
Private file pairs remain internal to the original translation unit/namespace so
Selection and MeshOptimizer stay unchanged.

Acceptance: A1 must already pass G1/G2. A2 builds once, runs G1/G2, and is committed
only after both pass. Phase A still needs G3/G4/GQ/G5; A2 alone is not accepted N4.
This document is preparation only: no production source has been edited.
