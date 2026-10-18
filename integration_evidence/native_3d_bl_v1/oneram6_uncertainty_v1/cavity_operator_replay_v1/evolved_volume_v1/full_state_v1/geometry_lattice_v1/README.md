# Frozen M6: binary64 exact-facet obstruction

The archived frozen M6 target cannot be constructed under the current exact-facet,
conforming binary64 mesh and full composed-edge rules. This is a sufficient local
obstruction; it does not identify every construction/performance issue elsewhere.
No geometry, metric, prescribed height or acceptance rule was changed.

The current terminal mesh still contains edge211--3481 between two SYMMETRY
facets8916/2297. Native authority reports different canonical charts and no analytic
reference. Exact rational orientation is nonzero (about-2.04378e-26): the two
almost-coplanar facets are treated as distinct planes and their intersection as
an exact physical crease. Under the no-gap/conforming exact-facet contract that
crease must remain covered by mesh boundary edges.

## Exact finite-coordinate certificate

Every binary64 value in a coordinate interval belongs to its finest possible
uniform binary lattice. Scaling the three chord components by those axis spacings
gives integers. A point a+t(b-a) on all three lattices requires t=k/g, where g is
the GCD of the three integer chord components. This is a necessary condition even
when the coordinate interval crosses a binade; actual binary64 values form a
subset of that lattice.

Here the integer chord is(-34852961739566,1044514287306,2774751391344372), GCD2.
Thus the midpoint is the ONLY possible interior binary64 point. Exact Fraction
checks establish that the midpoint really is representable. Both resulting
subsegments have GCD1, so neither can contain ANY further binary64 mesh vertex.
The adjacent facet triangles have at most1and2interior binary64 points respectively
from their exact integer-plane lattice counts. Those triangle counts are upper
bounds, not a claim that every lattice point is a binary64 value.

The certificate includes self-checks on adjacent/two-step doubles and a simple
integer triangle. Run `python3 certificate.py`; it performs no SU2 run.
See [exact certificate](certificate.json) and [source](certificate.py).

## Separate native frozen-field measurement

The actual original61330-node P1 sensor, original geometry, core
.003215747252591252 and frozen outer profile.875 are unchanged. All37original
bbox donors are retained. Native TargetEdgeLength evaluates the complete geometric
BL composition, with its actual geometry breakpoints and source-resolved intervals.
This is measurement only; no reconstruction or acceptance is attempted.

| Interval | Full composed length | Limit |
|---|---:|---:|
|Entire edge|12.7963549323|1.8|
|Endpoint211 to the only midpoint|11.5717815434|1.8|
|Midpoint to3481|1.22457338888|1.8|

The child lengths sum to the parent within1e-9. Sensor-only original-P1 lengths
are.32501548515 overall,.16560745520 left,.15940802995 right. Thus this is BL demand
meeting a discrete exact-geometry constraint, not an oversized sensor request.

Any conforming mesh preserving the two exact planes must cover the crease's left
subinterval without an interior binary64 vertex. Some boundary edge must therefore
span that whole subinterval. Its positive full-field integral is already far above
1.8. More cavities, reconnections, point-search trials or ranks cannot supply the
missing representable boundary vertices. Removing an old endpoint does not create
an interior representable point or eliminate the required crease interval.

The coordinate obstruction is exact. The length values use native's fixed-accuracy
quadrature; this report does not turn that numerical audit into a proven interval
quadrature bound. The11.57versus1.8margin is large. The conclusion concerns this
frozen field/reference and implemented acceptance contract, not every outer profile
or every geometry representation, and not budget feasibility alone.

## Consequence for the investigation

Stop rerunning the unchanged frozen M6 case to seek an accepted mesh. It remains
an inspectable rejection/constraint-diagnosis reference. Budget selection and thin-BL
integration success remain separate findings; they never established construction
feasibility. The complete rejected grid and actual current stars are preserved.

A further positive M6 attempt needs an explicitly declared geometry/reference
change with its own accuracy contract, or a separately designed bounded geometry
representation policy. Near-coplanar facets must not be silently merged and the
frozen field must not be weakened during local operations. A user preference is
pending; no geometry-authority change has been made. Other full Goal3 requirements
and work on declared feasible curved geometry remain active.
