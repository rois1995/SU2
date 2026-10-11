# Full local-star facet filtering

A directed collapse must preserve every incident reference facet of its removed
vertex. The edge-cell cache did not see facets exclusively in other vertex-star
cells. Private reconstruction correctly rejected these proposals, wasting bounded
attempts. The new `LeavesFacet` helper is shared by cached checks and a bounded
scan of locally owned surviving star cells before exact orientation filtering.
No new index or collective is needed. A missing local witness still reaches full
private distributed validation. Invalid fields keep the existing repair path.
Frozen original-P1/BL composition, geometry, q/J/length and512-round policies stay
unchanged; source changes are limited to3D selection and its regression fixture.

Repository: /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated.
Branch: codex/native-3d-transport-accounting; based on8807b27d8d.
Build/source/object/archive pins: full_star_facet_build_v1/validation.json.
Actual complete68-object/774-member driver SHA256:
d99ccd8e4e4d0477b8badc1e3453a6ac063f579b0d65f1a92950420820dde0c5.

| Case folder under this report | Actual result |
|---|---|
| full_star_facet_build_v1 | PASS full configured MPI/MMG/CGNS driver |
| full_star_facet_mpi1_v1 | PASS72cases;12independent frames/6state pairs/6SU2CGNS round trips; serialMMG48assertions/2cases |
| full_star_facet_mpi2_v1 | PASS72cases/rank;24frames/12state pairs/12round trips |
| full_star_facet_mpi4_v1 | PASS72cases/rank;48frames/24state pairs/24round trips |
| full_star_facet_lifecycle_mpi1_v1 | PASS six actual metric/Hessian OFF/ON controls and CFD/BDF replacement |
| full_star_facet_n4_n4_m2_v1 | PASS three rotated/spatial wedge events, N4M2, strict independent audit |
| full_star_facet_restart_su2_n2_n1_m1_v1 | PASS SU2 persisted reference/current+BDF restart and later adaptation |
| full_star_facet_restart_cgns_n2_n4_m2_v1 | PASS CGNS persisted restart and later adaptation, N4M2 |
| full_star_facet_package_checks_v1 | PASS31lightweight tests and complete actual work/notification/nested-cost audit; original collector-only failure retained |
| full_star_facet_n8_n1_m1_v1 | FAIL full campaign at second coarsening; scoped first-event equivalence certificate PASS |

The new corner test first proves the actual complete-star private operator rejects
a collapse forbidden by a facet outside the candidate edge cells. It then proves
an admissible coarsening event preserves all six tetrahedra, markers and input
bytes. Serial selection makes zero private attempts. MPI owners missing a local
witness may still require complete private import. This is conservative filtering,
not permission to bypass final validation or require fixed boundary connectivity.

All five frozen n8 baseline files are byte-identical. First event is unchanged:
428collapse attempts/272accepted/156rejected,2489tets/590points. Six actual
mesh/state/reference/next-input artifacts match the prior independently certified
first event byte for byte. audit_first_cycle_equivalence.py validates its complete
saved certificate/input/tool chain and reruns the correct sensor, transported
metric/current+BDF and actualSU2/accounting checks. Tensor error0;state relative
residual3.899327321487061e-16. Full expensive geometry checks are reused, not claimed
as freshly rerun. Actual accepted envelope_mesh_00001.su2 and envelope_s0.vtu are
retained. No complete-case independent_audit.json is manufactured from cycle0.

Second event still exhausts512attempts:437accepted/75rejected,517valid cached
private cells,266pending directions,0repairs. Previous implementation had
429accepted/83rejected,545cells/323pending. The last rejection concerns edge
180,181 and no removal/oversized edge; it does not classify all75failures. The
accepted first CFD mesh/current+BDF/reference remains intact. No independently
certified or exported current private cycle1 mesh yet. Progress is local work,
not full completion or speedup. Source-matched trace/classification is next.

Accounting validates42/123/363MPI1/2/4ENGINE rows and all actual lifecycle/n4/
restart/failed-n8 detailed engine rows: integral nonnegative geometry/orientation/
change counts, bounded nested costs and globally closed sent/received records.
Facet time belongs to seed, selection and update together, not selection alone.
The first collector failed when two nonroot Catch summaries interrupted final
MPI4 ENGINE records. Recovery removes only exact recognized summary blocks,
retains raw/recovered logs and removed spans, restores all numeric fragments and
has a parser self-check. Original failed source/stderr/execution remain intact.
Numerical tests and independent mesh audits did not fail.

Runs were sequential nice19, one thread, MPI<=4 on one CPU under observed
contention. No performance/scaling/overhead/affordability claim follows. Goal1
remains active: repeated cluster timing, full traffic/wait and allocator
calibration, practical limits and further larger-case diagnosis remain. The
reusable closed SGE package now expects72cases; rebuild and prepare a fresh
bundle after updating the pushed branch. No cluster job was launched tonight.

## Separate larger-budget feasibility result

The unchanged production512default still rejects n8 at its second event. A
separate existing1024-round fixture now passes all three events and its complete
independent audit. Accepted meshes2489/457/2245tets; second coarsening615attempts,
third coarsening1002attempts. See
full_star_facet_n8_n1_m1_rounds1024_v1/REVIEW.md. This establishes feasibility
with more work, not a production budget change or performance/scaling result.
