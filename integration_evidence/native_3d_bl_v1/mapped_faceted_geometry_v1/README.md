# Mapped finite-facet parameters — core implemented, lifecycle NOT enabled

Full Goal3/general symmetry remains ACTIVE/incomplete. This checkpoint implements
parameter geometry and its complete finite-facet cover audit, not a new remeshing
mode or accepted aerodynamic mesh. No option activates it in production.

[Sealed evidence](validation_summary.json).
[Full connected implementation still required](../../../Handbook/GOAL3_MAPPED_PARAMETERS.md).

## Why parameter state is needed

`representation_replay.py/json` reuses the saved oblique edge1--9: no interior
binary64 literal point, but its rounded exact midpoint has L1 error2^-54, below an
explicit1e-13 mapping contract. This does NOT authorize relaxing the old strict case.
Two additional exact examples constrain the representation:

- Separately rounded1-t/t fails exact chart-edge membership for t=.1 and.3.
- Different latent parameters can map to the same non-endpoint solver coordinate.
  A local positive-parameter/positive-actual-tet witness becomes parameter-degenerate
  under plane-foot recovery. This is a local representation certificate, not global
  embedding/quality/full metric or an accepted mesh.

The user-authorized abstract Opus5.5/high review completed219.63s with tools/MCP off
and no repository/case data supplied. Its raw argument is not correctness evidence.
The exact replay corrects its displacement example: use normal offset epsilon/2;
using epsilon plus a nonzero tangential error exceeded its own claimed bound.
A deliberately canonical inverse policy is a different, more restrictive authority;
it cannot silently replace persisted forward parameters at restart.

## Implemented core

`ParametricPoint` represents a+u(b-a)+v(c-a) over the exact real binary64 inputs.
Existing bounded integer arithmetic gives adjacent binary64 coordinate bounds and
unrounded projected orientation signs. No new numerical dependency. Coordinate
products fit132 32-bit limbs and planar determinants264; bounds refuse nonfinite
realizations. Existing AffinePointBounds delegates to this same exact model.
Robust tetrahedron predicates, geometry/height/metric/transfer gates are unchanged.

`SurfaceBinding` carries original support/version and scalar parameters: vertex,
canonical original edge, or strict original facet interior. Dependent weights are
never rounded or refit from solver coordinates. Geometry weights are NOT original
sensor interpolation weights. Original corner/edge identities survive current-node
renumbering. Original IDs and complete declarations must still agree across ranks.

`MappedFacetReference` shares the immutable original reference, prepares finite
support incidence once, checks the minimum mapping allowance of every incident
facet (including zero for strict facets), and realizes one deterministic bracket
coordinate. It audits finite face orientation and vertexwise displacement. Convex
support gives a complete-face distance certificate; vertex proximity alone is not
used to admit an arbitrary finite region. Complete surface audit additionally
requires a positive oriented disk on EVERY original facet, complete directed
feature chains, unique original corners/edge parameters and a closed manifold.
Actual-volume positivity and actual embedding remain independent mandatory gates.

Storage/work admission precedes private index/audit allocation. The first version
copied the declaration map before constructor admission; inspection caught this.
Current const-reference input is copied only after the bound; core_v2 includes the
refusal/interior-parameter checks. Failed audits leave their output report unchanged.
ResidentBytes excludes the shared original reference, which callers count separately.

Limit: all original triangle edges remain protected. This is not general discrete
surface coarsening or smooth/CAD geometry, and may obstruct construction. Do not
claim general readiness or add local fallback ladders if that restriction blocks
an admitted full target; distinguish physical features from triangulation seams
within an explicit geometry accuracy contract.

## Checks and cost scope

| Evidence | Result | Scope |
|---|---|---|
| core_v1_n1 | 8cases/2861assertions | Existing affine/source helpers plus new parameter/finite-surface controls |
| core_v2_n1 | 1case/26assertions | Current constructor admission, valid interior parameters, full cover/refusal/renumbering |
| oracle_v1 |40samples,120adjacent bounds,120exact signs | Independent rational reference, scales1e-300..1e300,60exact zeros |
| coordinate_compatibility_v1_n1 |3cases/287assertions | Unchanged cancellation/subnormal/segment/saved-M6-midpoint controls |

Only three changed units were compiled per build. Existing pinned support archives,
unit main, and unchanged affected coordinate-test objects were reused; the integrated
executable was NOT replaced. Source capsules, commands, archive/object/source pins,
raw logs and actual executable hashes remain. The final header annotation only adds
the documented shortcut comment; seal.py proves that exact text-only change.
Arithmetic evidence is reused only for its byte-identical defining sources.

Named compile/link/control phases total about39.1s then25.6s, with controls.524/.400s.
These omit separately timed archive pinning/preparation; no full-event speedup or
peak-RSS claim. One nice19 compiler/threads1, compiler scratch on4TB; observed host
available memory40--41GiB. No new CFD advance, MPI campaign, profile/normalization
trial, full suite, old obstructed construction retry or cluster work.

## Still unimplemented

Boundary-only accepted binding storage; private split/move/reconnect propagation;
collective atomic publication and N/M return; finite plane/line consumption of
unrounded parameters; simultaneous body/symmetry authority checks; actual original
source-envelope admission; native_ref persistence with correct SU2/CGNS output-node
remapping and real restart. Multiple holes/bodies,2D runtime, mixed curved junctions,
complete oblique/exterior accepted meshes, physical accuracy and affordability remain
OPEN. No per-volume-node fields, configuration or production activation were added.
