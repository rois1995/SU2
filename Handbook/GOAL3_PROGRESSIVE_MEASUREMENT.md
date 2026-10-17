# Progressive full measurement and the first M6 guide-query failure

Work on `codex/native-3d-bl` after686b13dc6b. Full Goal3 ACTIVE/incomplete.

The engine now interleaves actual full composed-cell measurements with complete
cavity transactions. Unmeasured cells have no selection/cache certification;
removed pending cells are skipped, and every replacement is fully measured.
Acceptance still requires every surviving cell to satisfy all original geometry,
height, positive-volume, q/J, full composed-length and transfer requirements.
Sensor-only P1 retains cheaper bulk seeding. There are no approximate edge lengths,
new limits/configuration options, input-mesh classes or Euler/RANS transition modes.

The pre-midpoint producer passes focused serial and MPI2/MPI4 controls, including
actual removal of unmeasured remote/concentrated cells, unchanged input on budget
rejection, no-edit full tail and an invalid far-tail field. The serial composed
fixture takes54->432 accepted cells and retires6 cells before separate seed
measurement. This is avoided seed work, not a claim those cells require no private
old-cavity integrals. N4/M2 RANS/SA/current+BDF/conservation/SU2/CGNS/restart audits
pass. Ordinary/grouped/cold construction-removal phases complete; the stronger
finer-sensor case still fails at4096 work/162 rounds. Do not label that whole
campaign passing. Sources/build/binaries/config/inputs/helpers and failures are
retained in progressive_measurement_v1.

## Actual difficult case, unchanged field and300s control

| Phase/result | Previous complete-seed run | Progressive run |
|---|---:|---:|
| Wall parents initialized | 38,800 | 38,800 |
| First-row phase, max seconds | 26.79 | 26.98 |
| Seed phase, max seconds | 249.79 | 0.12825 |
| Fully seeded cells when stopped | 32,832 | 64 |
| Volume proposals attempted | 0 | 1 |
| Accepted volume edits | 0 | 0 |
| Backend elapsed, seconds | 301.00 | 51.53 |
| Outcome | seed deadline | original-donor query failure |

Both meshes reject. Different work/termination means this is NOT a full-adaptation
speedup. Original owner-UPPER6 field, h0, fade, finer demands and strict final gates
are unchanged; normalization accuracy remains unestablished. No CFD step advances.
The failed full private grid was not exported by the existing backend; source/input
receipts and location/witnesses remain inspectable, not an accepted wing mesh.

## Exact diagnosis, not missing MPI support

A diagnostic-only repeat (51.28s) captures query
(0.5611371436594632,0.07257178734880862,0.02827475207391552) and202 admitted donor
keys. Independent exact rational orientation against every possible original cell
finds the represented point OUTSIDE the physical original volume. It is the rounded
midpoint of original vertices6758/6804. The IDEAL midpoint belongs to3 ALREADY
ADMITTED cells74753/162041/187880 with exact half/unit weights. Rounding offsets
(-5.55e-17,-6.94e-18,0) make corresponding barycentric weights negative, approximately
-1.93e-15/-1.77e-16/-1.99e-15. Existing strict rejection is correct for that literal
point. Increasing donor counts or moving the wall metric would not solve it.

[Exact containment packet](../integration_evidence/native_3d_bl_v1/oneram6_uncertainty_v1/progressive_measurement_v1/missing_query_audit.json),
[midpoint witnesses](../integration_evidence/native_3d_bl_v1/oneram6_uncertainty_v1/progressive_measurement_v1/rounding_witness.json).

## Exact midpoint follow-up and qualification

Shared BarycentricMidpoint/QueryMidpoint now represent a declared midpoint exactly
for ORIGINAL P1 sensor interpolation, then compose BL at its represented coordinate.
Explicit midpoint guide/score callers use it. Arbitrary Query(Point), centroids,
full edge integration and final acceptance stay strict. No ordinary point-cache
entry is created by the midpoint query. Exact closed/isolated physical contacts
retain priority over declared extensions; no endpoint-tensor averaging across
original pieces. A regression checks the actual witness and strict-cache separation.
[Design and pending results](../integration_evidence/native_3d_bl_v1/oneram6_uncertainty_v1/exact_midpoint_queries_v1/DESIGN.md).
Current connected source passes16 focused serial cases/8980 assertions, MPI2/4,
and independent N4/M2 RANS/SA/current+BDF/conservation/SU2/CGNS/restart. Both controlled
accepted meshes and independent final masses remain194/240 cells and19.98080082/
19.93548877 versus20. This is scoped workflow correctness, not a full regression.
The stronger finer-sensor case remains rejected at4096 work; the combined campaign
is FAIL, preserved separately from successful ordinary/grouped/cold stages.

Actual unchanged M6 now passes the midpoint and runs30 private query/reconstruction
rounds, then rejects with `Uncovered native 3D source segment endpoint.` at84.355s
backend/90.698s launcher. No accepted wing mesh or solution transfer. Exact-midpoint
sensor semantics fix the demonstrated guide error; they do not authorize arbitrary
outside new nodes or relax strict source-resolved edges. No next local fallback or
increased work/tolerance added. The full private rejected wing is still not dumped.
[Current pinned checks/failure summary](../integration_evidence/native_3d_bl_v1/oneram6_uncertainty_v1/exact_midpoint_queries_v1/summary.json).
[Actual M6 receipt](../integration_evidence/native_3d_bl_v1/oneram6_uncertainty_v1/construction_progressive_midpoint_v1/result.json).
Full Goal3/accepted-M6/feasibility/affordability remain unproven. Source/build/binary/
configuration/input/helper hashes are verified by verify.py without restarting runs.

User extension2026-10-18 adds an opt-in budget-aware BL policy, with strict default
compatibility and every existing mesh/transfer gate retained. Diagnosing composed
cost by wall distance/surface region/normal-versus-tangent spacing comes first.
[Policy scope and required evidence](GOAL3_BUDGET_AWARE_BL.md).
