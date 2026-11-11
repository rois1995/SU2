# Connected surface preparation and BL fronts — build344

The missing capability remains a usable curved M6 BL, followed by the full RANS500k
workflow. This is the same connected construction cycle begun after terminal330.
Euler293's accepted two-event workflow and isolated restart are preserved.

## Implemented scope

FIRST_ROW now calls an explicitly provisional requested-wall phase of the existing
native engine. It reuses coupled split/collapse/relocation/reconnection, support
import, full-vertex reservations, migration, dependency checks and publication.
Construction measures positivity and geometry, without q/J ranking or vetoes.
No new scheduler, configuration, target policy, CAD requirement or budget increase.

Sided endpoint tangent guidance uses the transported flow core and the existing BL
target. Own-wall normal spacing does not enter a tangent chord simply through its
normal component. Other walls, actual geometry and existing thin-strip demand stay
relevant. Planning identity cannot certify full-target q/J or acceptance. Complete
surface certification and subsequent full-target remeshing remain separate.

Previously passing first children must survive. The current guard permits identical
physical support, including renumbering, with freshly checked children; unresolved
correspondence for edited protected support is refused. This is a conservative
limitation, not proof that redistribution is impossible. Missing rows remain debt.

BL transactions publish current parameters through a checked private receipt and
carry transported nodal cores. Physical anchors are canonicalized through the same
authority, retaining strict orientation/support checks. Relevant wall/carrier changes
invalidate the existing constrained/blended direction field. Pure interior changes
and equivalent canonical anchors do not. Direction maps and refresh overlap are
admitted; failed partial private mutation poisons the event. Growth uses current
constrained directions and refuses stale fronts. Reconstructing already advanced
fronts after relevant wall changes remains unimplemented.

## Demonstrated failures and corrections

- Build341: missing MetricStats declaration; terminal1, pins intact, all132 observed
  host identities absent before correction.
- Build342: five primitive cases passed; requested-wall refinement stalled. Planar
  production entry failed strict parameter publication because accepted topology
  stored canonical anchors while legacy mesh faces retained raw equivalent IDs.
- Build343: canonical publication/direction anchors and seam/orientation checks pass.
  Planar production entry passes69 assertions across fresh construction and retained
  continuation. Refinement still stalls after4 accepted splits.
- Build344: planning generation progress and ownership demand exclude unrelated
  raw interior-edge guidance. That guidance had polluted the four-generation
  stagnation test while requested-wall demand remained. The unchanged refinement
  case now reaches readiness. Full-target generation logic is unchanged.

## Evidence and limits

R=`integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57`.
`R/surface344_qualification.json` seals build344 and both serial checks:
160assertions/7focused cases,69assertions/1planar production-entry case. The latter
constructs64/64 first rows, advances530 column steps and continues its registry into
a second provisional event. Its384→3032→3572 tetrahedra are fixture counts, not M6.
Geometry/volume/skin/registry checks pass; this is not strict final-target acceptance.

All build/check pins and actual observed terminal handles were checked. Short checks
miss brief children in sampling; sampled individual RSS is not whole-job peak memory.
Build344 took72.170s; no performance comparison is claimed. All22 Euler payloads
and the same303 RANS mesh/reference/SA seed/config are unchanged.

Actual curved normal heights, alignment, spacing progression, coverage, collisions,
outer transition and constraint survival after all operators/migration remain
unproven. FIRST_ROW still requires one native worker; CFD MPI6 is not native parallel
layer generation. No new MPI milestone or full M6 attempt has run.

Next: qualify the connected consumer on actual curved M6 patches and neighboring
volume, including leading edge, symmetry, genuine features and junctions. Preserve
the queued curvature/target/conformity investigation and quantify inherited tangent
allocation versus the provisional1.1745M/500k discrepancy before changing policy.
Then the appropriate connected MPI milestone and same-seed full workflow, retaining
all geometry/q/J/size/height, conservative flow/SA transfer, CFD and restart gates.
