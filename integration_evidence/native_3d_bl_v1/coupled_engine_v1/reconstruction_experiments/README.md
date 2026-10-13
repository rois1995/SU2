# Bounded reconstruction thought experiments

Run from the repository root, one script at a time. Existing SciPy/NumPy are used
only by these offline experiments; they are not new SU2 dependencies. The saved
binary64 frame is the independently audited rejected 100-cell cube at checkpoint
15e5b2575e. Candidate geometry/quality/length calculations here use floating
arithmetic and a fixed Gauss rule, rather than the native authoritative gates.

The chain models enumerate tetrahedra on declared point sets and enforce the
prescribed oriented-face boundary with binary variables (10-second solve cap).
They explore fixed surface, larger cavities, selected coordinated movements,
additional interior points, and seam insertion with surface reconnection. The
unsplit sanity control admits the original patch when its single oversized edge
is exempted; a relaxed split control also finds a closed replacement. Neither
control constitutes an accepted adaptation. Strict models so far reject the
sampled proposals; this is not a proof that the full target is infeasible or
that a larger/different point distribution cannot work. No production operator
or acceptance threshold is changed by these probes.

The direct two/three-vertex placement scans and 2–3/3–2 flip probe found no
successful complete strict replacement. Preserve this negative evidence before
trying new point distributions or imposing a different target. The native tests
separately validate the new planar strip's geometry and its frozen-target
rejection on this saved obstruction.

The joint SLSQP probes subsequently found a shape-and-length feasible placement
for the ordinary wall/far-marker case. Native and independent validation of that
explicit witness are in `../validated_witness_v1/`. Common displacement damping
alone did not discover a complete witness; next compare stiffness-dependent
normal/tangential damping without changing the target field. The separate-marker
optimization still rejected its sampled placements; optimizer termination by
itself is not a feasibility or acceptance certificate.
