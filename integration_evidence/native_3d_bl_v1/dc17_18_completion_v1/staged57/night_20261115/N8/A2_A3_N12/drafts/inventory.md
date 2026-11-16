# N8 A2/A3 + N12 source inventory (read-only draft)

Source tree inspected at accepted/reverted commit `03fffc52e8`. A1 had failed and
was reverted before this inventory; no source was changed here. N8 A2 and A3 are
print-only requirements/census work. N12 is also print-only and is combined in the
same requested build/run. This is source evidence, not an endorsement of the M6b
orientation premise or of any wall-geometry theorem.

## A2 requirements record

`EngineOptions` in `Common/include/adaptation/CNativeEngine3D.hpp:103-115` contains
`maximum_work`, `maximum_seconds`, `memory_ceiling`, `job_memory_limit`,
`surface_tolerance`, `wall_heights`, and `region_walls`. It has no `hmin`, `hmax`,
`ARmax`, `hgrad`, or metric-family complexity fields. `MetricComposition` is only
`std::function<Tensor(Point, Tensor)>` (`Common/include/adaptation/CNativeField3D.hpp:33-34`),
so the engine cannot inspect its internal size controls. This is a real source-data
boundary, but the values already exist at the caller: `CNativeMetricFamily3D::controls`
records `hmin,hmax,ARmax,hgrad,complexity` (`SU2_CFD/src/solvers/CSolver.cpp:4085-4087`;
`SU2_CFD/include/adaptation/CNativeMetricFamily3D.hpp:42`), and the `CConfig` getters
are used when that family is populated. `CNativeRemesher3D::Remesh` receives the
config and sets current EngineOptions (`Common/src/adaptation/CNativeRemesher3D.cpp:201-218`);
BL composition is created with config and mutable control at `387-414`, or
`CSinglezoneDriver.cpp:318-345` / `392-425`.

**Reading used:** carry the five source size controls as a small read-only metadata
record into `EngineOptions`, then emit that record only under `options.timings` at
Adapt entry. This is the minimum caller-to-engine path consistent with “at the top of
Adapt” and avoids invoking/evaluating the opaque callback. It adds metadata, not a
new remeshing option or branch. For non-family cases use the same named config
controls at the remesher boundary; do not infer them from the callback or the output
mesh.

Other requested fields already have direct sources:

| Requirement field | Current source / reading |
|---|---|
| `composition_present` | `bool(composition)` at the `Adapt` parameter boundary; says only whether the callback exists. Do not call it to inspect the target. |
| `mapped`, `entity_geometry`, `curved` | `PlanarReference::Mapped()`, `EntityGeometry()`, `ExtensionComponents()` in `Common/include/adaptation/CNativeBoundary3D.hpp:58-60,99`; current `curved` definition is exactly nonempty extension components (`CNativeEngine3D.cpp:1353`). |
| geometry tolerance | `options.surface_tolerance` is the frozen physical tolerance (`CNativeRemesher3D.cpp:213`). `BoundaryParameterState::GeometryTolerance()` is a distinct mapping/query tolerance; keep the two separately labelled if printed. Reading `tolerance` as the EngineOptions physical surface tolerance follows its direct use in Engine. |
| feature policy | `geometry.Mobility()` (`CNativeBoundary3D.hpp:117`); `MobilityPolicy` stores `angle`, identity, protected `features`, `corners`, and facet-to-root `sheets` (`CNativeAffineReference.hpp:94-100`). `PrepareMobility` builds from configured `ADAP_ANGLE` and marker roles and reports protected edges/corners (`CNativeRemesher3D.cpp:68-100`). Report policy-present from nonnull/identity; count `features.size()` and `corners.size()`; count sheets as unique root IDs in `policy.sheets` (not number of map rows). |
| wall rows | `EngineOptions.wall_heights` and `region_walls` (`CNativeEngine3D.hpp:110-111`). The driver fills per-marker first height, growth and thickness (`CSinglezoneDriver.cpp:330-337`, repeated first-row at `411-417`). Emit each marker row and the global height tolerance; preserve missing region policy as missing, not zero. |
| budgets | `EngineOptions.maximum_work`, `maximum_seconds`, `memory_ceiling`, `job_memory_limit` (`CNativeEngine3D.hpp:104-109`); engine applies/validates values collectively (`CNativeEngine3D.cpp:2255-2268`). Preserve the configured zero/`SIZE_MAX` values as supplied; do not substitute derived work limits into a field named configured budget. |

`surface_tolerance` and mapped `GeometryTolerance()` are separate live source values;
printing a single unnamed “tolerance” would lose information. This is a report-row
ambiguity, not a result-gate contradiction. Keep both labels distinct or use the
reading above for the requested `tolerance` field and report the parameter tolerance
separately where mapped state supplies it.

## A2 switch census

`switch_table.csv` records all matching conditional source lines inside the `Adapt`
body plus the `PrepareSurface` entry precondition. Main interpretation: **R** checks
or validates an explicit requirement/policy; **P** reports/selects based on an
observable geometry or policy property; **X** chooses a code path based on the
current operation/target/case data. “X” describes existing dispatch; it does not
assert that the condition is an inappropriate scenario selector.

The implementation has multiple predicates per line and line-number references have
moved. In this snapshot, broad token grep yields 145 lexical lines inside Adapt and 150
through the end of the file; those are not decision-site counts. The attached CSV
contains 80 matching if/else-if/for/while predicate lines through PrepareSurface,
with line, predicate text, and R/P/X classification. The review’s stated 119/128
count uses a different site definition; this inventory does not equate the counts or
use them as a gate.

Most sites are **X** (stage/algorithm routing). The **R** sites are explicit
cross-rank consistency, validity, surface/height/region-policy admission and
`PrepareSurface` preconditions: `CNativeEngine3D.cpp:1626,2135,2242-2254,2288-2324,
4205,4217,6283-6291`. The geometry property extraction is **P** at `1353`; policy
presence/count fields requested by A2 are observational data, not new switches.
Every other conditional involving `composition`, `surface_target`, wall-height
emptiness, `region_walls`, or `curved` in the listed Adapt interval is classified
**X** in `switch_table.csv` and identifies an existing branch, skip, phase choice,
allocation, or callback invocation.

## A3 M6b projection/orientation fixture

The accepted Euler293 reference is the N5 R1-repeat geometry descriptor’s embedded
native reference:
`staged57/m6_inputs/euler_adapted293_event1_v1/adapted_mesh_adap_00001.su2.native_ref`
(descriptor source line equivalent `N5/descriptors/geometry_euler293_R1_repeat.json:7-15`,
reference SHA-256 `51fcfd52b2c9a90b4f532d978807910bfccf2d977320a6ad8b45aecbf9502eb3`). Its
accepted-input M1 census is **82,559** selected marker-2 edges, of which **7,521**
meet the old geometry-need estimate; the all-edge value is the M6b population, not
7,521 (`N5/reports/N5_measurement_table_draft.md`, M1 row; accepted-repeat summary
and CSVs are linked by `N5/unit_measurement_receipts/n5_d9_geometry_euler293_R1_repeat.json`).

Existing D9 fixture setup is reusable evidence for import/geometry authority:
`CNativeAffineReference_tests.cpp:2883-2922` decodes the `.native_ref`, uses embedded
parameter state when no override is supplied, obtains the authority/entity flag and
geometry tolerance, and reconstructs the parameterized `PlanarReference`. The exact
accepted input descriptor has `SU2_D9_WALL_MARKERS=2`, no `SU2_D9_PARAMETERS`, and
pins the `.su2`, `.native_ref`, checkpoint, and run config. It does **not** load the
full composed target (`D9WriteSummary`, `...:2860-2877`, reports
`full_composed_target_loaded=false`).

The existing projection helper is explicitly not M6b coverage. It selects only
`row.second.need` and stops after the first attempted edge (`...:2801-2814`), so on
Euler293 it is one first-need-edge sample. For that one edge it scans surface faces
to form the incident-face overlay (`...:2712-2731`), computes Cartesian chord
midpoint (`2733-2740`), constructs a private view (`2742-2776`), then calls
`BindNewPoint(projected, incident, &edge)` (`2774-2785`). Its R1-repeat receipt records
M5 selected need edges 7,521, attempted 1, BindNewPoint success 1, Bind time
0.000290427 s, private context 0.000006422 s, aggregate work 101, and the edge CSV
hash `e19ab8eef41204c1b668f443dc3376218d9c5e437463689f00a5c4a3b2a7fd09`. This is a
single first-need sample and cannot be multiplied into a qualified all-edge runtime.

The requested M6b measurement changes the population to every one of the 82,559
selected wall edges, not only the 7,521 estimated needs. It requires a projected
midpoint and exact `Orientation` signs against **all old physical wall faces in
each edge's tetrahedral shell**, including shell faces that do not contain the
edge. The old helper's overlay (`D9ProjectionOverlay`,
`CNativeAffineReference_tests.cpp:2712-2731`) only selects faces containing the
edge, so it does not establish that population. The accepted mesh import path is
available in `CNativeImport3D_tests.cpp:3057-3109`: it loads the `.su2` mesh,
loads its `.native_ref` sidecar, imports geometry, then builds `D9LocalityIndex`.
That index (`:2873-2878`) stores `physical_faces`, `owned_wall_faces` and
`edge_faces`, but its edge_faces insertion tests that the face contains BOTH
edge endpoints (`:2920-2924`), so it is NOT the full shell-face population.
The new measurement must index full cells incident to each selected wall edge,
then use D9EdgeCavity on those cells (or the equivalent full physical bindings).
This correction was caught by primary source review before any fixture edit.
This is the source-supported shell definition; it is not
the edge-incident overlay used by the old one-edge sample.

The old overlay scans 54,961 inventory faces per attempted edge (M3), giving a
naive upper loop count of 82,559 x 54,961 = **4,537,525,199** face/edge visits if
that scan were repeated for every selected edge. This is not a prediction of the
required shell-filtered algorithm's runtime or work. Neither the all-edge shell
population distribution nor its setup/projection/orientation runtime is present
in the old first-edge receipt. The fixture must report those values; the old
single first-need sample (M5 attempted 1, success 1) cannot be multiplied into an
all-edge qualification. No geometric sign theorem is inferred from the
endpoint-demand estimator, old face proof, or one `BindNewPoint` sample.

## N12 linked census: source findings and print-only conflicts

The existing `N12/drafts/source_inventory.md` is the detailed source trace for M24.
Its principal source facts are: the slot loop’s `dependencies`/`imported` are available
at `CNativeEngine3D.cpp:4035-4052`; quality and construction reach
`evaluate_candidate` at `5740` and `5789`; proposals retain `approved` and repair work
at `5765-5774`; quality commits occur after publication at `6206-6245`; construction
commits at `5920-5923` and `5967-5974`. Classify slots at the evaluate boundary using
all dependency owners for local/shared and any imported physical facet for
interior/wall. Do not classify all loop choices as trials.

Consolidated print-only discrepancies for primary review:

1. The requested slot trials are slots that enter `evaluate_candidate`, while existing
   ladder trials count one or multiple `note_ladder` attempts per slot (quality call
   sites `4172-4185`, `5587`, `5640-5722`). These sums cannot generally equal the
   existing ladder counters. **Reading used:** report slot-trial census and legacy
   ladder census side by side; do not assert equality as a production result.
2. `approved` after evaluation is not a commit: proposals can later fail clearance or
   reservation (`6081-6115`); commits mean successful publication (`6206-6245`).
   Construction `stats.accepted` is published later (`5920-5923`, `5967-5974`), and
   no matching construction ladder-published counter was found. Label approval and
   publication separately.
3. There is a whole private-loop/round busy timer, not a per-slot busy clock
   (`4033`, `6026-6035`). `note_ladder` times individual ladder attempts, potentially
   multiple per slot, so it cannot be relabeled as slot seconds. A per-slot elapsed
   interval is new instrumentation (print-only under `options.timings`) and requires
   definition of whether nested communication/other slot overhead is included.
4. Private quality does not currently compute `SameSkin` before publication; the
   construction path computes it at `5794`. A quality counter must compute it only
   as an observational post-trial/post-commit value and never feed a decision.
5. Per-proposal parameter edits are computed at `6131-6136` but merged in a batch;
   batch vector emptiness is not per-commit emptiness. Preserve per-proposal facts if
   counted. Coverage has source-supported proposal semantics: `fresh.coverage` is
   passed as the coverage-delta payload to `Directory::Publish` at `5881`; the
   directory forwards it through `PrepareParameters` (`CNativeDistributed3D.cpp:1619`)
   to `BoundaryParameterState::Prepare` (`:1441`). In private quality, each
   `proposal.fresh.coverage` map is unioned into the batch at `6097-6099` before
   publication. Thus the empty-delta observation can inspect the actual per-proposal
   payload's `faces` and `traces` maps; it must not infer per-proposal emptiness from
   the merged batch. This reading is source-supported, not an unresolved equality
   definition.
6. `repair_work`/`optimizer_work` measures optimizer work, not total engine work or
   wall time (`5762-5773`). Construction and quality publication counters are
   asymmetric. Separate per-stage values and do not reconcile construction commits
   against quality `ladder[].published`.

These are print-only measurement/scope contradictions; they do not change acceptance
or authorize implementing D section 5.11 lane behavior. The M24 table must show 6
ranks x 2 stages x 4 classes for both events, including zero-count rows, and include
local+interior share with the stated denominator. The older linked N12 inventory has
not been updated; its coverage-delta item is superseded by the source-supported
reading above. Parent review additionally specifies one class per evaluate_candidate
call (including unchosen construction peer calls; quality skips unchosen), RAII timing
around that call, repair_work attribution, and a fixed parallel 8-slot metadata array
so RepairProposal size is unchanged. Existing ladder trials remain distinct counts.

## Scope / limits

This draft inventories existing inputs and call paths only. It does not source-check
or endorse a sign theorem for M6b, does not determine a successful all-edge runtime,
does not implement A2/A3/N12, and does not update gates. A1’s failed identity result
remains the parent’s recorded/reverted item; this inventory does not reinterpret it.
