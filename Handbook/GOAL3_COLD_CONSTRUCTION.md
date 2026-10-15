# Goal3 — complete cold construction and removal

Worktree NativeIntegrated, branch `codex/native-3d-bl`. Full Goal3 stays active.
The published curved-target baseline is d4242c9c75. This connected batch completes
the cold/removal mesh milestone; full Goal3 remains open.

## Evidence and missing construction step

The first new direct24-cell/18-node coarse-cylinder probe generated976 private cells
but rejected. Actual frozen-field export found58 long edges (32 wall/24 rim/2 interior),
16 cap-shape failures, qmin .13888/Jmin .03397, Lmax2.05542. The previous surface planner
split only edges marked at the start; new wall diagonals could remain too long.
Preserved `region_construction_v1/native_cold_v1` and `native_cold_field_v1` include
source/binary/helper hashes and `region_proposal_rejected.su2`/reference. Field-export
PASS establishes execution/diagnosis, not admissible mesh generation.

## Connected implementation

- Close radial surface sampling over new actual-target long edges and geometry error
  before building the volume. Planar cap interiors are rebuilt, so their obsolete
  interior edges are not refined pointlessly.
- Reuse a rejected directed surface collapse only while both endpoint stars have the
  same versions. Every changed incident triangle invalidates the relevant versions;
  frozen coordinates/geometry/metric make other unchanged failures reusable.
- Use the same actual-field region planner without a normal band when no height is
  requested. Surface coarsening tests actual core-cell quality, followed by complete
  topology/geometry/no-overlap/metric validation. No Euler/RANS scenario dispatch.
- A single height-free core cannot resolve all interior fields; actual metric failure
  declines that constructor and retains the existing volume operators. No target is
  weakened. Domains beyond the256-original-donor regional admission also retain the
  previously qualified incremental path rather than failing at a global box import.
- Count private generated cells/surface/junction work on rejection as well as success;
  checkpoint junction probes, distinguish budget exits and include failing coordinates.

Preliminary serial `native_cold_v2` passes24 ->1368 BL ->68 released cells, with
2964+168=3132 work under4096 and a shared300s deadline. `native_complete_v2` preserves
complete difficult-target acceptance (1368 cells, q .211386792/J .052846703,0 long
edges), reducing work2996 ->2940 through rejected-star reuse. These are correctness
results, not measured lifecycle speedups. Primitive-runtime controls pass2557
assertions, including actual transfer/BDF and SU2/CGNS restart on the existing fields.

The final test build passes the whole24 ->1368 ->68 workflow on MPI1/2/4, with
construction/removal work2964+168 /3004+168 /3084+168 respectively. The same4096 work
and300s remaining deadline apply across both phases; remaining seconds use an MPI
minimum. BL qmin>=.21137/Jmin>=.05284/Lmax1.795195; released qmin .334181919,
Jmin .197166633/Lmax1.119411072. Rank-count BL graphs/placements differ admissibly;
released quality/length limits agree. These are correctness results, not a speedup.

Serial independent BL audit passes exact positivity, all vertex links/no positive
overlap, first feet/altitude, reference error and actual-tensor arithmetic. Released
independent audits pass all3 outcomes,68 cells/37 nodes, all vertex links, no positive
overlap, complete oriented physical skin, analytic reference/surface error and exact
constant-P1 tensor edge norms/shape. The tensor is asserted across every donor sample;
no released height constraint is imposed. Source coverage remains native evidence.

`native_complete_v3` still completes the known1402-cell target with2940 work. Its
accepted SU2 and reference bytes exactly match the prior independent case; original
sensor/reference data and geometry helpers match, with a verified reuse ledger.
The current native engine separately remeasures all frozen q/J/full edges.

Primitive-runtime transfer/BDF and SU2/CGNS restart control passes2557 assertions.
Its production sources/objects/libraries and complete test body remain unchanged;
the final harness binary differs only after cold logging/deadline checks, so the
control is explicitly reused in its unchanged scope, not reported as newly executed.

[Complete cold manifest](../integration_evidence/native_3d_bl_v1/region_construction_v1/cold_summary.json)
pins failures, field exports, complete current cases and independent audits. Inspect
`native_cold_v3/curved_cylinder_BL.su2` and `curved_cylinder_released.su2`, including
reference sidecars. No CFD solution is fabricated. Earlier failed976-cell proposal
and full measurements remain inspectable; diagnostic export PASS is not acceptance.

## Scope still missing

One complete visible radial/caps region is constructed on one worker. No arbitrary
CAD, exterior/holey domain decomposition, high-ratio multiband construction or
parallel generation/scaling follows. Original sensor-only P1, finer requests, geometric
BL/fade and h/q/J/length/geometry thresholds are unchanged. Stronger/variable sensors,
interacting walls and real unsteady CFD/viscous accuracy remain full Goal3 gates.
A1368-cell current CFD source usually exceeds the256 donor cap even though the frozen
coarse-source oracle does not. Complete-region distributed field queries are therefore
needed for repeated actual CFD rebuilding; the incremental-path preservation is a
compatibility safeguard, not a solution to that remaining capability.
