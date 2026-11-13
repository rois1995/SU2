import SU2Interior.Packets

set_option maxRecDepth 100000
set_option maxHeartbeats 8000000
noncomputable section
namespace SU2Interior
open SU2 SU2Source SU2Boundary SU2BoundaryMemory SU2Initialization SU2Preprocessing
variable {M E K : Nat} {vertices : Fin K → Nat}

def laneState (g : IdealGas) (geo : Geometry M E) (P : Primitives M) (lane : Lane E) (s : BoundaryState M) : BoundaryState M :=
  if lane.active then scatterState (geo.left lane.edge) (geo.right lane.edge) (edgeFlux g geo P lane.edge) s else s

def laneFootprint (geo : Geometry M E) (lane : Lane E) : List Address :=
  if lane.active then scatterFootprint (geo.left lane.edge) (geo.right lane.edge) flowColumns else []

def lanesState (g : IdealGas) (geo : Geometry M E) (P : Primitives M)
    (lanes : List (Lane E)) (s : BoundaryState M) : BoundaryState M :=
  lanes.foldl (fun state lane => laneState g geo P lane state) s

theorem computed_lanes_refine (g : IdealGas) (geo : Geometry M E) (P : Primitives M)
    (lanes : List (Lane E)) (m : Mem) (s : BoundaryState M) (h : BoundaryRep vertices m s P) :
    ∃ mf, executeComputedLanes geo (lanes.map (fun lane => (lane,edgeFlux g geo P lane.edge))) m = some mf ∧
      BoundaryRep vertices mf (lanesState g geo P lanes s) P ∧
      Frame (lanes.flatMap (laneFootprint geo)) m mf := by
  induction lanes generalizing m s with
  | nil => exact ⟨m,rfl,h,frame_refl _ _⟩
  | cons lane lanes ih =>
    by_cases active : lane.active=true
    · obtain ⟨mn,en,hn,fn⟩ := scatter_memory_refines (geo.left lane.edge) (geo.right lane.edge)
        (endpoint_distinct geo lane.edge) (edgeFlux g geo P lane.edge) m s P h
      obtain ⟨mf,ef,hf,ff⟩ := ih mn (scatterState (geo.left lane.edge) (geo.right lane.edge) (edgeFlux g geo P lane.edge) s) hn
      refine ⟨mf,?_,?_,?_⟩
      · simp [executeComputedLanes,active,en,ef]
      · simpa [lanesState,laneState,active] using hf
      · simpa [laneFootprint,active] using frame_trans fn ff
    · obtain ⟨mf,ef,hf,ff⟩ := ih m s h
      refine ⟨mf,?_,?_,?_⟩
      · simpa [executeComputedLanes,active] using ef
      · simpa [lanesState,laneState,active] using hf
      · simpa [laneFootprint,active] using ff

theorem packet_refines (w : Nat) (g : IdealGas) (geo : Geometry M E) (P : Primitives M)
    (packet : Packet (Fin E)) (m : Mem) (s : BoundaryState M) (h : BoundaryRep vertices m s P) :
    ∃ mf, executePacket w g geo packet m = some mf ∧
      BoundaryRep vertices mf (lanesState g geo P (packetLanes w packet) s) P ∧
      Frame ((packetLanes w packet).flatMap (laneFootprint geo)) m mf := by
  obtain ⟨mf,ef,hf,ff⟩ := computed_lanes_refine g geo P (packetLanes w packet) m s h
  refine ⟨mf,?_,hf,ff⟩
  simp only [executePacket,gather_fluxes_correct g geo _ m P h.primitives,
    Option.bind_eq_bind,Option.some_bind,ef]

def packetsState (w : Nat) (g : IdealGas) (geo : Geometry M E) (P : Primitives M)
    (ps : List (Packet (Fin E))) (s : BoundaryState M) : BoundaryState M :=
  ps.foldl (fun state packet => lanesState g geo P (packetLanes w packet) state) s

theorem packets_refine (w : Nat) (g : IdealGas) (geo : Geometry M E) (P : Primitives M)
    (ps : List (Packet (Fin E))) (m : Mem) (s : BoundaryState M) (h : BoundaryRep vertices m s P) :
    ∃ mf, executePackets w g geo ps m = some mf ∧
      BoundaryRep vertices mf (packetsState w g geo P ps s) P ∧
      Frame ((ps.flatMap (packetLanes w)).flatMap (laneFootprint geo)) m mf := by
  induction ps generalizing m s with
  | nil => exact ⟨m,rfl,h,frame_refl _ _⟩
  | cons p ps ih =>
    obtain ⟨mn,en,hn,fn⟩ := packet_refines w g geo P p m s h
    obtain ⟨mf,ef,hf,ff⟩ := ih mn (lanesState g geo P (packetLanes w p) s) hn
    refine ⟨mf,?_,hf,?_⟩
    · simp [executePackets,en,ef]
    · simpa [List.flatMap_append] using frame_trans fn ff

theorem packets_state_lanes (w : Nat) (g : IdealGas) (geo : Geometry M E) (P : Primitives M)
    (ps : List (Packet (Fin E))) (s : BoundaryState M) :
    packetsState w g geo P ps s = lanesState g geo P (ps.flatMap (packetLanes w)) s := by
  induction ps generalizing s with
  | nil => rfl
  | cons p ps ih =>
    simpa [packetsState,lanesState,List.foldl_append] using ih (lanesState g geo P (packetLanes w p) s)

def eventsOf (g : IdealGas) (geo : Geometry M E) (ready : GeometryReady geo) (P : Primitives M)
    (edges : List (Fin E)) : List (Event M 2) :=
  edges.map (fun e => (mathematicalEdge geo ready e,fromComponents (edgeFlux g geo P e)))

theorem lanes_residual_assembly (g : IdealGas) (geo : Geometry M E) (ready : GeometryReady geo)
    (P : Primitives M) (lanes : List (Lane E)) (s : BoundaryState M) :
    (lanesState g geo P lanes s).residual =
      SU2.assembleLoop (eventsOf g geo ready P (activeEdges lanes)) s.residual := by
  induction lanes generalizing s with
  | nil => rfl
  | cons lane lanes ih =>
    by_cases active : lane.active=true
    · simpa [lanesState,laneState,activeEdges,active,eventsOf,SU2.assembleLoop,
        scatter_state_mathematical geo ready] using ih (laneState g geo P lane s)
    · simpa [lanesState,laneState,activeEdges,active] using ih s

theorem lanes_other_fields (g : IdealGas) (geo : Geometry M E) (P : Primitives M)
    (lanes : List (Lane E)) (s : BoundaryState M) :
    (lanesState g geo P lanes s).current=s.current ∧
    (lanesState g geo P lanes s).old=s.old ∧
    (lanesState g geo P lanes s).truncation=s.truncation ∧
    (lanesState g geo P lanes s).cache=s.cache := by
  induction lanes generalizing s with
  | nil => exact ⟨rfl,rfl,rfl,rfl⟩
  | cons lane lanes ih =>
    have rest := ih (laneState g geo P lane s)
    cases flag : lane.active <;> simpa [lanesState,laneState,flag,scatterState] using rest

def interiorState (g : IdealGas) (geo : Geometry M E) (P : Primitives M)
    (ready : GeometryReady geo) (s : BoundaryState M) : BoundaryState M :=
  {s with residual := s.residual+SU2.assemble (eventsOf g geo ready P (naturalEdges E))}

theorem interior_state_correct (w : Nat) (g : IdealGas) (geo : Geometry M E)
    (P : Primitives M) (ready : GeometryReady geo) (s : BoundaryState M) :
    packetsState w g geo P (packets w (naturalEdges E)) s = interiorState g geo P ready s := by
  rw [packets_state_lanes]
  have residual := lanes_residual_assembly g geo ready P (allLanes w E) s
  rw [all_active_edges,SU2.accumulation_correct] at residual
  have rest := lanes_other_fields g geo P (allLanes w E) s
  change lanesState g geo P (allLanes w E) s = interiorState g geo P ready s
  generalize eq : lanesState g geo P (allLanes w E) s = t at residual rest ⊢
  cases s
  cases t
  simp_all only [interiorState,BoundaryState.mk.injEq]

/-- Residuals are derived from genuine gather/compute/scatter execution. The flux is
an explicit implementation model; full independent numerical-Roe refinement is still open. -/
theorem interior_refines (w : Nat) (g : IdealGas) (geo : Geometry M E) (P : Primitives M)
    (ready : GeometryReady geo) (counter : Fin E → Nat) (m : Mem) (s : BoundaryState M)
    (h : BoundaryRep vertices m s P) :
    ∃ mf, executeInterior w g geo counter m = some (mf,reconstructionCount (allLanes w E) counter) ∧
      BoundaryRep vertices mf (interiorState g geo P ready s) P ∧
      Frame ((allLanes w E).flatMap (laneFootprint geo)) m mf := by
  obtain ⟨mf,ef,hf,ff⟩ := packets_refine w g geo P (packets w (naturalEdges E)) m s h
  refine ⟨mf,?_,?_,ff⟩
  · simp [executeInterior,ef]
  · rw [interior_state_correct w g geo P ready s] at hf
    exact hf

/-- Conserves the sum over all allocated points, including halo rows. Owned-only
conservation would require an interface-exchange balance and is not asserted here. -/
theorem interior_conservation (g : IdealGas) (geo : Geometry M E) (P : Primitives M)
    (ready : GeometryReady geo) (s : BoundaryState M) :
    ∑ p, (interiorState g geo P ready s).residual p = ∑ p, s.residual p := by
  simp only [interiorState,Pi.add_apply,Finset.sum_add_distrib]
  rw [SU2.assembly_conservation,add_zero]

theorem interior_protected (geo : Geometry M E) (lanes : List (Lane E)) (a : Address) (ne : a.1≠5) :
    a ∉ lanes.flatMap (laneFootprint geo) := by
  intro mem
  obtain ⟨lane,_,hl⟩ := List.mem_flatMap.mp mem
  by_cases active : lane.active=true
  · exact scatter_protected _ _ a ne (by simpa [laneFootprint,active] using hl)
  · simp [laneFootprint,active] at hl

end SU2Interior
