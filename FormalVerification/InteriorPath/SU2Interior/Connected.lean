import SU2Interior.Composition

noncomputable section
namespace SU2Interior
open SU2 SU2Source SU2Euler SU2Boundary SU2BoundaryMemory SU2Initialization SU2Preprocessing
variable {M E K : Nat} {vertices : Fin K → Nat}

/-- Selected CIntegration interstitial dispatch: inviscid virtual viscous call is empty;
all Source_Residual guards are false, no fluid-interface marker, no dual time/Giles.
The C++ scratch allocation/free and pointer-table load remain outside this memory model. -/
structure InterstitialConfig where
  bodyForce : Bool := false
  rotatingFrame : Bool := false
  axisymmetric : Bool := false
  gravity : Bool := false
  harmonicBalance : Bool := false
  vorticity : Bool := false
  manufactured : Bool := false
  pythonSource : Bool := false
  dualTime : Bool := false
  fluidInterface : Bool := false
  giles : Bool := false
  deriving DecidableEq

def InterstitialConfig.enabled (c : InterstitialConfig) : Bool :=
  c.bodyForce || c.rotatingFrame || c.axisymmetric || c.gravity || c.harmonicBalance ||
  c.vorticity || c.manufactured || c.pythonSource || c.dualTime || c.fluidInterface || c.giles

def selectedInterstitial : InterstitialConfig := {}

/-- No unconstrained finish function: unsupported configured operations explicitly fail. -/
def executeInterstitial (c : InterstitialConfig) (m : Mem) : Option Mem :=
  if c.enabled then none else some m

theorem selected_interstitial_frame (m : Mem) :
    executeInterstitial selectedInterstitial m=some m ∧ Frame [] m m := ⟨rfl,frame_refl [] m⟩

def executeIteration (K w : Nat) {vertices : Fin K → Nat} (g : IdealGas) (geo : Geometry M E)
    (bs : List (BoundaryFace M)) (cfl cap : ℝ) (counter : Fin E → Nat)
    (far : List (FarSite M K vertices)) (walls : List (WallVisit M)) (free : Primitive 2)
    (s0 : ControlState) (prior : TimeStats) : Option Mem := do
  let processed ← executeToBoundary K w g geo bs cfl cap counter s0 prior
  let mi ← executeInterstitial selectedInterstitial processed.control.machine.memory
  let mb ← executeBoundary M farCode wallCode g free far walls mi
  executeLoop (N := M) M SU2Boundary.Generated.volume_original SU2Boundary.Generated.periodic_original
    SU2Boundary.Generated.timestep_original SU2Boundary.Generated.points_original
    SU2Boundary.Generated.components_original mb

/-- Connected exact-real execution through the complete explicit update. The only memory
premises concern the pre-preprocessing input. Geometry construction, source-to-model
heap decoding and complete independent Roe/far-field refinement are NOT discharged. -/
theorem connected_iteration (w : Nat) (g : IdealGas) (geo : Geometry M E)
    (bs : List (BoundaryFace M)) (ready : GeometryReady geo) (cfl cap : ℝ)
    (counter : Fin E → Nat) (s0 : ControlState) (prior : TimeStats) (d : Data M)
    (T : Residual M 2) (cache : Nat → Option Ghost) (vol : Fin M → ℝ)
    (hd : DataRep s0.machine d) (hf : FluidReady g s0.machine.fluid)
    (ht : Stored 6 s0.machine.memory (fun c => T c.1 (channel c.2)))
    (hc : CacheRep vertices s0.machine.memory cache)
    (hv : ∀ p, s0.machine.memory (2,p.val)=some (vol p))
    (hper : ∀ p : Fin M, s0.machine.memory (3,p.val)=some 0)
    (far : List (FarSite M K vertices)) (walls : List (WallVisit M)) (free : Primitive 2)
    (bounds : Bounds M M K vertices)
    (farReady : FarReady g free (far.map (farVisit (modelPoints g (points M) d 0).1.primitive))) :
    let data := (modelPoints g (points M) d 0).1
    let si := interiorState g geo data.primitive ready (snapshotState (preprocessingState data T cache))
    let sb := boundarySpec g free (far.map (farVisit data.primitive)) walls si
    let dt := fun p => localPseudoStep (vol p) cfl (eigenvalue data.primitive geo bs p) cap
    ∃ processed mb mu,
      executeToBoundary K w g geo bs cfl cap counter s0 prior=some processed ∧
      executeBoundary M farCode wallCode g free far walls processed.control.machine.memory=some mb ∧
      InputsReady mb (ownedState (Nat.le_refl M) sb) vol (fun _ => 0) dt ∧
      executeIteration K w g geo bs cfl cap counter far walls free s0 prior=some mu ∧
      LoopContract (ownedState (Nat.le_refl M) sb) vol dt mb mu ∧
      Frame (prefixFootprint K w M E geo bs ++ boundaryFootprint far walls ++
        (visits M).map destination) s0.machine.memory mu := by
  dsimp only
  obtain ⟨processed,ep,hp,hg,_,_,_,fp⟩ := preprocessing_to_boundary_memory w g geo bs ready cfl cap counter s0 prior d T cache vol hd hf ht hc hv hper
  obtain ⟨mb,mu,eb,_,hr,eu,hu,fb⟩ := boundary_to_complete_update (Nat.le_refl M) bounds g free _ far walls _ processed.control.machine.memory vol _ hp hg farReady
  refine ⟨processed,mb,mu,ep,eb,?_,?_,?_,?_⟩
  · exact hr
  · simp only [executeIteration,ep,Option.bind_eq_bind,Option.some_bind,
      executeInterstitial,selectedInterstitial,InterstitialConfig.enabled,Bool.or_self,Bool.false_eq_true,
      reduceIte,eb,eu]
  · exact hu
  · simpa only [List.append_assoc] using frame_trans fp fb

end SU2Interior
