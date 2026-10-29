# Native adaptation checkpoint — 2026-10-29 user-requested stop

The user requested saving the current work and stopping before a context reset.
Their updated HANDOFF_Codex.md is preserved unchanged. No owned build, validation
or agent job remains active. DC-17/DC-18 and the full BL/RANS goal remain open.

## Saved implementation

- Classified boundary shelling repairs the captured sliver over its complete
  incident-edge ring. It reuses original-geometry/feature validation and the
  existing atomic cell/physical-face publication path; incomplete closure rejects.
- Optimizer geometry overlays refresh after boundary changes and movement.
- Geometry decisions use existing original-surface face error. Removed the custom
  curvature sampler that reported .0604383 where validated-face error is
  .00726343, below the requested .01. Whole-surface coverage remains a separate gate.
- Quality smoothing selects geometry-feasible candidates independent of the global
  ValidityOnly builder mode. Ordinary repairs cannot worsen area-weighted geometry
  excess. Requested-height GeometryRepair retains its explicit route. This merit
  rule is an implementation decision, not an extra user requirement.
- Corrected regression assumptions: Absolute admission rejects poor shape before
  progress checks; an empty directory snapshot has a header; byte-identity fixtures
  now initialize their current nodal sensor metadata before saving the baseline.
- The diagnostic workflow preserves its exact original metric-donor vector.
  No work/time limits, geometry tolerance, final q/J/height gates or restart format
  were raised or weakened. No new projector or format-specific CAD infrastructure.

## Validation and limits

| Evidence | Result |
| --- | --- |
| Build124, latest source/test/helper pins | PASS186.280s,1295 pins unchanged |
| Latest serial regression | PASS45 cases/9611 assertions,9.906s |
| Latest six-rank allworker regression | 11/12 cases pass per rank,79.305s; only two curved COMPLETE assertions fail |
| Captured boundary kernel/optimizer replay118 | PASS; incomplete closure refused; actual optimizer removes the sliver |
| Compiled filter audit118 | All28 selections pass; new boundary regression present in broad compiled selection |
| Latest smaller workflow124 | FAIL43.985s, unchanged4096 logical work;256 private cells,q/J2/2,size22,geometry0 |
| Full M6 build114 | FAIL1860.786s at1800s native limit; quality entered497s after first length pair; no projection-failure captures |

The latest smaller run does not exhaust arithmetic work:38,382,505 of67,207,168.
Geometry stays satisfied, but quality and size remain incomplete. Flat Euler/RANS
current+BDF2 conservative transfer and SU2/CGNS reload pass within that test;
curved transfer is not reached. Actual solver restart remains unvalidated. M6 has
not been rerun against the later geometry/quality corrections.

## Continuation diagnostics — do not overinterpret

1. Reconstructing metric donors from the current unaccepted mesh preserves its
   uniform diag4 values where supported, but changes bootstrap/source support.
   That diagnostic reaches one q/J-invalid cell and no size/geometry failures,
   then times out inside repair. It is not the original complete workflow.
2. Replaying with the six captured original metric donors fails bootstrap before
   any proposal: `No original donor contains a requested point`, work0/atlas0.
   Reentering AdaptMetric starts a fresh metric bootstrap, not a resumed in-event
   field context. Neither this failure nor the timeout establishes geometric
   impossibility or justifies raising a production budget.
3. CavityMetricSource::Build intentionally refreshes prior metrics; existing tests
   require replacing old values and sampling changed cells. Do not bypass that
   behavior merely because sensor_present is true. Exact in-event continuation
   and a fresh metric epoch must be distinguished before changing this path.

The captured donor records are metric support, not conservative CFD solution or
history records. No CFD transfer/restart claim follows from these diagnostics.

## Evidence locations

All paths below are under
integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/.

- checkpoint124_stop_summary.json: latest results, hashes and exact case paths.
- cases/mapped_work_trace_20261029T132820_1197260/: complete rejection state,
  immutable authority, parameters, owned rows and original metric donors.
- cases/serial_20261029T134217_1231105/: latest passing serial regression.
- cases/allworker6_20261029T134356_1235074/: latest MPI regression, only workflow failures.
- serial_precheck_value120/: false-veto numerical evidence; sandbox attempt and
  successful standalone run are separate logs.
- serial_continuation123_build122_mpi/: reconstructed-metric-support timeout.
- serial_continuation124_mapped_work_trace_20261029T132820_1197260/: original-support
  bootstrap refusal, before any operator.
- build114_m6_summary.json and m6_runs/euler_build114_classified_stages/: full M6 result.

Raw evidence remains in the workspace. Compact receipts, relevant rejection inputs
and replay sources are checkpointed; compiled binaries/objects are not committed.
Continue only from the user's new HANDOFF and current instructions.
