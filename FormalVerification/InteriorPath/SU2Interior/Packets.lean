import SU2Interior.Scatter

set_option maxRecDepth 100000
set_option maxHeartbeats 8000000
noncomputable section
namespace SU2Interior
open SU2 SU2Source SU2Boundary SU2BoundaryMemory SU2Initialization SU2Preprocessing
variable {M E K : Nat} {vertices : Fin K → Nat}

structure Packet (α : Type) where
  head : α
  tail : List α

def Packet.values {α : Type} (p : Packet α) : List α := p.head :: p.tail

/-- Width is w+1, representing every possible positive Double::Size. This is the
source k+=width loop split into nonempty chunks, before padding gathers. -/
def packets {α : Type} (w : Nat) : (xs : List α) → List (Packet α)
  | [] => []
  | x :: xs => ⟨x,xs.take w⟩ :: packets w (xs.drop w)
termination_by xs => xs.length
decreasing_by simp_wf; omega

theorem packets_flatten {α : Type} (w : Nat) (xs : List α) :
    (packets w xs).flatMap Packet.values = xs := by
  cases xs with
  | nil => simp [packets]
  | cons x xs =>
    rw [packets,List.flatMap_cons,packets_flatten w (xs.drop w)]
    simp [Packet.values,List.cons_append,List.take_append_drop]
termination_by xs.length
decreasing_by simp_wf; omega

theorem packet_length_bound {α : Type} (w : Nat) (xs : List α) (p : Packet α)
    (member : p ∈ packets w xs) : p.values.length ≤ w+1 := by
  induction xs using packets.induct w with
  | case1 => simp [packets] at member
  | case2 x xs ih =>
    rw [packets] at member
    rcases List.mem_cons.mp member with rfl | rest
    · simp only [Packet.values,List.length_cons,List.length_take]
      omega
    · exact ih rest

def Packet.slots {α : Type} (w : Nat) (p : Packet α) : List (Option α) :=
  p.values.map some ++ List.replicate (w+1-p.values.length) none

structure Lane (E : Nat) where
  edge : Fin E
  active : Bool

def packetLanes (w : Nat) (p : Packet (Fin E)) : List (Lane E) :=
  (p.slots w).map (fun slot => match slot with | some e => ⟨e,true⟩ | none => ⟨p.head,false⟩)

def activeEdges (lanes : List (Lane E)) : List (Fin E) :=
  lanes.filterMap (fun lane => if lane.active then some lane.edge else none)

theorem packet_active_edges (w : Nat) (p : Packet (Fin E)) : activeEdges (packetLanes w p) = p.values := by
  simp [activeEdges,packetLanes,Packet.slots,List.filterMap_map,List.map_map,Function.comp_def]

theorem packet_width (w : Nat) (p : Packet (Fin E)) (bound : p.values.length≤w+1) :
    (packetLanes w p).length=w+1 := by
  simp only [packetLanes,List.length_map,Packet.slots,List.length_append,List.length_map,List.length_replicate]
  omega

def allLanes (w E : Nat) : List (Lane E) :=
  (packets w (naturalEdges E)).flatMap (packetLanes w)

theorem active_edges_append (a b : List (Lane E)) : activeEdges (a++b) = activeEdges a ++ activeEdges b := by
  simp [activeEdges]

theorem active_packets (w : Nat) (ps : List (Packet (Fin E))) :
    activeEdges (ps.flatMap (packetLanes w)) = ps.flatMap Packet.values := by
  induction ps with
  | nil => rfl
  | cons p ps ih => simp only [List.flatMap_cons,active_edges_append,packet_active_edges,ih]

theorem all_active_edges (w E : Nat) : activeEdges (allLanes w E) = naturalEdges E := by
  rw [allLanes,active_packets,packets_flatten]

theorem active_traversal_unique (w E : Nat) : (activeEdges (allLanes w E)).Nodup := by
  rw [all_active_edges]; exact natural_edge_unique E

theorem active_traversal_coverage (w : Nat) (e : Fin E) : e ∈ activeEdges (allLanes w E) := by
  rw [all_active_edges]; exact natural_edge_coverage e

/-- Gather/compute every lane, including inactive repeats of the packet base edge,
BEFORE any lane writes. Primitive data and numerical scratch pointer semantics are separate. -/
def gatherFluxes (g : IdealGas) (geo : Geometry M E) : List (Lane E) → Mem → Option (List (Lane E × Flux4))
  | [], _ => some []
  | lane :: lanes, m => do
    let f ← executeFlux g geo lane.edge m
    let rest ← gatherFluxes g geo lanes m
    return (lane,f) :: rest

def executeComputedLanes (geo : Geometry M E) : List (Lane E × Flux4) → Mem → Option Mem
  | [], m => some m
  | (lane,f) :: lanes, m => do
    let m ← if lane.active then executeScatter (geo.left lane.edge) (geo.right lane.edge) f flowColumns m else pure m
    executeComputedLanes geo lanes m

def executePacket (w : Nat) (g : IdealGas) (geo : Geometry M E) (p : Packet (Fin E)) (m : Mem) : Option Mem := do
  let computed ← gatherFluxes g geo (packetLanes w p) m
  executeComputedLanes geo computed m

def executePackets (w : Nat) (g : IdealGas) (geo : Geometry M E) : List (Packet (Fin E)) → Mem → Option Mem
  | [], m => some m
  | p :: ps, m => do
    let m ← executePacket w g geo p m
    executePackets w g geo ps m

/-- Source diagnostics count every gathered lane, including inactive duplicates.
First-order reconstruction does not update edge counters; initialized zeros imply zero. -/
def reconstructionCount (lanes : List (Lane E)) (counter : Fin E → Nat) : Nat :=
  (lanes.map (fun lane => if counter lane.edge > 0 then 1 else 0)).sum

def executeInterior (w : Nat) (g : IdealGas) (geo : Geometry M E) (counter : Fin E → Nat) (m : Mem) : Option (Mem × Nat) := do
  let m ← executePackets w g geo (packets w (naturalEdges E)) m
  return (m,reconstructionCount (allLanes w E) counter)

theorem zero_reconstruction_count (w E : Nat) : reconstructionCount (allLanes w E) (fun _ => 0)=0 := by
  simp [reconstructionCount]

theorem gather_fluxes_correct (g : IdealGas) (geo : Geometry M E) (lanes : List (Lane E))
    (m : Mem) (P : Primitives M) (h : PrimitiveRep m P) :
    gatherFluxes g geo lanes m = some (lanes.map (fun lane => (lane,edgeFlux g geo P lane.edge))) := by
  induction lanes with
  | nil => rfl
  | cons lane lanes ih => simp [gatherFluxes,execute_flux_correct g geo lane.edge m P h,ih]

end SU2Interior
