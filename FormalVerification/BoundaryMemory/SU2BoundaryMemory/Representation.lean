import SU2BoundaryMemory.Storage

noncomputable section
namespace SU2BoundaryMemory
open SU2 SU2Source SU2Boundary
variable {N M K : Nat} {vertices : Fin K → Nat}

inductive Buffer where | old | residual | truncation
  deriving DecidableEq, Repr

def Buffer.block : Buffer → Nat
  | .old => 1 | .residual => 5 | .truncation => 6

def Buffer.get (b : Buffer) (s : BoundaryState N) : Residual N 2 :=
  match b with | .old => s.old | .residual => s.residual | .truncation => s.truncation

def Buffer.set (b : Buffer) (s : BoundaryState N) (u : Residual N 2) : BoundaryState N :=
  match b with
  | .old => {s with old := u}
  | .residual => {s with residual := u}
  | .truncation => {s with truncation := u}

theorem buffer_block_bound (b : Buffer) : b.block < 8 := by cases b <;> decide

theorem buffer_block_injective {a b : Buffer} (h : a.block = b.block) : a = b := by
  cases a <;> cases b <;> simp_all [Buffer.block]

def BuffersRep (m : Mem) (s : BoundaryState N) : Prop :=
  ∀ (b : Buffer) (p : Fin N) (c : Fin 4), m (flowAddr b.block p c) = some (b.get s p (channel c))

abbrev Primitives (N : Nat) := Fin N → Fin 11 → ℝ

def PrimitiveRep (m : Mem) (P : Primitives N) : Prop :=
  ∀ p c, m (primitiveAddr p c) = some (P p c)

def inside (P : Primitives N) (p : Fin N) : Primitive 2 :=
  ⟨P p 4, ![P p 1, P p 2], P p 3⟩

def ghostField (g : Ghost) : Fin 6 → ℝ :=
  ![g.temperature, g.primitive.velocity 0, g.primitive.velocity 1,
    g.primitive.pressure, g.primitive.density, g.enthalpy]

/-- None records no known logical ghost: it does not assert physical cells are absent
or initialized. Known ghosts constrain their six fields; all other cells are framed. -/
def CacheRep (vertices : Fin K → Nat) (m : Mem) (cache : Nat → Option Ghost) : Prop :=
  ∀ (s : CacheSite K vertices) (g : Ghost), cache (cacheKey s.marker s.vertex) = some g →
    ∀ c : Fin 6, m (cacheAddr s.marker s.vertex c) = some (ghostField g c)

/-- Current may initially be arbitrary, including uninitialized. Boundary execution
frames its actual memory; the subsequent update establishes all owned current cells. -/
structure BoundaryRep (vertices : Fin K → Nat) (m : Mem) (s : BoundaryState N) (P : Primitives N) : Prop where
  buffers : BuffersRep m s
  primitives : PrimitiveRep m P
  cache : CacheRep vertices m s.cache

def GeometryRep (m : Mem) (vol per dt : Fin N → ℝ) : Prop :=
  (∀ p, m (2,p.val) = some (vol p)) ∧
  (∀ p, m (3,p.val) = some (per p)) ∧
  (∀ p, m (4,p.val) = some (dt p))

theorem readiness_of_rep (m : Mem) (s : BoundaryState N) (vol per dt : Fin N → ℝ)
    (h : BuffersRep m s) (hg : GeometryRep m vol per dt) : InputsReady m s vol per dt := by
  exact ⟨fun c => h .old c.1 c.2, fun c => h .residual c.1 c.2,
    fun c => h .truncation c.1 c.2, hg⟩

def farVisit (P : Primitives N) (v : FarSite N K vertices) : FarVisit N :=
  ⟨v.point, cacheKey v.cache.marker v.cache.vertex, v.owned, v.normal, v.area, v.area_pos,
    inside P v.point, P v.point 5⟩

/-- Characteristic reconstruction reads velocity, pressure and density first;
stored enthalpy is read later by the scalar Roe computation. -/
def readInterior (rows : Nat) (m : Mem) (p : Fin N) : Option (Primitive 2) := do
  if ¬ validRow rows 11 p.val then none else
  let vx ← m (primitiveAddr p 1)
  let vy ← m (primitiveAddr p 2)
  let pressure ← m (primitiveAddr p 3)
  let density ← m (primitiveAddr p 4)
  return ⟨density, ![vx,vy], pressure⟩

/-- The production primitive getters have an eleven-column row stride. -/
def readPrimitive (rows : Nat) (m : Mem) (p : Fin N) : Option (Primitive 2 × ℝ) := do
  if ¬ validRow rows 11 p.val then none else
  let vx ← m (primitiveAddr p 1)
  let vy ← m (primitiveAddr p 2)
  let pressure ← m (primitiveAddr p 3)
  let density ← m (primitiveAddr p 4)
  let enthalpy ← m (primitiveAddr p 5)
  return (⟨density, ![vx,vy], pressure⟩, enthalpy)

theorem valid_point_primitive (bounds : Bounds N M K vertices) (p : Fin N) : validRow M 11 p.val :=
  ⟨lt_of_lt_of_le p.isLt bounds.owned, bounds.points⟩

theorem valid_point_flow (bounds : Bounds N M K vertices) (p : Fin N) : validRow M 4 p.val := by
  refine ⟨lt_of_lt_of_le p.isLt bounds.owned, ?_⟩
  have h := bounds.points
  omega

theorem valid_cache (bounds : Bounds N M K vertices) (s : CacheSite K vertices) :
    validRow (vertices s.marker) 11 s.vertex.val := ⟨s.vertex.isLt, bounds.caches s.marker⟩

theorem primitive_bounds (bounds : Bounds N M K vertices) (p : Fin N) (c : Fin 11) :
    p.val * 11 + c.val < M * 11 ∧ 8 * (p.val * 11 + c.val) + 7 < 2^64 := by
  have hp := lt_of_lt_of_le p.isLt bounds.owned
  exact ⟨row_major_bound M 11 _ _ hp c.isLt, byte_offset_bound M 11 _ _ hp c.isLt bounds.points⟩

theorem cache_bounds (bounds : Bounds N M K vertices) (s : CacheSite K vertices) (c : Fin 11) :
    s.vertex.val * 11 + c.val < vertices s.marker * 11 ∧
    8 * (s.vertex.val * 11 + c.val) + 7 < 2^64 ∧ s.marker.val < 2^16 := by
  exact ⟨row_major_bound _ 11 _ _ s.vertex.isLt c.isLt,
    byte_offset_bound _ 11 _ _ s.vertex.isLt c.isLt (bounds.caches s.marker),
    lt_trans s.marker.isLt bounds.markers⟩

theorem read_primitive_correct (m : Mem) (P : Primitives N) (p : Fin N)
    (bounds : Bounds N M K vertices) (h : PrimitiveRep m P) :
    readPrimitive M m p = some (inside P p, P p 5) := by
  unfold PrimitiveRep at h
  simp [readPrimitive, valid_point_primitive bounds p, h, inside]

theorem read_interior_correct (m : Mem) (P : Primitives N) (p : Fin N)
    (bounds : Bounds N M K vertices) (h : PrimitiveRep m P) :
    readInterior M m p = some (inside P p) := by
  unfold PrimitiveRep at h
  simp [readInterior, valid_point_primitive bounds p, h, inside]

end SU2BoundaryMemory
