# Optional abstract second opinion — Opus5.5/medium

User-authorized local Claude Code consultation,2026-10-19. Only generic numerical
geometry/source-extension questions were supplied: no repository files, code or
saved testcase data. All tools/MCP/skills were disabled, no weaker fallback and no
session persistence. Both calls returned claude-opus-5-5, at requested medium effort.
Model review is design input, not correctness evidence.

## First answer — superseded recommendation

# Short answer

Keep **exact internal coordinates** as the real state. This is the only option that removes the obstruction. The other two options only move it.

# The three options

**1. Local charts with audited mapping error.** A chart is a small flat map of a surface patch.
- The chart points are still binary64 numbers. The mapped 3D points are also rounded.
- The obstruction moves into the "audited error".
- Two charts that meet at a junction need a third rule to agree.
- Any error above zero changes the body geometry, the first wall height or the metric acceptance. You said these must not change.
- **Verdict:** this moves the problem.

**2. Once-only perturbed faceted reference.** You shift the planes one time, so that the points become representable.
- This changes the immutable body.
- It breaks the membership of the symmetry plane.
- A shift that fixes one feature can make a new near-degenerate feature in a different location.
- **Verdict:** this moves the problem.

**3. Exact or rational internal coordinates.** Each binary64 plane coefficient is an exact rational number.
- An intersection of three planes is rational, with bounded bit length.
- A convex combination of rational points is rational. Thus, interior points always exist.
- The length target becomes possible again.
- **Verdict:** this removes the problem in the geometry layer.

# Minimal architecture

**A. Geometry kernel (immutable, versioned once at adoption)**
- Entities are a B-rep. B-rep means "boundary representation": bodies, faces, edges and corners with topology.
- Each face points to one carrier. A carrier is an exact plane or an analytic surface.
- Each finite line or plane has explicit bounds and loops. Thus, holes and disconnected regions are only topology.
- The symmetry plane is its own carrier. Do not merge it with near-coplanar body facets. Its junction with a body face is a new edge, and it has one owner.
- Adopt the geometry before you create states. Freeze the sensor-only P1 donors after that.

**B. Vertex = tag + exact position**
- Tag = the lowest-dimension entity that the vertex is on (corner, edge, face or volume).
- On a plane, the position is rational. On an analytic surface, store (surface id, parameter) and a lazy exact number. A lazy exact number is an interval that gets more precise only when needed.

**C. Predicates**
- Use a filtered orientation test. First try a fast double test with an error bound. If that is not certain, use exact rational arithmetic.
- Accept a simplex only if its orientation is strictly positive in exact arithmetic.

**D. Junction ownership**
- Each edge and corner has one global id and one owner rank.
- All ranks get the same exact coordinate from the same tag. Thus, ghost copies are equal bit for bit. Ghost copies are local copies of data from other ranks.
- A patch transaction locks the closure of the patch. The closure includes all junction entities that the patch touches.
- Commit only if all exact predicates are positive and all tags stay valid. If not, roll back.

**E. Sensor source extension**
- Inside the original volume, evaluate the original P1 exactly with exact barycentric coordinates.
- Outside the original rounded boundary, use the **tag**, not a nearest-point search.
  - Project the point exactly onto its tagged original facet, edge or corner.
  - Evaluate the P1 trace at that projection.
- The P1 trace is continuous. Thus, the values agree at shared edges and corners.
- The coverage band must not be wider than half of the local feature size. If a query is outside that band, report it as a contract failure. Do not fix it silently.

**F. Binary64 export**
- The solver view is a derived output. It has an explicit audited deviation.
- Do a second positivity check in double precision.
- If that check fails, report it as a declared contract violation. Do not merge points.

# What simultaneous junction ownership and source extension need

1. One owner and one exact coordinate rule for each junction.
2. The same tag must control the mesh constraint and the sensor projection. If the two use different rules, the traces will not agree at edges.

# Hardest counterexamples

1. **Nonconvex (reflex) corner.** Outside the corner, a nearest-point search can jump between two faces that are not adjacent. The tag-based projection prevents this jump. But a vertex that slides across the corner must change its tag in the transaction.
2. **Close separate bodies.** A thin gap is narrower than the coverage band. A point in the gap can be near two bodies with different traces. The tag prevents mixing the two bodies. If the gap is narrower than the band, report it. Do not merge the bodies.
3. **Symmetry plane next to a body facet that is almost coplanar.** These are two carriers with an exact intersection line. The angle can be very small. Exact predicates still give the correct sign, but the bit length becomes large.
4. **Tangent analytic surfaces.** The lazy filter can refine forever. You must declare a separation bound, or report the case.
5. **Ownership race across ranks.** One edge can be in two patches on two ranks. The lock on the closure must serialize the two transactions.
6. **Bit-length growth.** Many local edits can make the rational numbers very large. Round to a new exact point only inside the tagged entity, and only if the predicates stay positive.

# What to do now

Use Option 3 with parts A to F.

## Clarification supplied

Both designs must use the same explicit bounded geometry mapping contracts; the
actual solver mesh and final positivity/metric checks use binary64. Exact internal
coordinates still require export auditing. Also, tagged projection alone cannot
establish continuity: nearby projections to adjacent facets can sample different
P1 traces, and private/remote queries need one globally frozen field.

## Follow-up — narrowed recommendation

**Short answer:** Only a small part of the claim survives. You do not need exact coordinates everywhere. You need exact **decisions** in a few places.

## 1. What survives

- **Exact predicates survive.** A predicate is a yes/no geometry test. Examples are "which side of a plane", "is this point inside this tet", and "do these two entities touch". These tests must give one consistent answer. Use exact or certified-filtered predicates. A filtered predicate uses fast floating point and switches to exact arithmetic only when the result is near zero.
- **"Exact coordinates are the only way" does not survive.** All authorities share one deviation bound, δ, and the final checks run on binary64. Under that contract, rational internal coordinates give no extra guarantee. They are rounded at export, and the rounding can still invert an element. So the final binary64 checks decide in both designs.

## 2. Tags + parametric positions vs exact internal coordinates

Both designs use the same δ and the same final binary64 checks.

| Item | Tags + parameters | Exact internal coordinates |
|---|---|---|
| Shared vertex, edge, or face | Evaluate it once, from its lowest-dimension owner. Store one binary64 value. Reuse that value. | One exact value, rounded once at export. |
| Why shared points agree | Same stored value, not same arithmetic. | Same exact value. |
| Exact coplanarity after rounding | Not given. | Not given. |
| Main risk | Two stages re-evaluate the same parameter differently. | Export rounding inverts an element. |

**Rule:** Never re-project a shared entity. Compute it once and copy the bits.

**Verdict:** The two designs are equal under this contract. Exact arithmetic is useful inside predicates, not as a storage format.

## 3. Tagged projection does not give a continuous extension

Take two facets that share an edge.

- A point near the edge projects to a different foot point on each facet. A foot point is the closest point on that facet.
- The two source traces agree **on** the edge. Away from the edge, they differ.
- A nearest-facet rule switches tags at the medial surface. The medial surface is the set of points with the same distance to both facets. The value jumps there.
- The original P1 field has priority inside its own volume, Ω0. This adds a second seam at the boundary of Ω0.

So tagging alone does not prove continuity. It also does not prove coverage.

## 4. Small complete design: a frozen collar mesh

1. Build one tetrahedral collar mesh, M_c, outside Ω0. It must share the exact boundary nodes and faces of the P1 sensor mesh. "Conforming" means this exact sharing.
2. At boundary nodes, set the value to the P1 value.
3. At outer nodes, set the value with one frozen rule. An example is the linear extension of the owning boundary element. Freeze this choice once.
4. Define the extension, E:
   - E = P1 inside Ω0.
   - E = P1 interpolation on M_c.
   - Outside Ω0 ∪ M_c, E is undefined. A query there is a hard fail.
5. Use exact orientation predicates to locate query points. A query point on a shared face gets the same value from both sides, because the mesh conforms. The tie rule does not change the value, apart from rounding during evaluation.
6. Freeze M_c and its nodal values before all stages. Private and remote queries must use only this one object. No candidate may choose a tag.

**Result:** E is continuous by construction and is defined globally on Ω0 ∪ M_c. It does not depend on the candidate.

## 5. What remains unproved

- **Coverage.** Every query point must lie in Ω0 ∪ M_c. This holds only if the collar width is at least δ plus the mapping error. You must check this with a certificate. It is not automatic.
- **Nonconvex junction collars.** At re-entrant edges and corners, offset surfaces fold or self-intersect. A conforming collar exists if you add Steiner points (extra mesh nodes). Valid binary64 placement with good quality is not proved in general. Check it with exact orientation tests.
- **Narrow gaps.** If a gap is narrower than two collar widths, the two collars overlap. Mesh the gap as **one** region, with both traces as boundary data. Then E stays single-valued. Two separate collars are not allowed.
- **Gradient bound.** The Lipschitz bound of E depends on collar element quality. A Lipschitz bound limits how fast E can change. This bound is not proved. It needs a per-mesh certificate.

## What to do now

Keep exact predicates. Drop exact coordinates as a requirement. Build and freeze one conforming collar mesh. Then certify three items: coverage width, valid junction and gap meshing, and the gradient bound.

## Implementation judgment and remaining proof obligations

Keep existing robust predicates on actual solver coordinates. The initial
"only exact coordinates" recommendation is withdrawn; no rational-coordinate
rewrite is justified by this review. Reuse the existing affine/analytic reference
and geometry/source machinery, with shared feature identities and finite-domain
operations through the full lifecycle.

A conforming frozen collar is a useful pattern, not a completed algorithm or
coverage certificate. It must reuse original sensor traces, preserve SPD (generic
linear extrapolation is not automatically SPD), admit memory/work and prove
coverage/nonoverlap at corners. Source priority inside the original volume stays
unchanged. Candidate-dependent tags/projection must not select different targets
at the same point. Close physical bodies must not be silently merged or have their
traces blended. Actual gap topology and original-fluid coverage must be examined;
the review's generic "mesh the gap" advice is not a universal rule.

Complete finite feature/face containment must survive mapped coordinates: merely
moving the same exact-collinearity obstruction into a2D chart is insufficient.
Shared bindings/parameters, mapping/error validation, source coverage, MPI ownership
and persistence/restart need one connected implementation. None is established by
these responses. No new construction or performance success is claimed.
