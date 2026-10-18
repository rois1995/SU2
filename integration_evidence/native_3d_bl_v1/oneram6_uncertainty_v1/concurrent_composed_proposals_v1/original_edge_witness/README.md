# Original-input edge-star witness

original_star.su2 is a viewable five-tetrahedron ORIGINAL mesh patch, not the late
rejected working mesh or an accepted adaptation. Its local0..6node numbering maps
to global IDs in witness.json. The ten faces are artificial diagnostic interfaces.
The original reported centroid and obstructing face match; full late-cavity
connectivity/version equality is unproven.

metric_diagnosis.json uses the archived original sensor values, exact rational
coordinate-gap checks against configured wall boxes, and the analytic integral
of sqrt of the linear P1 quadratic form along an original edge. It shows this
original witness is outside BL support and has sensor edge length2.60882008>1.8.
It is not a feasibility proof or a native reconstruction replay. Use the full
original field/reference for actual-query local experiments; do not rescale the
clipped values or interpret artificial patch boundaries as physical walls.
