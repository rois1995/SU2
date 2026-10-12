# Conservative segment-box filter sign review

A donor tetrahedron is contained in its nodal axis-aligned box. A segment can
intersect that box only if its projected line intersects each of the xy/xz/yz
rectangles. For axes i,j, define

    f(x,y) = (b_i-a_i)*(y-a_j) - (b_j-a_j)*(x-a_i).

Its extrema on a rectangle occur at the two selected opposite corners: choose
x lower/upper according to the sign of -(b_j-a_j), and y according to the sign
of b_i-a_i. A certified positive minimum or negative maximum separates the line
from the rectangle and therefore the segment from the original donor. Touching,
zero/uncertain values retain the exact segment/tetrahedron test. This prefilter
does not authorize containment or establish source coverage. Point queries keep
the existing coordinate box check.

All input coordinates are finite binary64 values checked during source build and
request admission. A nonzero coordinate difference is at least2^-1074; differences
are below2^1025. Products are at least2^-2148 when nonzero, and product sums are
below2^2051. The compile-time exponent/precision guard requires enough long-double
range/precision to represent these operations as finite normal values, with an
additional exponent margin. On narrower platforms the filter returns possible;
the exact predicate remains authoritative. It does not clip or repair coordinates.

Volatile long-double intermediates preserve four subtraction and two product
rounding points under the surrounding fast-math build. Corner-selection
subtractions are volatile too. For unit roundoff bounded by numeric_limits
long-double epsilon, each product of two rounded differences has relative error
at most gamma3 =3eps/(1-3eps). The final subtraction adds at most eps times the
computed product magnitude sum. Conversion of that sum's bound to its rounded
permanent adds a factor1/(1-eps). For the supported IEEE precision,64eps times
that permanent exceeds the resulting sign-error bound with substantial margin.
Finite/normal-range checks fail open. Thus only a resolved strict sign authorizes
rejection; unresolved signs do not trigger an exact calculation in the prefilter,
but leave the existing exact tetrahedron test enabled.

The proof assumes ordinary IEEE long-double operation semantics for the explicit
rounding points and that the supplied box encloses the immutable donor. Original
box/source validation is unchanged. No metric, P1 interpolation, composition,
fade, quality/sliver/length, geometry, MPI transaction or work-budget contract is
relaxed. This does not replace the exact sign/magnitude logic used for P1 weights
or segment/source interval integration.

Standalone safeguarded experimentv4 passes7258 exact binary64 Fraction closed-box
oracle cases in strict, productionfastmath and binary64-long-double modes. It
adds coarsened accepted-grid observations and endpoint reversal.0false rejections
in all modes; on the actual wide-range modes144/532sampled saved n8 candidate
boxes reject before tetrahedral testing. The narrow long-double mode disables the
optimization and retains all candidates. Earlierv1/v2 harness failures andv3
less constrained prototype remain preserved. Finite-corpus evidence complements
this sign-bound review; it is not a universal mesh or performance certificate.

Production integration in CNativeDonor3D uses this same fail-open projected-box
check only for known metric query footprints. Full-cell/unknown queries are
unchanged. donor_box_filtered is a subset of donor_filtered and donor_candidates;
its wall cost remains in the existing nested donor filter/search scopes. Counter
schemas must remain integral and consistent before performance aggregation.
