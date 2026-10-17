# Exact first-row ranking without unnecessary full-edge work

Scoped checkpoint on codex/native-3d-bl after b1b2f9f1ec. Full Goal3 ACTIVE/incomplete.
[Design/proof and fixed acceptance scope](../integration_evidence/native_3d_bl_v1/oneram6_uncertainty_v1/deferred_first_row_lengths_v1/DESIGN.md).

The actual M6 source-support run still timed out during initialization; geometric
edge-cut preparation alone consumed192s on rank2. The existing ranking is shape
first, full length only for exact shape ties. A unique maximum never needs a full
edge integral for ranking. Every selected cell still undergoes mandatory full
composed seeding and unchanged final acceptance before CFD publication.

All existing geometric proposals/height/skin checks remain. Retain tied shape maxima
in original order, measure full lengths only for ties, preserve the original first
choice on equal lengths. No tolerance, cache, configuration option, point-placement
variant or target change. The bounded13-background/16-cell proposals fit existing
private scratch; superseded shape candidates enter the existing pruning counter.

Serial71 cases/14,440 assertions pass, including a full-reference unique maximum
and an exact tie where a localized metric feature makes length select a different
background. The test also checks mandatory seed edge work. MPI ownership/subset,
atomic/work/clock, curved/reader checks pass. A frozen M6 replay selects exactly
the sealed legacy geometry:1600 unique actual queries versus219,532 previously;
every remaining sensor/composed-tensor row is an exact subset of the old trace.
Ranking edge evaluations are0 on this64-parent replay. It remains a private subset,
not an accepted full wing or a whole-event speedup claim.

The complete changed M6 backend replay is terminal in
`integration_evidence/native_3d_bl_v1/oneram6_uncertainty_v1/construction_deferred_lengths_v1`.
Same frozen original UPPER6 field, geometry, h0, q/J/length and300s control; no CFD
advancement. All38,800 parents initialize in26.793s, producing536,159 PRIVATE cells.
Mandatory full seeding then uses249.789s and measures32,832 cells before the deadline.
No volume operations or transfer run. Geometry-cut preparation alone reaches172.949s
on rank2. Event rejects at301.004s, preserving accepted CFD; the successful fixture
exit code is not mesh acceptance. No complete rejected private wing was exported.
No whole-event speedup or construction-feasibility conclusion follows from this.

Actual N4/M2 RANS/SA/current+BDF workflows at initial scales1/10 pass independent
geometry/full composed field/height/sensor transport/conservation/SU2+CGNS and real
restart audits. The combined ordinary/grouped/cold/finer campaign has6 completed
construction/removal engine phases, then FAILS the known finer-sensor control at
4096 work/162 rounds. Its initial and rejected grids are byte-identical to the
already qualified unchanged-baseline failure. Do not label the whole campaign
passing, rerun that baseline build, or change its gates/cap to hide the limitation.

[Verified results, hashes and scoped limits](../integration_evidence/native_3d_bl_v1/oneram6_uncertainty_v1/deferred_first_row_lengths_v1/summary.json).
`summarize.py` independently rechecks current source/build/binary/helper pins,
executed source capsules, logs, frozen query/geometry equivalence and baseline
failure grids without launching native jobs. Initial postprocessing copied a byte
buffer repeatedly and was stopped; a Python3.8 Path API incompatibility also failed
before writing a summary. Both are reporting issues, fixed without repeating MPI.

Next substantial work must address mandatory seed query geometry/repeated edge
work, with unchanged frozen-field and final mesh contracts. Do not repeat this
300s replay unchanged, raise limits, introduce scenario dispatch, or add another
constructor fallback. General accepted M6 construction, calibrated global error/
allocation, full Goal3 gates and lifecycle affordability remain open.

One owned heavy job, nice19, build-j1/thread1/MPI<=4. Host~39--40GiB available and
816MiB swap unchanged during build. No cluster execution or full-suite-per-edit.
