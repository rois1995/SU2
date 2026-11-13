import SU2Interior.Generated

noncomputable section
namespace SU2Interior
open SU2 SU2Source SU2Boundary SU2BoundaryMemory SU2Euler

def CappedStoreContract (code : Bool) : Prop :=
  ∀ (raw cap : ℝ) (m : Mem), sourceTimeStore code (0 : Fin 1) raw cap m = put m (4,0) (min raw cap)

theorem time_store_positive : CappedStoreContract Generated.time_original := by intro raw cap m; rfl

theorem time_store_refactor : CappedStoreContract Generated.time_refactored := by intro raw cap m; rfl

theorem time_uncapped_rejected : ¬ CappedStoreContract Generated.time_uncapped := by
  intro h
  have witness := congrFun (h 3 1 (fun _ => some 0)) (4,0)
  norm_num [Generated.time_uncapped,sourceTimeStore,put,Function.update_apply] at witness

def ScatterMemoryContract (code : ScatterCode) : Prop :=
  ((sourceScatterComponents code (0 : Fin 2) 1 (fun _ => 1) flowColumns (fun _ => some 0)).bind
    (fun m => m (flowAddr 5 (1 : Fin 2) 0))) = some (-1)

theorem scatter_memory_positive : ScatterMemoryContract Generated.scatter_original := by
  have v3 : (3 : Fin 4).val = 3 := rfl
  norm_num [v3,ScatterMemoryContract,Generated.scatter_original,sourceScatterComponents,flowColumns,
    opReal,flowAddr,offset,put,Function.update_apply,Fin.ofNat',Fin.val_natCast]

theorem scatter_memory_wrong_sign_rejected : ¬ ScatterMemoryContract Generated.scatter_wrongSign := by
  have v3 : (3 : Fin 4).val = 3 := rfl
  norm_num [v3,ScatterMemoryContract,Generated.scatter_wrongSign,sourceScatterComponents,flowColumns,
    opReal,flowAddr,offset,put,Function.update_apply,Fin.ofNat',Fin.val_natCast]

theorem scatter_memory_refactor {M : Nat} (p q : Fin M) (f : Flux4) (cs : List (Fin 4)) (m : Mem) :
    sourceScatterComponents Generated.scatter_refactored p q f cs m = executeScatter p q f cs m :=
  source_scatter_components_correct p q f cs m

/-- Wrong active padding repeats the last edge: this is a model-level traversal
counterexample. The native source mutation separately checks a compiled wrong gather. -/
def activatePadding {E : Nat} (lanes : List (Lane E)) : List (Lane E) :=
  lanes.map (fun lane => {lane with active := true})

def LastEdgeOnce (lanes : List (Lane 3)) : Prop := (activeEdges lanes).count 2=1

theorem packet_positive : LastEdgeOnce (allLanes 1 3) := by simp [LastEdgeOnce,all_active_edges,naturalEdges,List.ofFn_succ]

theorem active_padding_rejected : ¬ LastEdgeOnce (activatePadding (allLanes 1 3)) := by simp [LastEdgeOnce,activatePadding,allLanes,naturalEdges,List.ofFn_succ,packets,packetLanes,Packet.slots,Packet.values,activeEdges]

end SU2Interior
