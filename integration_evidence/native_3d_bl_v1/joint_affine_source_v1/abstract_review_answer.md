# Review: collar embedding, r-neighborhood, and SPD transfer

## 1. Do the collar interiors avoid the original volume?

**Verdict: yes, this is a proof.** It needs no exhaustive check against original tetrahedra. Check (3) is in fact redundant once (2) holds for all collar pairs.

### Setup

- Let C be the collar complex and Omega its polyhedron.
- Let U = int Omega.
- Let B_i be the components of B, and let C_i = B_i × [0,1] be their collars.
- Let V_i be the image of B_i × (0,1), and let V be the union of the V_i.
- Let B′ be the outer skin, the image of B × {1}.

### Argument

**(a) The collar is an embedded B × [0,1].** The consistent prism subdivision makes C combinatorially a triangulation of B × [0,1]. Check (2), over all pairs, makes the realization a geometric simplicial complex. A simplicial complex is homeomorphic to its polyhedron, so C is a topological embedding of B × [0,1].
- By invariance of domain, V is open in R³.
- By injectivity, V ∩ B = ∅ and B′ ∩ B = ∅.
- Each V_i is connected, and distinct V_i are disjoint.

**(b) V ∩ U is clopen in V.** It is open because V and U are open. For closedness, take x ∈ V in the closure of V ∩ U.
- Then x lies in the closure of U, which is Omega.
- Also x ∉ B, by (a).
- So x ∈ Omega \ B = U. This step uses that the frontier of Omega is exactly B.
- Hence x ∈ V ∩ U.

**(c) Each V_i is disjoint from U.** Since V_i is connected, (b) leaves two options: V_i ⊂ U or V_i ∩ U = ∅. Pick one collar tetrahedron whose base is a triangle t of B_i, and a point p in the relative interior of t.
- Omega lies only on the inward side of t near p, because t has exactly one incident original tetrahedron.
- By (1), points of that collar tetrahedron's interior near p lie on the outward side, hence outside Omega.
- So V_i is not contained in U, and therefore V_i ∩ U = ∅.

**(d) The collar meets Omega only in B.** From (a)–(c), V ∩ Omega = ∅. Also B′ ∩ U = ∅, because B′ lies in the closure of V and U is open. So C ∩ Omega = B.

**(e) The augmented complex is conforming.** Take a collar tetrahedron σ and an original tetrahedron τ.
- By (d), σ ∩ τ ⊂ B.
- By (2), σ ∩ B is the single face s of σ spanned by its B-vertices, and s is a simplex of B.
- Hence σ ∩ τ = s ∩ τ.
- Because s and τ both belong to the conforming complex Omega, s ∩ τ is a common face (or empty).

So Omega ∪ C is an embedded conforming augmented complex.

### Entering at a shared boundary vertex

This is excluded, and not by a local picture. A collar interior point inside U would make V ∩ U nonempty, contradicting (c).

The local picture is also consistent. At a boundary vertex v, the link of v in Omega is the closure of one component of S² minus γ, where γ is the link of v in B. By (2), the link of v in C is an embedded disk with boundary γ. By Schoenflies, that disk is the closure of one of the two components, and (1) at the faces around v selects the other one. The global connectivity argument does not need this.

### Assumptions that are essential

1. **(2) must hold for every pair of collar tetrahedra**, not only adjacent ones.
   - Counterexample if only adjacent pairs are checked, together with (3): take a solid plate thinner than the collar between two fluid regions. The collar from one side passes through the plate, through B on the far side, and into far-side tetrahedra that touch B only at edges or vertices, or not at all.
   - The full check (2) catches this because the two sides' collars overlap.
2. **The frontier of Omega must be exactly B, and Omega must be pure**: no dangling lower-dimensional pieces, no zero-thickness internal baffles.
   - If some boundary faces are excluded from B (for example, patches not treated as "physical"), step (b) fails and a collar can enter through those faces.
3. **The collar's bottom must be literally B**, with the same vertex identities, not duplicated coordinates.
4. **Exactness:** the outward orientation of B must be derived from Omega's own tetrahedra, and the predicates must be evaluated exactly on the stored binary64 coordinates. These are your stated conditions; no volume-sign tolerance enters.

Connectedness of each V_i holds automatically. Genus, multiple bodies and disconnected B are all harmless, because the argument runs per component.

## 2. Does the augmented domain contain the external open r-neighborhood of B?

**Verdict: yes.** Here A = Omega ∪ C, embedded, with frontier of A equal to B′, and dist(B, B′) ≥ r > 0, where the minimum over triangle pairs is exactly the set distance.

### Path argument

Take x with d(x, B) < r. Choose p ∈ B with |x − p| < r, and consider the segment [p, x].

- **The path avoids B′.** Every y on the segment satisfies d(y, B) ≤ |y − p| < r. So the segment never meets B′.
- **The path starts inside A.** p lies in A and not in B′, which is the frontier of A. So p ∈ int A.
- **The path stays inside A.** The set of segment points in int A is open in the segment. It is also closed: a limit point lies in A because A is closed, and it is not in the frontier because the segment avoids B′.
- By connectedness, all of [p, x] lies in int A.

So the whole open r-neighborhood of B lies in int A. Its external part (outside Omega) lies in C.

### Rigour caveats

- The "proven ≥ r" must be a rigorous lower bound on the exact distance between the binary64 coordinates.
- "Frontier of A = B′" must be established, for example from (1)–(2) plus pure embedding. Given those, frontier faces are exactly the faces with one incident tetrahedron.

## 3. SPD tensor transfer

**Verdict: correct as exact mathematics, under the preconditions below.**

### Preconditions

1. **The original field must be the nodal P1 interpolant on Omega**, with one tensor per vertex. Per-element corner values, P0 fields or higher-order fields break the continuity argument.
2. **The augmented complex must be conforming with shared vertex identities**, as established in §1.
3. **Each outer vertex v′ receives exactly M_v**, the tensor of its own base vertex v.
4. **Each M_v must be exactly symmetric and positive definite.**

### Continuity

On each B triangle, the original side and the collar side both restrict to the same linear interpolant of the same three nodal values. Inside the collar, conformity gives continuity across shared faces.

The result is C⁰ only. On each prism, the field equals the base-triangle interpolant composed with the simplexwise-affine combinatorial projection a′ ↦ a, b′ ↦ b, c′ ↦ c. This is not the Euclidean nearest-point projection.

### SPD preservation

Inside a nondegenerate tetrahedron (guaranteed by (1)), the interpolated value is Σ λ_i M_i with λ_i ≥ 0 and Σ λ_i = 1.

- **Symmetry** is preserved by componentwise interpolation.
- **Positive definiteness:** xᵀ(Σ λ_i M_i)x = Σ λ_i xᵀ M_i x > 0 for x ≠ 0, since at least one λ_i is positive.
- **Eigenvalue bounds:** λ_min of the combination is at least min_i λ_min(M_i), and λ_max is at most max_i λ_max(M_i).

### Exact proof versus floating-point behaviour

The statement above is about exact arithmetic. In binary64:

- **Symmetry:** store 6 components, or compute (i,j) and (j,i) by the identical expression; otherwise rounding can break symmetry.
- **Definiteness:** a rounded result can lose positive definiteness when λ_min/‖M‖ is near machine epsilon. Floating-point SPD needs a separate error bound, roughly λ_min > c·ε·max‖M_i‖.
- **Barycentric coordinates:** computed coordinates can come out slightly negative for points on or near faces. Positivity of the weights must be certified, for example by clamping and renormalizing, or by exact evaluation.
