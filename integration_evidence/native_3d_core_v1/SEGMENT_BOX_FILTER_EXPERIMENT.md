# Segment-box prefilter experiment — 2026-10-12

Working repository: /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated.
Branch: codex/native-3d-transport-accounting; production checkpoint7f76fd9a279cf384a37f8ae137dc4dc26212ce82 remains unchanged.

The saved n8 worker profiles still put donor filtering first. Second coarsening
exclusive donor time was62.847s in routing and70.331s in packing; nested filtering
was62.181/69.991s. Nested filtering also spans seed/update and must not be added
to the exclusive donor phase. Byte-unchanged stages and edge tracing slowed too;
these two contended runs do not isolate packing or establish regression. Full
values and source-log identities: PACKING_SAVED_PHASE_REVIEW.json.

The proposed broad phase rejects a segment only when it is strictly separated
from the donor axis-aligned box. Existing coordinate interval checks remain;
three projected line/rectangle tests use extreme rectangle corners. A two-product
long-double sign filter accepts a sign only above64epsilon times the computed
absolute product sum, with finite/range checks. Unresolved/underflow/overflow
cases retain the downstream exact segment/tetrahedron test. No midpoint-based
box reduction or tolerance is used to decide separation. Endpoint/face touching
is retained. Finite-input/error-bound/compiler assumptions still need an explicit
production review; a finite corpus is evidence, not a universal proof.

segment_box_filter_experiment_v3 PASS3256 cases against an independent exact
Fraction oracle using the actual binary64 values, under strictO2, production
O3/march-native/fast-math/fno-finite-math-only and binary64-long-double modes.
The corpus includes random boxes, exponent scaling from-1070to1000, ULP point
boxes, zero-length segments, face/endpoint touch, invalid fail-open controls and
sampled actual n8 mesh/source boxes. No true closed-box intersection was rejected
in any mode. Strict/native/ld64 leave0/7/83 false positives, respectively; those
cases stay eligible for exact downstream work.

On158 saved n8 candidate-box observations,28 (17.72%) are certainly separated and
can avoid the exact tetrahedron predicate. This is a small sample of accepted
fine-mesh edges, not an exhaustive replay of the expensive rejected coarse
cavities, actual speedup or MPI evidence. It does not reduce the ADT candidate
scan itself. No production predicate, metric, query, operator, config or guard
changed in this experiment.

Preserved harness failures: v1 Python3.8 lacks math.nextafter; v2 a generated
newline literal failed compilation. v3 uses a fixture-specific IEEE bit-step
helper and corrected C++ output. Original source/logs/receipts remain separate.
Source/corpus/compiler commands/stdout and binary digests are retained in v3;
compiled experiment executables are reproducible local artifacts, not required
repository files. One CPU/nice19/thread1 correctness only, no SU2/MPI/profiling run.

Next: review the conservative sign bound, include coarse accepted/private-cavity
edge observations and reversal/touching adversaries, then test a bounded
production integration against exhaustive donor IDs and current MPI/whole-driver
contracts. Keep the exact predicate authoritative. Do not call Goal1 complete or
claim a performance win from this standalone filter.
