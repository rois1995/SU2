# Affine initial-reference adoption — connected batch, UNQUALIFIED

Full Goal3/general symmetry remain open. Prior checkpoint6f0bdfabce independently
passes complete Cartesian annular N1M1/N2M2/N4M2/N4M4 construction/transfer/restart.
This batch is building in owned session53426, one nice19/CPU1/j1 job. No new runtime
or accepted oblique mesh is claimed. Do not edit pinned C++ while it runs.

## Connected reach

One bounded decoder now serves initial adoption, immutable reference creation and
frozen/restart declaration checks. Version1 retains its Cartesian behavior. Version2
separates input-deviation and mapping-error ceilings and declares stable identity/
version/origin/normal. Multiple definitions may share one marker: each finite input
facet must match uniquely; missing, ambiguous and conflicting definitions reject.
Input-deviation audit uses the existing conservative affine residual bound, not an
exact Euclidean distance. Geometry backend remains independent of Euler/RANS.

Schema (maximum64entries/65536bytes):
```
SU2_NATIVE_SYMMETRY_PLANES 2 1
"symmetry" 17 1 0 0 0 -.37 1 -.61 1e-10 1e-13
```
Fields: marker, identity, version, originXYZ, normalXYZ, input ceiling, mapping
ceiling. Input ceiling must be at least the positive mapping ceiling. Identities/
versions use unsigned decimal syntax. Existing v1 syntax stays valid and exact.

Initial projection precedes CFD geometric quantities, states and original donors.
Matching points stay unchanged, including signed-zero representations. Restart/
returned geometry audits the mapping contract and never projects frozen coordinates.
The strict shared projector checks all supplied independent affine constraints;
dependent/ill-conditioned declarations refuse instead of merging close planes.
At most3independent3D or2independent2D constraints; the2D geometry primitive is not
runtime2D declaration support. Existing distinct-plane/analytic source collars are
still unsupported and reject; projection alone does not make that workflow valid.

For v2, a conservative strict-arithmetic L1 nodal displacement bound protects all
incident non-symmetry faceted boundaries against ADAP_HAUSD. With unchanged triangle
connectivity the same bound controls the paired piecewise-linear body surfaces.
Changing coordinates beneath a persisted native reference still refuses. Analytic
body junctions/curves require their own simultaneous authority; no body substitute
or general CAD support is claimed.

## Initial embedding and resource contract

Every touched tetrahedron preserves its exact orientation sign. V2 changes also
check the closed physical-boundary topology and boundary-adjacent cell contacts.
Only uniquely owned boundary-adjacent tetrahedra are exchanged; their count is at
most the physical-face count. Original halo coordinates must agree. The house local
box index filters pairs, exact ConformingDisjoint remains authoritative, and ranks
share the checks. No whole-volume gather or per-query geometry negotiation is added.

For an accepted embedded manifold source with unchanged connectivity, consistent
relative orientation and a bijective boundary map are sufficient for a bijective
simplicial map. We use that criterion; boundary-cell conformity is a stronger
check than boundary-face injectivity. This is the application of
[Lipman, Theorem1](https://arxiv.org/pdf/1310.0955), not a proof that arbitrary
invalid input meshes can be repaired. Disconnected components are checked together
for contact. The result still needs the planned independent geometry audits.

Memory is admitted before transport/index construction using house byte models;
CFD/allocator/MPI process memory remains separate. Pair work is capped globally at
1000000; ADAP_NATIVE_MAX_TIME is checked between index queries and every128exact
pairs. House construction/collectives are not preemptible. Exhaustion rejects
before coordinates publish. Cold witnesses/pairs/time and mapping/body/volume
bounds are in the audit; matching return/restart avoids rebuilding this audit.

## Validation batch, pending

`run.py controls 1 v1`: shared affine primitives, actual Euler and RANS initial
adoption, one/two disconnected planar components under the SAME marker, original
P1/affine coverage/codec/state preservation, and a narrow-gap counterexample with
positive individual tetrahedra but colliding proposed boundary. Rejection must keep
all CFD coordinates/states unchanged; `_rejected.su2` and diagnostics are retained.

`run.py annulus 1 v1`: difficult sheared one-hole RANS annulus through the same full
construction/full-field/height/transfer/SU2+CGNS/real restart harness. Freestream is
tangent to its declared symmetry plane. Requires a complete accepted result; a
valid initial reference is not construction feasibility. Stale configuration
versions must reject collectively without changing original donors/reference.

Only after those resolve: affected MPI controls and complete lifecycle N=M/M<N,
plus current Cartesian compatibility. No broad suite per small edit or CFD advance.
Exact source capsules, binary/build/config/input/helper pins and raw failures remain
required. See build_plan.log/pre_build_source_v1.json.gz/build_pending_snapshot.json;
active resource/log files are not sealed until the process is terminal.

Remaining full scope: accepted arbitrary oblique/exterior workflows, shared curved
body/plane source coverage, general junctions/2D/multiple bodies/holes, robust richer
front construction, global uncertainty calibration/composed gradation and unsteady
accepted-cycle affordability. This batch does not narrow those requirements.
