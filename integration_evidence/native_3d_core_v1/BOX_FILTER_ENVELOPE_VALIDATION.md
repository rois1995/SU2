# Current donor-prefilter anisotropic envelope — 2026-10-12

Repository: /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated.
Branch: codex/native-3d-transport-accounting. Production remains at the qualified
8c3b40c9be1cca3919d627c7b3851b65bc05312a implementation; the starting branch tip
was 47480bbd415b240ee16c9b71bea138ff6332e2a0. No C++/config/target/guard change.
Goal 1 remains active and Goal 2 is not started.

BOX_FILTER_ENVELOPE_MATRIX.json predeclared three cases before execution. Each
uses the actual box_filter_build_v2 configured 68-object/774-member binary,
SHA256 c1bdef2a09d1e13bd1d137910db5713de32514f7a421369d6b1a177b48de54d6.
All complete build source/object/archive/tool pins remain unchanged. Actual MPI
and all independent auditors ran sequentially, one CPU, nice19, one numerical
thread, after each runner's actual host/process/memory checks. Four unrelated
FSI ranks were active; no local performance campaign or cluster submission.

Each size 4 wedge starts with 192 tetrahedra and spatially turning metric directions
and strength variation 0.125. The aspect2 cases use reference aspect ratio 2;
the H20 thin case uses reference aspect ratio 1.5 and physical height 2^-20 with
covariantly transformed metric tensors. Per-event targets remain original nodal
P1, not analytic sensor reevaluation at new points. The production 512 transaction
budget, q>=0.20, J>=0.05, L<=1.8 and Hessian noise 0 remain unchanged. All three
actual refine/coarsen/refine driver replacements pass. Planar surfaces adapt,
including marker junction preservation and accepted return to N CFD ranks.

| Case folder | N/M | Accepted tetrahedra | Minimum q | Minimum J | Maximum L | Maximum current/BDF residual |
|---|---:|---|---:|---:|---:|---:|
| box_filter_a2_direction_n4_n4_m2_v1 | 4/2 | 402/71/435 | 0.203997406 | 0.058952194 | 1.795651615 | 3.71563e-16 |
| box_filter_a2_direction_n4_n1_m1_v1 | 1/1 | 402/54/424 | 0.360574696 | 0.165487609 | 1.796383465 | 3.77297e-16 |
| box_filter_thin_h20_n4_n4_m2_v1 | 4/2 | 289/48/314 | 0.238838158 | 0.102718740 | 1.799674941 | 4.60667e-14 |

Each case has a fresh full independent audit of eight mesh/input/readback frames,
three state frames, exact positivity/embedding/no-overlap/incidence/volume/domain/
finite facets/markers, independently integrated original-P1 quality/sliver/edge
metrics, transported nodal tensors, actual SU2/CGNS write/readback and closed
parent/rank accounting. All transported tensor residuals are zero, including
physical and normalized thin units. Domain volume is exactly 1/2 for the aspect2
cases and 1/2097152 for H20. The MPI aspect2 q minimum is close to the guard;
this is a bounded tested envelope, not a margin guarantee for stronger metrics.

check_box_filter_envelope.py verifies all predeclared parameter profiles, unique
case coverage, binary/build/receipt/artifact pins, complete fresh audits and
strict gates. Each case's eight geometry/donor files exactly match its older
query-routing control (24 matches). All nine current/BDF state files also match
those controls byte for byte. This comparison supplements the fresh audits; no
historical certificate is substituted for them. Serial and MPI meshes differ,
and no cross-partition byte identity is required or established.

The eight H20 geometry/donor frames additionally match the current unscaled
box_filter_n4_n4_m2_v1 frames after exact z/tensor normalization. Physical affine
states are validated in physical coordinates rather than equated across heights.
This establishes thin-domain covariance, not actual geometric BL construction.

The box prefilter is exercised in all three cases: summed rank-work counts
165180/22482/62070 exact donor tests avoided, respectively. Every actual row
satisfies donor_box_filtered<=donor_filtered<=donor_candidates. These are work
counts, not elapsed-time or speedup measures. Parent phases and detailed counters
are retained in case.stdout, and each case receipt retains diagnostic stage
clocks. No repeated/scaling/detail-overhead/wait/allocator qualification follows.

Recheck the complete matrix:

```sh
PYTHONDONTWRITEBYTECODE=1 python3 integration_evidence/native_3d_core_v1/check_box_filter_envelope.py
```

BOX_FILTER_ENVELOPE_VALIDATION.json pins the matrix/checker/build and each
complete case receipt/audit. Each folder retains source boundary/runner/host
provenance, original inputs, three accepted SU2 meshes/immutable-reference
sidecars, original/current/BDF states, SU2/CGNS readbacks, logs and three inspectable
envelope_s0/s1/s2.vtu grids. Total case data is about 7.1 MB; no rebuild, duplicate
binary, bulk CFD series or per-candidate dump was created. All owned jobs are
terminal. Foreign ClusterResults, AdapNoExt and submodules remain untouched.

Size8 production 512 completion remains limited, with separate 1024 diagnostic
feasibility already certified. This matrix does not fix that limit. Repeated
cluster total-cost, instrumentation overhead, rank/weight/wait/traffic/memory and
practical scaling qualification remain required before Goal 1 completion. No
curved geometry, production 3D BL, actual full-unsteady CFD or affordability claim.
