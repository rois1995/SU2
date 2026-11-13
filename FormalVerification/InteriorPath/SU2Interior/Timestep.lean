import SU2Interior.TimestepSpec

set_option maxRecDepth 100000
set_option maxHeartbeats 8000000
noncomputable section
namespace SU2Interior
open SU2 SU2Source SU2Boundary SU2BoundaryMemory SU2Initialization SU2Preprocessing
variable {M E : Nat}

/-- Fresh after the K characteristic-cache blocks; existing addresses stay unchanged. -/
def auxBlock (K slot : Nat) : Nat := 9+K+slot
def auxAddr (K slot : Nat) (p : Fin M) : Address := (auxBlock K slot,p.val)
def lambdaAddr (K : Nat) (p : Fin M) : Address := auxAddr K 0 p
def cflAddr (K : Nat) (p : Fin M) : Address := auxAddr K 1 p

def ownedPoints (N M : Nat) : List (Fin M) := (points M).filter (fun p => p.val < N)

theorem owned_coverage (N : Nat) (p : Fin M) (hp : p.val < N) : p ∈ ownedPoints N M := by
  simp [ownedPoints,points_coverage p,hp]

theorem owned_membership (N : Nat) (p : Fin M) : p ∈ ownedPoints N M ↔ p.val < N := by
  simp [ownedPoints,points_coverage p]

theorem owned_unique (N M : Nat) : (ownedPoints N M).Nodup :=
  (points_no_duplicates (M := M)).filter _

def ScalarRep (b : Nat) (m : Mem) (values : Fin M → Option ℝ) : Prop :=
  ∀ p, m (b,p.val) = values p

theorem scalar_put (b : Nat) (p : Fin M) (x : ℝ) (m : Mem) (values : Fin M → Option ℝ)
    (h : ScalarRep b m values) :
    ScalarRep b (put m (b,p.val) x) (Function.update values p (some x)) := by
  intro q
  by_cases eq : q = p
  · subst q; simp [put]
  · have neq : (b,q.val) ≠ (b,p.val) := by intro e; exact eq (Fin.ext (congrArg Prod.snd e))
    simp only [put,Function.update_of_ne neq,Function.update_of_ne eq]
    exact h q

theorem scalar_primitive_frame (b : Nat) (hb : b ≠ 8) (p : Fin M) (x : ℝ)
    (m : Mem) (P : Primitives M) (h : PrimitiveRep m P) :
    PrimitiveRep (put m (b,p.val) x) P := by
  intro q c
  have neq : primitiveAddr q c ≠ (b,p.val) := fun eq => hb (congrArg Prod.fst eq).symm
  simpa [put,Function.update_of_ne neq] using h q c

theorem aux_not_primitive (K slot : Nat) : auxBlock K slot ≠ 8 := by unfold auxBlock; omega

def readProjected (m : Mem) (p : Fin M) (n : Vec 2) : Option ℝ := do
  let vx ← m (primitiveAddr p 1)
  let vy ← m (primitiveAddr p 2)
  return (0+vx*n 0)+vy*n 1

def readEdgeSpeed (m : Mem) (g : Geometry M E) (p : Fin M) (nb : Fin M × Fin E) : Option ℝ := do
  let vi ← readProjected m p (g.normal nb.2)
  let vj ← readProjected m nb.1 (g.normal nb.2)
  let ai ← m (primitiveAddr p 6)
  let aj ← m (primitiveAddr nb.1 6)
  return |(vi+vj)/2|+((ai+aj)/2)*faceArea (g.normal nb.2)

def readBoundarySpeed (m : Mem) (b : BoundaryFace M) : Option ℝ := do
  let velocity ← readProjected m b.point b.normal
  let sound ← m (primitiveAddr b.point 6)
  return |velocity|+sound*faceArea b.normal

theorem read_projected_correct (m : Mem) (P : Primitives M) (h : PrimitiveRep m P) (p : Fin M) (n : Vec 2) :
    readProjected m p n = some (projected P p n) := by
  unfold PrimitiveRep at h
  simp [readProjected,h,projected]

theorem read_edge_speed_correct (m : Mem) (P : Primitives M) (h : PrimitiveRep m P)
    (g : Geometry M E) (p : Fin M) (nb : Fin M × Fin E) :
    readEdgeSpeed m g p nb = some (edgeSpeed P g p nb) := by
  unfold PrimitiveRep at h
  simp [readEdgeSpeed,read_projected_correct m P h,h,edgeSpeed]

theorem read_boundary_speed_correct (m : Mem) (P : Primitives M) (h : PrimitiveRep m P) (b : BoundaryFace M) :
    readBoundarySpeed m b = some (boundarySpeed P b) := by
  unfold PrimitiveRep at h
  simp [readBoundarySpeed,read_projected_correct m P h,h,boundarySpeed]

def executeNeighbors (K : Nat) (g : Geometry M E) (p : Fin M) : List (Fin M × Fin E) → Mem → Option Mem
  | [], m => some m
  | nb :: rest, m => do
    let speed ← readEdgeSpeed m g p nb
    let prior ← m (lambdaAddr K p)
    executeNeighbors K g p rest (put m (lambdaAddr K p) (prior+speed))

def executeNeighborPoints (K : Nat) (g : Geometry M E) : List (Fin M) → Mem → Option Mem
  | [], m => some m
  | p :: ps, m => do
    let m := put m (lambdaAddr K p) 0
    let m ← executeNeighbors K g p (g.neighbors p) m
    executeNeighborPoints K g ps m

def executeBoundarySpeeds (K : Nat) : List (BoundaryFace M) → Mem → Option Mem
  | [], m => some m
  | b :: bs, m => do
    let m ← if b.owned && b.contributes then do
        let speed ← readBoundarySpeed m b
        let prior ← m (lambdaAddr K b.point)
        pure (put m (lambdaAddr K b.point) (prior+speed))
      else pure m
    executeBoundarySpeeds K bs m

structure TimeStats where
  minimum : ℝ
  maximum : ℝ
  globalPseudo : ℝ
  globalUnsteady : ℝ

def initialTimeStats (prior : TimeStats) : TimeStats := {prior with minimum := 10^30,maximum := 0,globalUnsteady := 10^30}

def executeTimeStores (K : Nat) (cap : ℝ) : List (Fin M) → Mem → TimeStats → Option (Mem × TimeStats)
  | [], m, stats => some (m,{stats with globalPseudo := stats.minimum})
  | p :: ps, m, stats => do
    let vol ← m (2,p.val)
    if vol = 0 then
      executeTimeStores K cap ps (put m (4,p.val) 0) stats
    else
      let cfl ← m (cflAddr K p)
      let lam ← m (lambdaAddr K p)
      let raw := cfl*vol/lam
      let stats := {stats with minimum := min stats.minimum raw,maximum := max stats.maximum raw}
      executeTimeStores K cap ps (put m (4,p.val) (min raw cap)) stats

def executeTimeStep (K N : Nat) (g : Geometry M E) (bs : List (BoundaryFace M)) (cap : ℝ) (m : Mem) (prior : TimeStats) : Option (Mem × TimeStats) := do
  let m ← executeNeighborPoints K g (ownedPoints N M) m
  let m ← executeBoundarySpeeds K bs m
  executeTimeStores K cap (ownedPoints N M) m (initialTimeStats prior)

/-- Pure mathematical fold of raw min/max diagnostics; the cap affects storage only. -/
def statsFold (vol cfl lam : Fin M → ℝ) : List (Fin M) → TimeStats → TimeStats
  | [], stats => {stats with globalPseudo := stats.minimum}
  | p :: ps, stats =>
    if vol p = 0 then statsFold vol cfl lam ps stats else
      let raw := cfl p*vol p/lam p
      statsFold vol cfl lam ps {stats with minimum := min stats.minimum raw,maximum := max stats.maximum raw}

end SU2Interior
