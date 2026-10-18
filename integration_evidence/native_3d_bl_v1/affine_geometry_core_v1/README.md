# General affine geometry core — not runtime support

Full Goal3 ACTIVE. This connected geometry-only implementation introduces a shared
immutable versioned chart and finite interval/plane references. Euler/RANS are
absent from the geometry interface. Cartesian initial runtime behavior from
10c81365d0 remains separately qualified; do not promote this core to CFD support.

Chart coordinates retain selected global binary64 coordinates, dropping the
largest normal component. Strict-arithmetic lifting audits an outward physical
point-to-plane/line bound. No exact global coplanarity or global tolerance increase.
Finite line components retain body-footprint gaps; complete plane segment/face
queries reuse the existing exact finite planar kernel, including nonconvex borders,
multiple holes and disconnected components. Feature queries return original IDs.
Topology/corner preparation is retained by an immutable authority, not rebuilt for
every chord. Requested memory is admitted before geometry preparation.

Remaining REQUIRED capability: atomic collective arbitrary-geometry initial
adoption, simultaneous body/symmetry intersections preserving body accuracy,
original-P1 donor coverage across rounded surfaces, declaration agreement, version
persistence/SU2/CGNS/real restart, full Euler/RANS BL adaptation/transfer/N=M/M<N,
bounded collective work and representative exterior-body acceptance. No automatic
retrofitting of existing states. Query component search is currently linear; cold
preparation and query cost must be measured before broad runtime adoption.

Annular construction remains separately blocked despite an independently feasible
same-field mesh. Do not infer a constructor fix from a valid affine reference.
See ../planar_symmetry_reference_v1 and ../../../Handbook/GOAL3_GENERAL_SYMMETRY.md.

run.py executes focused serial primitive/reference controls and audits saved
lifting residuals using independent exact rational arithmetic. It does not run CFD.

## Executed checkpoint — 2026-10-18

- `serial_v1`:9 cases/3731 assertions PASS. Five translated/oblique charts exercise
  both dominant coordinates in2D and all three in3D; finite interval gaps, two
  holes, a nonconvex notch, disconnected regions, tangencies, full chord/face
  containment, original feature IDs, conflicting shared coordinates, malformed
  topology and storage rejection. Relabeling BC markers and reversing all facet
  orientations leave the finite geometry unchanged. Already-authorized projection
  retains the original coordinates.
- Independent exact-rational lifting audit:320 samples PASS,319 nonzero exact
  line/plane equations. Maximum sampled physical distance2.042804e-16; maximum
  reported bound2.229226e-16, under the declared1e-13 contracts. These are sampled
  audits of the outward arithmetic enclosure, not a proof of every geometry/scale.
  Positive subnormal error contracts are explicitly unsupported; no tolerance is
  enlarged. Original robust volume predicates remain unchanged.
- Current-binary compatibility: `../planar_symmetry_reference_v1/baseline_v5_n4`
  passes15 cases/rank (1149--1214 assertions); `workflow_v5_n4` passes N4/M2
  initialized RANS/SA, current/BDF transfer, SU2/CGNS, reference and real restart.
  Its independent audit passes both budgets20 and3. Height/sensor/restart residuals
  are0; conservative flow/BDF defects<=4.15e-16. This is Cartesian runtime evidence,
  not arbitrary-orientation adoption or interacting body-authority qualification.

The focused serial invocation took1.24s and compatibility lifecycle33.32s on a
contended host. These are bounded correctness receipts, not performance comparisons
or whole-event affordability. Build used one worker; no CFD advance or new M6 run.
The corner lookup is reused per immutable reference and included in resident bytes;
component queries remain linear and finite graph preflight has quadratic worst case.
Analytic body-intersection curves remain unsupported by the straight-edge core.

Next connected work must preserve the full requirements: runtime adoption,
simultaneous junctions/body accuracy, original-donor coverage, collective identity/
version publication and persisted restart. In parallel design terms, saved annular
construction requires coordinated surface/first-row/volume transition reconstruction;
another edge-local fallback or a new chart alone cannot demonstrate that capability.
