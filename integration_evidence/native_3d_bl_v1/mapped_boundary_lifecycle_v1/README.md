# Mapped boundary lifecycle — connected implementation in progress

Full Goal3 and general symmetry scope remain ACTIVE/incomplete. This batch connects
boundary-only forward parameters to private edits, staged collective publication,
engine-stage isolation, original-source envelope preparation, N/M parent return,
the reader's existing dense IDs and v5 native_ref output/reload. Existing strict
formats and actual-query original sensor/BL policies remain unchanged.

The first connected prototype built successfully; current corrections are rebuilding. Focused controls are
`[NativeMappedPublication3D]` (saved unrepresentable edge, atomic refusal/publication,
parameter persistence/renumbering) and `[NativeMappedReader3D]` (oblique Euler/RANS
reader numbering and actual SU2/CGNS geometry reload). They do not prove accepted
BL construction, solution restart, exterior/multiple-body readiness or 2D support.
All original triangle seams remain protected in this first mapped representation.

The bounded build worker is reused from joint_affine_source_v1. It archives and
fully verifies the qualified old executable before replacing it, pins connected
sources, runs one nice19 worker on a sampled CPU, uses compile-tmp on4TB and records
15second descendant-RSS/host-memory samples. Compilation runs through official
Ninja dependencies: no old-layout support archive is reused for runtime linking.
Do not edit pinned C++ until the actual build handle is terminal. Build or runtime
failure is preserved before correcting sources or starting a new evidence version.

## Current qualification

`linked_v1`: first official dependency build PASS2487.78s; binary68f696577d50.
Publication_v1 passes38assertions. Reader_v1 aborts before reader execution because
its steady fixture used the BDF snapshot helper; stack/failure evidence retained.

`linked_v2`: guard/private-scope build PASS286.47s; binary
76ffde56417555462279bd12c30b7d75ed7a02c6801d53b1019c895852b16e5a.
- realized_v2_n1: PASS25assertions, .488s.
- publication_v2_n1: PASS38assertions, .442s.
- reader_v2_n1: PASS38assertions,3.911s, Euler/RANS actual dense IDs/SU2+CGNS reload.
- orientation_oracle_v2: independent exact-rational normal-dot209samples,
  107positive/88negative/14zero;171filtered/38exact. Python3.8 launcher correction
  retained; it failed after probe link, before any samples, and changed no C++.
- runtime_v2_n1: Euler completes6->58tets, conservative BDF2 transfer and both
  reloads. RANS constructs58tets but state comparison fails: MakeConfig declares
  no-slip walls while the new fixture prescribed nonzero momentum there. Failure
  preserved; corrected affine momentum vanishes at z=0, tolerance unchanged.

`linked_v3`: complete connected production/observer build PASS587.96s, binary
78b1aa43fbf58cd4111693d8ad9c62968dc0ac752bbaec12bfd6561dcd320d55.
- runtime_v3_n1: complete uniform Euler/RANS-SA PASS195assertions/15.85s,6->58tets,
  conservative BDF2 transfer and SU2/CGNS/reference geometry reload.
- rejection_v3_n1: matching borrowed private reference/accepted-state isolation,
  collective diagnostic failure and successful-case noninvocation PASS24assertions.
- audits/runtime_v3_n1_uniform.json:4independent output audits,270exact overlap pairs
  each, all12finite original-facet covers; max mapping1.665e-16,volume drift9.252e-18,
  minq.56508/J.25677,Lmax1.743674,state/history residual3.862e-13. Three corrupted
  mesh controls reject. Initial post-check helper report-path failure retained and
  fixed; only the offline audit repeated, no native/CFD rerun.

Final local qualification is sealed in `validation_summary.json` against current
binary bcb271be14f7cc1b9cc4fe5295fc8d5caf73d0dbf005a0177e8533ceddebf78d.
All jobs terminal. Native production/observer batch linked; test-only deadline and
lazy-reference ordering are corrected and separately archived in linked_v4/source
scopes. No full suite or new physical CFD advance.

- Uniform Euler/RANS-SA N1M1,N2M1,N4M2,N4M4 complete6->58tets/31nodes, conservative
  current/BDF2 transfer and SU2/CGNS/v5 geometry reload. Serial195assertions;
  affected N4geometry8cases/rank, reader/publication/private-rejection pass.
- Independent4output audits for each lifecycle pass finite ideal cover, mapping
  bounds, actual orientation, volume/overlap/topology, uniform metric and affine
  flow/history;3corrupted-mesh controls reject. Point coordinates agree across
  ranks; N4connectivity differs and passes. No exact topology determinism claim.
- Annulus_v5_n1 reaches complexity19.9996/budget20; remeshing rejects8192work after
  18private edits/96tets/69edge tasks/84target-invalid cells. No regional constructor
  attempted. PrismChart excludes extension components, mapped reference uses that
  path; radial alternative requires analytic scene. All raw seams stay protected.
  [Exact diagnosis and construction decision](CONSTRUCTION_STATUS.md). No accepted
  BL or full mapped real solution-restart/exterior/general2D qualification.
- Rejected mesh, v5 private reference, trial bindings and source are retained, with
  inspectable `_rejected.su2`.120s native/180s outer cap; no raised work/mesh gates.
- Opus5.5/medium source review completed with correct model, normal authentication,
  tools/hooks/MCP disabled. It corroborates the conditional facade-pointer/profile
  reuse issue; `authority_review_v2/assessment.json` records additional safeguards.
  Review is not validation and is separate from observed construction rejection.

Build times and contended rejected-case timings establish no unsteady affordability.
Original facet seams, affine BL walls/mixed analytic interfaces,2D runtime,
multiple-body/exterior/full solution restart and all original scope remain OPEN.

## Resumable focused checks (run after build PASS)

`run.py` reuses the qualified joint-source/replay provenance, bounded180second
execution and artifact checks, with an explicit local launcher copy for sampled
CPU affinity. Historical helpers remain byte-unchanged. It requires the completed
official build's exact binary/source pins before executing. Select a new `vN`
suffix for a changed source or a failed case; never overwrite rejected evidence.
One case at a time, nice19, threads1, MPI1/2/4 only. Announce its full directory
before running; do not run alongside an owned build.

```bash
python3 integration_evidence/native_3d_bl_v1/mapped_boundary_lifecycle_v1/run.py rejection 1 v3
python3 integration_evidence/native_3d_bl_v1/mapped_boundary_lifecycle_v1/run.py runtime 1 v3
```

The `affected` and `compatibility` modes are available for the connected milestone;
MPI runs follow initial serial diagnosis, not every small correction. These checks
still do not establish full oblique construction, flow-state restart or Goal3.

The `annulus 1 v3` mode selects the existing oblique symmetry/RANS/BL workflow
with an explicit test-only matching mapped authority before field freeze. It uses
unchanged production geometry, height, full-metric, quality and transfer gates.
A rejection must retain its matching private reference/parameters; accepted uniform
controls do not predict BL construction feasibility.
