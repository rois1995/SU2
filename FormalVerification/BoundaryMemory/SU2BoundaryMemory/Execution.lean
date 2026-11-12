import SU2BoundaryMemory.Representation

noncomputable section
namespace SU2BoundaryMemory
open SU2 SU2Source SU2Euler SU2Boundary
variable {N K : Nat} {vertices : Fin K → Nat}

/-- Both source reduction terms precede any store. The component is reloaded at each
store, as in the source. Alternate modes are explicit semantic controls. -/
def projectMemory (block : Nat) (p : Fin N) (n : Vec 2) (op : Op)
    (refresh reverse : Bool) (m : Mem) : Option Mem := do
  let x ← m (flowAddr block p 1)
  let y ← m (flowAddr block p 2)
  let d := (0 + x * n 0) + y * n 1
  if reverse then
    let vy ← m (flowAddr block p 2)
    let m1 := put m (flowAddr block p 2) (opReal op vy (d * n 1))
    let vx ← m1 (flowAddr block p 1)
    return put m1 (flowAddr block p 1) (opReal op vx (d * n 0))
  else
    let vx ← m (flowAddr block p 1)
    let m1 := put m (flowAddr block p 1) (opReal op vx (d * n 0))
    let d2 ← if refresh then do
      let x2 ← m1 (flowAddr block p 1)
      let y2 ← m1 (flowAddr block p 2)
      pure ((0 + x2 * n 0) + y2 * n 1)
      else pure d
    let vy ← m1 (flowAddr block p 2)
    return put m1 (flowAddr block p 2) (opReal op vy (d2 * n 1))

structure WallCode where
  oldBlock : Nat
  oldOp : Op
  residualOp : Op
  truncationOp : Op
  refresh : Bool
  reverse : Bool
  deriving DecidableEq, Repr

def wallCode : WallCode :=
  ⟨1, SU2Boundary.Generated.wallOld_original, SU2Boundary.Generated.wallRes_original,
    SU2Boundary.Generated.wallTrunc_original, false, false⟩

def executeWall (rows : Nat) (code : WallCode) (v : WallVisit N) (m : Mem) : Option Mem :=
  if v.owned then
    if validRow rows 4 v.point.val then do
      let m1 ← projectMemory code.oldBlock v.point v.normal.vector code.oldOp code.refresh code.reverse m
      let m2 ← projectMemory 5 v.point v.normal.vector code.residualOp code.refresh code.reverse m1
      projectMemory 6 v.point v.normal.vector code.truncationOp code.refresh code.reverse m2
    else none
  else some m

inductive CacheMode where | normal | shortStride | firstMarker | firstVertex
  deriving DecidableEq, Repr

def selectedCacheAddr (mode : CacheMode) (marker vertex column : Nat) : Address :=
  (9 + (if mode = .firstMarker then 0 else marker),
   (if mode = .firstVertex then 0 else vertex) * (if mode = .shortStride then 6 else 11) + column)

/-- The six scalar source assignments are explicit; columns 6..10 are untouched by
the selected code. Erasing them is a separate, syntactically valid mutant. -/
def writeCache (mode : CacheMode) (eraseTail : Bool) (site : CacheSite K vertices)
    (g : Ghost) (m : Mem) : Mem :=
  let a := selectedCacheAddr mode site.marker site.vertex
  let m1 := put m (a 0) g.temperature
  let m2 := put m1 (a 1) (g.primitive.velocity 0)
  let m3 := put m2 (a 2) (g.primitive.velocity 1)
  let m4 := put m3 (a 3) g.primitive.pressure
  let m5 := put m4 (a 4) g.primitive.density
  let m6 := put m5 (a 5) g.enthalpy
  if eraseTail then
    put (put (put (put (put m6 (a 6) 0) (a 7) 0) (a 8) 0) (a 9) 0) (a 10) 0
  else m6

def readCached (mode : CacheMode) (site : CacheSite K vertices) (m : Mem) : Option (Primitive 2 × ℝ) := do
  let a := selectedCacheAddr mode site.marker site.vertex
  let vx ← m (a 1)
  let vy ← m (a 2)
  let pressure ← m (a 3)
  let density ← m (a 4)
  let enthalpy ← m (a 5)
  return (⟨density, ![vx,vy], pressure⟩, enthalpy)

/-- The selected AddBlock/SetBlock<false> loop, in component order. Each residual
load occurs in the memory left by the preceding component's store. -/
def accumulateResidual (op : Op) (p : Fin N) (f : Fin 4 → ℝ) (m : Mem) : Option Mem := do
  let r0 ← m (flowAddr 5 p 0)
  let m1 := put m (flowAddr 5 p 0) (opReal op r0 (f 0))
  let r1 ← m1 (flowAddr 5 p 1)
  let m2 := put m1 (flowAddr 5 p 1) (opReal op r1 (f 1))
  let r2 ← m2 (flowAddr 5 p 2)
  let m3 := put m2 (flowAddr 5 p 2) (opReal op r2 (f 2))
  let r3 ← m3 (flowAddr 5 p 3)
  return put m3 (flowAddr 5 p 3) (opReal op r3 (f 3))

structure FarCode where
  cacheMode : CacheMode
  eraseTail : Bool
  addition : Op
  deriving DecidableEq, Repr

def farCode : FarCode := ⟨.normal, false, SU2Boundary.Generated.farAdd_original⟩

/-- The source cache-row lookup occurs before the ownership test. Arithmetic uses
only loaded data and the closed, disclosed source model; no expected post-state or
unconstrained flux callback is an execution input. -/
def executeFar (rows : Nat) (code : FarCode) (g : IdealGas) (free : Primitive 2)
    (v : FarSite N K vertices) (m : Mem) : Option Mem := do
  if ¬ validRow (vertices v.cache.marker) 11 v.cache.vertex.val then none else
  if v.owned then
    let L0 ← readInterior rows m v.point
    let W := farPrimitiveImpl g v.normal.vector L0 free
    let mc := writeCache code.cacheMode code.eraseTail v.cache (ghost g W) m
    let (L, HL) ← readPrimitive rows mc v.point
    let (R, HR) ← readCached code.cacheMode v.cache mc
    let f := roeImplementation g v.normal.vector v.area L R HL HR
    accumulateResidual code.addition v.point f mc
  else some m

def executeFars (rows : Nat) (code : FarCode) (g : IdealGas) (free : Primitive 2) :
    List (FarSite N K vertices) → Mem → Option Mem
  | [], m => some m
  | v :: vs, m => (executeFar rows code g free v m).bind (executeFars rows code g free vs)

def executeWalls (rows : Nat) (code : WallCode) : List (WallVisit N) → Mem → Option Mem
  | [], m => some m
  | v :: vs, m => (executeWall rows code v m).bind (executeWalls rows code vs)

/-- Weak far-field pass, then final wall pass, retaining repetitions and evolving memory. -/
def executeBoundary (rows : Nat) (fc : FarCode) (wc : WallCode) (g : IdealGas) (free : Primitive 2)
    (far : List (FarSite N K vertices)) (walls : List (WallVisit N)) (m : Mem) : Option Mem :=
  (executeFars rows fc g free far m).bind (executeWalls rows wc walls)

end SU2BoundaryMemory
