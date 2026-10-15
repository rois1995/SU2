# Goal3 — coordinate insertion with private connectivity repair

Repository `/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated`, branch
`codex/native-3d-bl`, following published shared-apex checkpoint
`c289837b2e99877ba502ac139d1b4813ce695a01`. Full Goal3 remains active and incomplete.

## Actual missing capability

The1820-cell source-pinned residual has46 long edges, including four interior
edges. Replaying the worst interior edge42,65 gives a four-tet point-only failure
(253 placement trials, shape loss1.19998) and a28-cell/two-apex coupled failure
(256 placement trials, shape loss .107093). Both retain valid geometry/height;
neither reaches the full metric gate because inherited connectivity remains poor.
The preceding full grid independently passes geometry/height/shape; this is an
operator blockage, not proof of global target infeasibility. Node IDs refer to the
reloaded residual stage, not another mesh's numbering.

## Connected proposal

`PlaceSplitPatch` now shares its existing maximum256 trials between placement and
point-preserving private connectivity repair, reserving half for the latter. The
existing two/three/four-tet flips and constrained movement use the same frozen
original field. No point retirement is allowed in this phase. An optional movable
set restricts displacement to the inserted point and fully admitted support apices;
other existing repair callers retain their original default behavior.

Only reserved apex identities may change. Stationary apices retain their original
IDs; moved apices use their reserved fresh IDs even if movement starts after the
placement phase. Complete original edge/apex stars, physical skin, first-height/
normal-foot/geometry and full actual-field Refine/progress remain mandatory.
Private shape improvement is not acceptance. No metric/fade/height/quality limit,
64-cell admission or global4096-work ceiling changes. The existing private scratch
admission covers the bounded additional working patch; the same FrozenField is
reused rather than built again. Phase trial/flip/move counters are exported.

## Qualification plan and current state

One connected build, then the actual failed point/cover probes, the earlier passing
26-cell cover (to detect placement-budget regressions), and focused movement-mask/
existing repair controls. The strict same1402-cell complete-target run must keep
all final gates and preserve failures. A meaningful bounded MPI2 follow-up uses
that same original input, without relabeling old MPI1/4/CFD/restart evidence.
Full coarse curved construction/removal, wider geometry/fields, transfer/current+
BDF/M<N, viscous accuracy and lifecycle affordability remain required for Goal3.

## Connected dependency escalation

The initial shared-budget attempt preserves the earlier26-cell passing cover:
129 total trials with one flip (previously132 placement-only trials),464 assertions.
All five focused existing repair/movement-mask controls pass. The hard28-cell
residual still fails after three flips; its bad cell contains a nonphysical vertex
fixed at the artificial interface. Global mesh inspection proves that vertex184
is free, has a12-cell star, and expands the union28→36, below64.

The existing actual-field repair hint now reaches the engine. Failed admitted
placement can request that exact complete vertex star; Directory batch metadata/
payload validates its global physical classification and versions. Only globally
free hints enter the movable apex set. Repeated/end-point/physical/oversized hints
stop, the total apex bound stays eight, every new union must fit64, and every
private trial is charged to the same4096 global work ceiling. No blind radius
expansion or new target is introduced. `curved_refit_interior_dependencies` replays
this exact mechanism; native qualification is pending the connected rebuild.

## Strict whole-target result

The connected engine on the same1402-cell input advances94 insertions to1842
cells, versus88/1820 before this change. It still FAILS at4096 work:40 oversized
edges, q .200793/J .050224, squared excess1.587450761→.065596571 (previous
remaining .188121334). Phase counters prove907 connectivity trials,29 flips,10
moves and three actual dependency expansions; no speedup claim from rejected grids.

The original-field residual export classifies16 cylinder edges,22 cylinder/cap
junction edges and two interior edges. Maximum interior length1.811106875 replaces
the preceding2.125178 obstruction; maximum overall1.948298940. Thus the dominant
remaining construction issue is surface/junction repair. The isolated expanded
36-cell replay still rejects (shape loss .062401), even though global edits make
further progress. Preserve both facts rather than declaring local/global feasibility.

### Next source-level restriction to verify

`SplitSurfaceStrip` currently projects a curved feature midpoint through all
authorities only when `moves.empty()`. Interior-only apex movement can therefore
route a projected wall split into the off-edge chart/marker-junction rejection,
although the physical endpoints/feature did not move. `RefitWallSplit` uses this
operation twice: first to split the skin, then with its interior apex moves.
Verify this on an actual remaining rim cavity before changing the shared projection
rule; retain every finite-chart/marker/geometry and final metric check. This is a
source-level suspect, not an established complete explanation for all38 boundary
residuals. No more unchanged-budget whole-grid continuations are planned.

Local reference inspection: `/home/rausa/Software/mmg/src/mmg3d/anisomovpt_3d.c`
uses a volumetric barycenter and minimum-quality checks, with its own metric
transport assumptions; do not transplant it as actual-query BL optimization.
`/media/rausa/4TB/MeshAdaptation/Scripts/FunScripts/MetricOptimize3D_Fun.py` instead
contains coupled coordinate least-squares and spatial metric derivatives, a useful
algorithm reference if coordinate minimization remains blocked. Its nodal cell
metric averaging/shape-size gates differ from native's actual-centroid/full-edge
contracts. No external source was copied or new dependency introduced.

## Completed checkpoint qualification

The final-binary regression cover passes129 trials/one flip, and the five focused
repair/movement-mask controls pass. Current MPI2 bounded protocol/source-reference
audit passes512 work/1705 rejected cells,286 joint trials/24 validated, including
60 connectivity trials. All meshes in protocol stages remain rejected.

The entire1842-cell result independently passes exact positive volumes, no overlap
over60042 admitted pairs, closed oriented ball skin,82 wall normal feet/first
height (relative error3.06e-15) and q/J arithmetic from actual-centroid exported
tensors. All40 oversized native full-composed edges remain rejection gates.
Read `joint_connectivity_summary.json`; all owned jobs are terminal. Goal3 remains
active, with all original integration/physics/performance gates intact.
