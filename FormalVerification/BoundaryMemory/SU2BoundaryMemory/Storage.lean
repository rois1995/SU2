import SU2Boundary
import Mathlib.Data.Nat.Pairing

noncomputable section
namespace SU2BoundaryMemory
open SU2 SU2Source SU2Boundary

abbrev Mem := Memory ℝ
variable {N K : Nat}

/-- Original four-component buffers and the actual eleven-column primitive/cache rows. -/
def flowAddr (block : Nat) (p : Fin N) (c : Fin 4) : Address := (block, p.val * 4 + c.val)
def primitiveAddr (p : Fin N) (c : Fin 11) : Address := (8, p.val * 11 + c.val)
def cacheAddr (marker vertex column : Nat) : Address := (9 + marker, vertex * 11 + column)
def cacheKey (marker vertex : Nat) : Nat := Nat.pair marker vertex

structure CacheSite (K : Nat) (vertices : Fin K → Nat) where
  marker : Fin K
  vertex : Fin (vertices marker)

structure FarSite (N K : Nat) (vertices : Fin K → Nat) where
  point : Fin N
  cache : CacheSite K vertices
  owned : Bool
  normal : UnitNormal 2
  area : ℝ
  area_pos : 0 < area

/-- Geometry, allocation and timestep computation are caller obligations. -/
structure Bounds (N M K : Nat) (vertices : Fin K → Nat) : Prop where
  owned : N ≤ M
  points : 8 * (M * 11) ≤ 2^64
  markers : K < 2^16
  caches : ∀ k, 8 * (vertices k * 11) ≤ 2^64

def validRow (rows stride row : Nat) : Prop := row < rows ∧ 8 * (rows * stride) ≤ 2^64
instance (rows stride row : Nat) : Decidable (validRow rows stride row) := inferInstanceAs (Decidable (_ ∧ _))

def put (m : Mem) (a : Address) (x : ℝ) : Mem := Function.update m a (some x)

def Frame (addresses : List Address) (before after : Mem) : Prop :=
  ∀ a, a ∉ addresses → after a = before a

theorem frame_refl (as : List Address) (m : Mem) : Frame as m m := by
  intro a _; rfl

theorem frame_trans {xs ys : List Address} {a b c : Mem}
    (h : Frame xs a b) (k : Frame ys b c) : Frame (xs ++ ys) a c := by
  intro p hp
  rw [List.mem_append, not_or] at hp
  exact (k p hp.2).trans (h p hp.1)

theorem put_frame (m : Mem) (a : Address) (x : ℝ) : Frame [a] m (put m a x) := by
  intro b hb
  unfold put
  apply Function.update_of_ne
  simpa only [List.mem_singleton] using hb

theorem cache_key_injective {a b c d : Nat} (h : cacheKey a b = cacheKey c d) : a = c ∧ b = d :=
  Nat.pair_eq_pair.mp h

theorem flow_addr_injective (block : Nat) {p q : Fin N} {i j : Fin 4}
    (h : flowAddr block p i = flowAddr block q j) : p = q ∧ i = j := by
  have h' : offset (p,i) = offset (q,j) := congrArg Prod.snd h
  have eq := offset_injective h'
  exact ⟨congrArg Prod.fst eq, congrArg Prod.snd eq⟩

theorem cache_addr_injective {a b c d : Nat} {i j : Fin 11}
    (h : cacheAddr a b i = cacheAddr c d j) : a = c ∧ b = d ∧ i = j := by
  have hm := congrArg Prod.fst h
  have ho := congrArg Prod.snd h
  have hi := i.isLt
  have hj := j.isLt
  simp only [cacheAddr, Prod.fst, Prod.snd] at hm ho
  exact ⟨by omega, by omega, Fin.ext (by omega)⟩

theorem primitive_cache_separate (p : Fin N) (c : Fin 11) (k v j : Nat) :
    primitiveAddr p c ≠ cacheAddr k v j := by
  intro h
  have := congrArg Prod.fst h
  simp only [primitiveAddr, cacheAddr, Prod.fst] at this
  omega

theorem flow_primitive_separate (b : Nat) (hb : b < 8) (p q : Fin N) (i : Fin 4) (j : Fin 11) :
    flowAddr b p i ≠ primitiveAddr q j := by
  intro h
  have := congrArg Prod.fst h
  simp only [flowAddr, primitiveAddr, Prod.fst] at this
  omega

theorem cache_flow_separate (b : Nat) (hb : b ≤ 8) (p : Fin N) (i : Fin 4) (k v j : Nat) :
    cacheAddr k v j ≠ flowAddr b p i := by
  intro h
  have := congrArg Prod.fst h
  simp only [flowAddr, cacheAddr, Prod.fst] at this
  omega

@[simp] theorem flow_addr_eq (b d : Nat) (p q : Fin N) (i j : Fin 4) :
    flowAddr b p i = flowAddr d q j ↔ b = d ∧ p = q ∧ i = j := by
  constructor
  · intro h
    have hb := congrArg Prod.fst h
    have ho := congrArg Prod.snd h
    have hi := i.isLt
    have hj := j.isLt
    simp only [flowAddr, Prod.fst, Prod.snd] at hb ho
    exact ⟨hb, Fin.ext (by omega), Fin.ext (by omega)⟩
  · rintro ⟨rfl,rfl,rfl⟩; rfl

@[simp] theorem primitive_addr_eq (p q : Fin N) (i j : Fin 11) :
    primitiveAddr p i = primitiveAddr q j ↔ p = q ∧ i = j := by
  constructor
  · intro h
    have ho := congrArg Prod.snd h
    have hi := i.isLt
    have hj := j.isLt
    simp only [primitiveAddr, Prod.snd] at ho
    exact ⟨Fin.ext (by omega), Fin.ext (by omega)⟩
  · rintro ⟨rfl,rfl⟩; rfl

/-- Sequential scalar stores. Used with computed local values, never a required post-state. -/
def writeFields {L : Nat} (addr : Fin L → Address) (value : Fin L → ℝ) :
    List (Fin L) → Mem → Mem
  | [], m => m
  | c :: cs, m => writeFields addr value cs (put m (addr c) (value c))

theorem write_fields_value {L : Nat} (addr : Fin L → Address) (inj : Function.Injective addr)
    (value : Fin L → ℝ) (cs : List (Fin L)) (m : Mem) (c : Fin L) :
    writeFields addr value cs m (addr c) = if c ∈ cs then some (value c) else m (addr c) := by
  induction cs generalizing m with
  | nil => simp [writeFields]
  | cons a cs ih =>
    rw [writeFields, ih]
    by_cases hc : c ∈ cs
    · simp [hc]
    · by_cases ha : c = a
      · subst a; simp [hc, put]
      · have hd : addr c ≠ addr a := fun h => ha (inj h)
        simp [hc, ha, put, Function.update_of_ne hd]

theorem write_fields_frame {L : Nat} (addr : Fin L → Address) (value : Fin L → ℝ)
    (cs : List (Fin L)) (m : Mem) : Frame (cs.map addr) m (writeFields addr value cs m) := by
  induction cs generalizing m with
  | nil => exact frame_refl _ _
  | cons c cs ih =>
    simpa only [writeFields, List.map_cons, List.singleton_append] using
      frame_trans (put_frame m (addr c) (value c)) (ih (put m (addr c) (value c)))

end SU2BoundaryMemory
