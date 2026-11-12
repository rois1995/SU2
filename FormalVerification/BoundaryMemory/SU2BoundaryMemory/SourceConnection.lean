import SU2BoundaryMemory.Composition
import SU2BoundaryMemory.Generated

set_option maxRecDepth 100000
set_option maxHeartbeats 4000000
namespace SU2BoundaryMemory
open SU2 SU2Source SU2Boundary
variable {N M K : Nat} {vertices : Fin K → Nat}

/-- Only plans with the same refresh/order switch in the three buffers are in this
restricted interpreter. Other combinations fail; no statements are silently skipped. -/
def wallFrom (block : Nat) (old res trunc : ProjectionPlan) : Option WallCode :=
  if old.refresh = res.refresh ∧ old.refresh = trunc.refresh ∧
      old.reverse = res.reverse ∧ old.reverse = trunc.reverse then
    some ⟨block, old.op, res.op, trunc.op, old.refresh, old.reverse⟩
  else none

def farFrom (extra : Nat) (lookup : CacheMode) (erase : Bool) (op : SU2Euler.Op) : Option FarCode :=
  if extra = 9 ∧ lookup ≠ .shortStride then some ⟨lookup, erase, op⟩
  else if extra = 4 ∧ lookup = .normal then some ⟨.shortStride, erase, op⟩ else none

def sourceWallCode : WallCode :=
  ⟨Generated.oldBinding_original, Generated.wallOld_original.op, Generated.wallRes_original.op,
    Generated.wallTrunc_original.op, Generated.wallOld_original.refresh, Generated.wallOld_original.reverse⟩

def sourceFarCode : FarCode :=
  ⟨if Generated.primitiveExtra_original = 4 then .shortStride else Generated.cacheLookup_original,
    Generated.cacheWrites_original, Generated.setBlock_original⟩

theorem source_old_plan : Generated.wallOld_original = ⟨.sub, false, false⟩ := by decide
theorem source_residual_plan : Generated.wallRes_original = ⟨.sub, false, false⟩ := by decide
theorem source_truncation_plan : Generated.wallTrunc_original = ⟨.sub, false, false⟩ := by decide

theorem source_wall_connection : sourceWallCode = wallCode := by
  have hb : Generated.oldBinding_original = 1 := by decide
  have ho : SU2Boundary.Generated.wallOld_original = .sub := by decide
  have hr : SU2Boundary.Generated.wallRes_original = .sub := by decide
  have ht : SU2Boundary.Generated.wallTrunc_original = .sub := by decide
  simp only [sourceWallCode, source_old_plan, source_residual_plan, source_truncation_plan, hb,
    wallCode, ho, hr, ht]

theorem source_far_connection : sourceFarCode = farCode := by
  have he : Generated.primitiveExtra_original = 9 := by decide
  have hk : Generated.cacheLookup_original = .normal := by decide
  have hw : Generated.cacheWrites_original = false := by decide
  have ha : Generated.setBlock_original = .add := by decide
  have hf : SU2Boundary.Generated.farAdd_original = .add := by decide
  simp [sourceFarCode, he, hk, hw, ha, farCode, hf]

theorem source_wall_supported : wallFrom Generated.oldBinding_original Generated.wallOld_original
    Generated.wallRes_original Generated.wallTrunc_original = some sourceWallCode := by
  simp [wallFrom, sourceWallCode, source_old_plan, source_residual_plan, source_truncation_plan]

theorem source_far_supported : farFrom Generated.primitiveExtra_original Generated.cacheLookup_original
    Generated.cacheWrites_original Generated.setBlock_original = some sourceFarCode := by
  have he : Generated.primitiveExtra_original = 9 := by decide
  have hk : Generated.cacheLookup_original = .normal := by decide
  simp [farFrom, sourceFarCode, he, hk]

/-- Specialization of parsed nDim+9 and the row-pointer multiplication to nDim=2.
Linking actual C++ objects to these separate blocks remains a documented assumption. -/
theorem source_storage_stride : 2 + Generated.primitiveExtra_original = 11 := by decide

theorem source_cache_address (site : CacheSite K vertices) (column : Fin 11) :
    cacheAddr site.marker site.vertex column =
      (9 + site.marker.val, site.vertex.val * (2 + Generated.primitiveExtra_original) + column.val) := rfl

theorem source_primitive_address (p : Fin N) (column : Fin 11) :
    primitiveAddr p column = (8, p.val * (2 + Generated.primitiveExtra_original) + column.val) := rfl

/-- Fail-closed syntax controls, distinct from the numerical mutation controls. -/
theorem extra_statement_rejected : parseOldBinding
    (Generated.oldBinding_originalSource ++ " solutionOld[0] = 9;") = none := by decide

theorem unknown_cache_write_rejected : parseCacheWrites
    (Generated.cacheWrites_originalSource ++ " V_infty[10] = Density;") = none := by decide

theorem unsupported_stride_rejected : parsePrimitiveExtra "nPrimVar = nDim + 8;" = none := by decide

noncomputable def executeSourceBoundary (rows : Nat) (g : IdealGas) (free : Primitive 2)
    (far : List (FarSite N K vertices)) (walls : List (WallVisit N)) (m : Mem) : Option Mem :=
  executeBoundary rows sourceFarCode sourceWallCode g free far walls m

/-- Source-selected fragments are connected to the memory theorem. Characteristic/Roe
translation, the supplied boundary visits and C++ pointer semantics are still manual interfaces. -/
theorem source_boundary_to_complete_update (owned : N ≤ M) (bounds : Bounds M M K vertices)
    (g : IdealGas) (free : Primitive 2) (P : Primitives M)
    (far : List (FarSite M K vertices)) (walls : List (WallVisit M))
    (s0 : BoundaryState M) (m0 : Mem) (vol dt : Fin M → ℝ)
    (h0 : BoundaryRep vertices m0 s0 P) (hg0 : GeometryRep m0 vol (fun _ => 0) dt)
    (farReady : FarReady g free (far.map (farVisit P))) :
    let sb := boundarySpec g free (far.map (farVisit P)) walls s0
    let so := ownedState owned sb
    let vo := fun p => vol (p.castLE owned)
    let dto := fun p => dt (p.castLE owned)
    ∃ mb mu, executeSourceBoundary M g free far walls m0 = some mb ∧
      BoundaryRep vertices mb sb P ∧ InputsReady mb so vo (fun _ => 0) dto ∧
      executeLoop (N := N) M SU2Boundary.Generated.volume_original SU2Boundary.Generated.periodic_original
        SU2Boundary.Generated.timestep_original SU2Boundary.Generated.points_original
        SU2Boundary.Generated.components_original mb = some mu ∧
      LoopContract so vo dto mb mu ∧
      Frame (boundaryFootprint far walls ++ (visits N).map destination) m0 mu := by
  simpa only [executeSourceBoundary, source_wall_connection, source_far_connection] using
    boundary_to_complete_update owned bounds g free P far walls s0 m0 vol dt h0 hg0 farReady

end SU2BoundaryMemory
