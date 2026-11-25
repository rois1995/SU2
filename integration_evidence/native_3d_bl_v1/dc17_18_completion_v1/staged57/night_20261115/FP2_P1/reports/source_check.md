# FP2-P1 R4 source check at 3228ca0e1e

Base is the pushed FP2-M3 final checkpoint. Checked the two public query entries, the memo and bypass paths, curved prepared scope selection and retention, interval helpers and compiler flags, the triangle union, and the outer directed proof.

- The prepared facet leaves supply the selected chord list on a scope miss. Selection, key and completeness use original facet leaf identities and counts. The selected list is charged by capacity before the old witness trace, and the scope restores the caller charge and accounts its final retained size. The closure is inserted between that trace and construction of the joined TriangleUnion. Flat/local/model-to-mesh paths and a cached joined scope do not rebuild that list.
- UnionChord encloses the deviation between the cubic model and its flat interpolation through the Bernstein coefficients. The triangle union uses each chord's stored error for its single-chord upper, lift, sample distance and grown index boxes. FinitePlaneReference validates projected footprints, and rejected or trimmed groups retain their ordinary distance path. A virtual subdivision preserves both directions of the chord certificate under the offset bound in section 3.
- The complete-witness lower bound uses the joined index. With a cropped witness the lower path retains all original support leaves, and the exact support rules stay in force. SurfaceForest's approximation floor uses maximum_error from the original facet leaves.
- Work and storage can share the existing AtlasOps primitives without constructing a new timed or traced operation. The affine translation unit is compiled with FP contraction disabled. R11's switch and closed-row exceptions are explicit in the order.

Contradictions found: none. The word resolution in sections 2.3 and 2.6 refers to the current facet preparation resolution, including on a refinement.
