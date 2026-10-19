# Review: inverse support/parameter recovery for the finite-facet authority

## Verdict

Recovery from actual coordinates can be made **sound**: anything it accepts, it certifies. It cannot be **complete** or **restart-transparent**. The ideal parameters carry information that no binary64 actual coordinate encodes. A deterministic inverse will therefore reject some states your forward contract proves valid, and a run with a restart can diverge from one without.

The two halves of the binding behave differently:

- **The support class is recoverable exactly**, but only from combinatorics. Your geometric priority rule is a heuristic and is wrong.
- **The continuous parameter (t or u,v) is not recoverable.** You need to persist it, or redefine the authority so it never has sub-coordinate resolution.

## 1. Support class: recoverable, by proof, without geometry

You already persist a facet ID per output boundary face. For a boundary node N, let S(N) be the set of facet IDs on its incident faces. Define

> support(N) = the meet (smallest common closed cell) of S(N) in the original complex.

The result is a facet if |S| = 1, the shared edge if S spans exactly two facets sharing that edge, and otherwise the common original vertex. If the meet is empty, reject.

**Why this is exact.** Assume the surface is closed and every face's ideal triangle lies in its facet. A node whose ideal point is interior to edge e has an edge-neighbourhood covered on every facet incident to e, so its faces include all of them. A node interior to facet F has faces only in F. The closure audit independently rejects any state where these fail. So classification needs no thresholds and no exact vertex-coordinate matching. It also stays correct for near-coincident unrelated planes, thin fins, and slits, because face IDs disambiguate them.

**Your geometric rule fails.** It prefers an edge whose mapped foot is inside [0,1] and within the contract distance. Counterexample:

- F1 is z=0 with y≥0. F2 is y=0 with z≥0. The shared edge e = AB runs along the x-axis. Take ε = 2⁻⁴⁰.
- Ideal node q is interior to F1 at (0.5, 2⁻⁴⁵, 0), and all its faces are in F1.
- Actual x = (0.5, −2⁻⁴⁴, 0). The displacement is 3·2⁻⁴⁵ ≤ ε, so the contract is satisfied.
- The rule classifies q as an e-node. F2's edge chain then contains a node that no F2 face uses, so closure fails.

If you instead reject ambiguity anywhere within ε of an edge, no interior insertion is possible in an ε-band around every feature. That breaks finite-footprint coverage exactly where an anisotropic target asks for h < ε.

**Junction rule (needed either way).** A node's displacement bound is the minimum of ε_F over S(N), not the bound of whichever facet you happened to query.

## 2. Parameter: not recoverable — concrete counterexamples

### A. Sub-lattice sliver (proof by pigeonhole)

Take facet A=(1024,0,0), B=(1025,0,0), C=(1024,1,0), parameterised as x = 1024+u, y = v, z = 0. The plane z=0 is chosen so that projection is exact and only the tangential information question remains.

- Ideal p₁=(0.25, 0.25), p₂=(0.25, 0.5), p₃=(0.25−2⁻⁵⁰, 0.375). The exact orientation is nonzero, with the sign the mesh requires.
- The mapped x-coordinate of p₃ is 1024.25−2⁻⁵⁰. The ulp at 1024 is 2⁻⁴², so every representable realisation is 1024.25 or a neighbour of it. The parameter lattice is about 2⁸ times finer than the coordinate lattice.
- Realise x₃ = (1024.25, 0.375, −2⁻⁴⁰). The displacement is about 2⁻⁴⁰ ≤ ε. The normal offset makes the 3D face non-degenerate, and actual tetrahedron positivity can hold with the adjacent apex.
- **Forward contract:** the ideal triangle is positively oriented, the vertex displacement is within ε, and the actual tet is positive. The state is accepted.
- **Recovery:** the foot gives p₃' = (0.25, 0.375) exactly, so the recovered orientation is exactly 0. Choosing the realisation 1024.25+2⁻⁴² instead makes it inverted. Restart rejects.

Two parameter states, one valid and one degenerate or inverted, share identical actual coordinates and connectivity. **No** deterministic function of (coordinates, IDs) can accept the first without also deciding the second, because it receives the same input. This is a proof, not a tuning artefact. It applies to every facet far enough from the origin, i.e. essentially all real geometry.

This is not an exotic configuration. Your contract deliberately admits ε-scale tangential folds, because the actual surface only needs to be within ε of an ideal surface that tiles the facet. Recovery silently replaces that contract with a stricter one: the actual surface must project without folds at ulp scale.

### B. Near-edge interior node: foot leaves the facet

Reuse the setting of §1. Now classify q correctly as F1 using face IDs. Its F1-plane foot is (0.5, −2⁻⁴⁴), which lies outside F1. Your remaining choices are to clamp, which is closest-donor clipping and excluded, or to reject a valid state. Stored (u,v) has no such problem.

### C. Edge chain order (the 1D version of A)

Take t₁ = 0.5 and t₂ = 0.5+2⁻⁵⁰ on an edge near x = 1024. Directed realisations that are both within ε, plus normal offsets keeping the tets positive, can swap the recovered order. Canonical endpoint ordering makes F1 and F2 *agree* on the swapped t's. The chain is then consistent across facets but contradicts the face connectivity. Restart rejects a valid state.

### D. Restart transparency and drift

Suppose a proposal computes a child at the ideal midpoint of its parents' parameters. With stored parameters it uses mid(p₁, p₂). After a restart it uses mid(R(x₁), R(x₂)). These children differ, so the remeshing trajectory depends on whether a restart happened.

If any stage re-realises actual coordinates from recovered parameters, the cycle R → realise → R is not proven idempotent. Tangential positions can then random-walk by ulps per cycle, and nothing bounds the accumulation.

With stored parameters, realisation happens exactly once, so drift is impossible by construction. That is a proof. The claim that recovery-based drift actually occurs in practice is heuristic; the proven part is that the absence of drift is not guaranteed.

## 3. Items that hold up, with conditions

**Exact zeros after scalar arithmetic.** An edge node's facet weights must be the exact *embedding* (1−t, t, 0) in the facet's local vertex order. Never recompute them from a facet foot, which would give w ≠ 0 by rounding and make F1 and F2 disagree.

Since 1−t and 1−u−v are generally not binary64, evaluate the orientation as the exact 3×3 determinant of real weights using expansion or rational arithmetic, not a 2D determinant on rounded values. Zero weights then stay exactly zero.

**Finite-footprint coverage (proof).** The faces of facet F tile F if all of the following hold:

1. every F-face has positive exact parameter orientation;
2. every interior F-edge has exactly two F-faces;
3. the F-boundary edges are exactly the canonical-t subdivisions of F's three original edges, consistently oriented.

The parameter map is then a degree-1 covering, which implies no overlaps or gaps. This holds whether the parameters are stored or recovered. Recovery just fails condition 1 or 3 more often (§2).

**Interface/volume closure (proof).** Use the vertexwise straight-line homotopy from the ideal to the actual surface. It stays within the band, provided the band is ⋃_F (ε_F-neighbourhood of F) and node bounds use the junction minimum from §1. So winding numbers outside the band are preserved and the actual volume agrees with the original body off the band. Your certified exterior shell covers the band, and the original volume wins.

This argument needs *some* ideal↔actual vertex correspondence. That correspondence is exactly what stored parameters provide and recovery only sometimes reproduces.

**No bridging (proof).** Each face's ideal triangle lies in one convex facet, so no ideal face crosses a hole. Actual-surface intersections across gaps narrower than 2ε are the global embedding audit's job. That audit is necessary, not optional.

**Determinism of R, if you still use it anywhere.** R must be specified bit-exactly: exact rational foot, then correct rounding. Ordinary floating-point evaluation differs across FMA availability and compilers, which would make "deterministic recovery" platform-dependent.

## 4. Is "verify everything at proposal and restart" sufficient?

- **Soundness: yes.** If every proposal and every restart verify the recovered state (unique support, all incident memberships, whole-face support and orientation, global embedding), then anything accepted is certified.
- **Completeness and reproducibility: no.** Valid forward-certified states are rejected at restart (§2 A–C). Making proposals also certify only R(x) fixes reproducibility. But the authority then becomes "projection of actual", which:
  - forbids ideal geometry finer than the coordinate lattice;
  - forbids tangential ε-folds the contract otherwise allows;
  - blocks interior nodes whose foot exits the facet near edges.

  These restrictions bite exactly at features where h < ε, which is the regime this authority exists for.

## 5. Least extra state

**Recommendation.** Persist the support class and the parameters per boundary node in the immutable sidecar: an original vertex ID, or (edge ID, binary64 t with canonical endpoints), or (facet ID, binary64 u, v). That costs at most one ID plus 16 bytes per boundary node and never touches the volume.

Then at proposal, import, and restart:

1. Cross-check the stored class against the facet-ID meet (§1, exact).
2. Verify the exact parameter orientation and the coverage conditions (§3).
3. Recompute the outward-rounded mapped bounds and certify displacement ≤ min ε_F.
4. Run actual tetrahedron positivity and the global embedding audit.

All checks are local except the embedding audit. No negotiation or gather is needed, and restart is bit-identical.

**Not recommended:**

- *Classification only.* Store nothing and derive the class from the meet. This saves only the ID, because the parameters are still needed.
- *Sparse override list.* Store parameters only where R(realise(p)) ≠ p. On non-axis-aligned facets that is nearly every node, since a binary64 point's foot is almost never the stored binary64 parameter. This is a heuristic expectation, but it means the "sparse" list is effectively complete while adding a bit-exact R as a dependency.
- *Zero-state, recovered authority.* This is sound and deterministic, but provably incomplete by §2A. Accept it only if you are willing to forbid sub-lattice slivers and near-edge interior insertion.

## Proof vs heuristic summary

| Claim | Status |
|---|---|
| Meet classification | Proof (given a closed surface and the closure audit) |
| Information loss in §2A | Proof (pigeonhole on lattices) |
| Coverage, closure, no-bridging | Proofs |
| Drift actually occurring under recovery | Heuristic (proven only that it is not excluded) |
| Override list being near-total | Heuristic |
| Everything is sound under re-verification | Proof |
| Recovery-based design is complete | False (counterexamples §2 A–C) |