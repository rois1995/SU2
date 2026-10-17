# Canonical owner point samples — validated connected batch

Full Goal3 ACTIVE/incomplete. Base5ebed3f3b81244c54fd89b6814d70a8f12a64987,
branch codex/native-3d-bl. Source implementation and connected serial/MPI/runtime validation PASS; full goal incomplete.
Evidence: integration_evidence/native_3d_bl_v1/oneram6_uncertainty_v1/owner_point_samples_v1.
The preceding whole-donor caches remain withdrawn and their evidence preserved.

## Capability and scope

Point-only partition-weight and mesh-return queries previously imported whole donor
stars and built a second FrozenField index per batch. The replacement queries the
existing immutable source-owner ADT. Each owner considers exact physical P1 donors
first, then explicitly eligible extensions, in canonical node-key order. One local
winner per requested point is returned; the requester selects the same global
physical-first/canonical winner. Actual source publishers accompany ordered original
node IDs, native barycentric weights and sensor-only tensors. Missing coverage and
elected storage/protocol errors return no partial answers, never zero/fallback fields.

Local and owner queries share the SAME strict InterpolateSensor arithmetic. Full
barycentric precision is preserved by binary mantissa chunks plus exponent through
the existing fixed-record integer/double codec; no long-double padding or reduced-
precision interpolation is sent. Sensor interpolation happens before transport.

WorkingWeights and ReaderSlices use one bounded2048-entry sensor-only memo for the
immutable call. It replaces their previous per-batch field cache/index; it is not
an overlapping index. Original donor cells/stars are not cached. The original source
cannot be rebuilt during this lifetime. BL composition still runs at EVERY actual
query, including memo hits. Core/fade/finer demands and all mesh/transfer gates are
unchanged. Per-batch copies pin answers while FIFO eviction maintains the bound.

Requests, eligibility declarations and replies use the existing fixed-size house
transport on the source's explicit MPI communicator. Eligibility belongs to the
requesting rank, so an original owner need not carry that requester's geometry
pointer. Native storage admission includes source/index/query scratch, peer request
and reply buffers, memo/current-batch storage and eligibility. These are requested
byte models, not allocator/RSS or aggregate MPI RAM. Caller outputs publish only
after matching collective failure votes.

## Validation required before a gain claim

- Canonical physical/extension priorities, shared-boundary ties and actual source
  publishers on shifted storage; exact direct-field sensor/weight comparison.
- A288-cell genuine incident star: ordinary full import still rejects the256 cap,
  point-only sampling succeeds without raising that cap. Empty ranks, missing/
  nonfinite points and asymmetric storage refusal; full-precision wire round trip.
- Existing bridge/current+BDF/conservative/barycentric/failure checks and actual
  composition on memo hits. Strict interpolation, composed-edge and declared
  exterior-coverage controls, since their arithmetic helper is now shared.
- Same512 original M6 cells from the archived owner-UPPER6 backend field, exact
  query coordinates/sensor/composed tensors and partition weights against fresh
  imports; saved serial reference and MPI1/2/4 consistency. Hosting-rank RAW9
  integration archives are untouched. Serial pilot first; no CFD or construction.
- Affected curved-extension runtime plus initialized RANS/SA construction/removal
  N4/M2, independently audited geometry/height/full field/current+BDF/SA/restart/
  SU2+CGNS. Scoped accepted meshes remain distinct from an accepted M6 wing mesh.

Report source point queries/payloads/time/owner search/interpolation, memo hits,
index builds, preparation memory and complete scoped runtime. No full M6 adaptation
speedup from a selected-cell kernel. Build-j1/nice19/thread1, one owned heavy job,
MPI<=4 and host memory observations; foreign jobs/ClusterResults untouched.

## Remaining full goal

No accepted M6 construction, original-field reassessment/correction epochs,
calibrated global uncertainty/error allocation or complete unsteady affordability
is established by this batch. Broader reconstruction-campaign support, scoped
composed-edge reuse and independent transactions remain separate requested work.
Do not raise work/time/donor caps or weaken unchanged mesh gates to obtain a pass.


## Achieved evidence

Same512-cell M6 exact query/weight checks PASS1/2/4 ranks. Preparation.03337/.03086/
.02960s versus cold.12975/.11124/.08883s; no second index builds. Serial966 lookups/
1594 memo hits for2560 actual compositions. Larger4096-cell MPI4 replay PASS all
20480 query tensors/weights;8098 lookups/12382 hits,0 versus1852 index builds,
.26134 versus2.28299s and capacity-exceeding lookup counts requiring4002 evictions.
These are selected-cell contended kernel comparisons, not full M6 adaptation gains.
18 serial cases/2941 assertions and MPI4 source/bridge controls PASS. Affected
curved return and independently audited initialized RANS/SA N4/M2 BL construction/
removal/current+BDF/SA/restart/SU2+CGNS PASS.24->6 cells, unchanged full constraints;
no accepted M6 mesh or converged viscous result. Composed gradation violations remain.
[Exact source, comparison, scope, receipts and reproduction](../integration_evidence/native_3d_bl_v1/oneram6_uncertainty_v1/owner_point_samples_v1/README.md).
Successful build7m48s/peak process2.39GiB; largest serial pilot process1.63GiB,
host38–39GiB available. One owned heavy job/nice19/thread1/MPI<=4. Long-double
codec compile rejection/source patch retained; integer mantissa transport passes
full-precision round trip. All owned native handles terminal; current binaries rebuilt.
