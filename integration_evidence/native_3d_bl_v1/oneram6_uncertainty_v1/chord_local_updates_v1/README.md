# WITHDRAWN: local interval updates

69 actual original M6 edges, selected from a declared mixture and including the32
largest candidate boxes;0--2544 admitted wall facets per edge. Two alternating-order
legacy/candidate passes compare cuts,65 distance queries, feature/piece/comparison
counts and exhaustion status EXACTLY: all138 comparisons pass, no exhaustion.
Aggregate kernel time legacy/candidate0.64019/0.64396s and0.64345/0.64650s: ratios
0.9941/0.9953, no useful measured improvement. These are contended component samples,
not a full remeshing benchmark. Compile peak/resource files and inputs retained.

The indexed interval/splice change is WITHDRAWN. Exact source and added test are in
withdrawn.patch; candidate_envelope.hpp and legacy_envelope.hpp preserve both kernels.
No full native build/MPI/CFD run was needed. An initial helper relative-path error
occurred before compilation; fixed in run.py. Source files restored exactly to2ab7.

Next: measure excluding geometrically inactive support outside the existing fade
radius, retaining every active crossing and actual-query BL. Copy avoidance was not
the demonstrated lever; do not retain this as a claimed speedup or add it on top.
