# Native 3D exact coarsening prefilter — 2026-10-11

Repository `/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated`, branch
`codex/native-3d-core`, predecessor `f05ca3a8828d56619bdcafc763cbe4c8facae1d0`.
Implementation/evidence publication pending. Full Goal 1 remains active.

## Change

A directed collapse cannot preserve an incident immutable boundary facet if its
retained endpoint lies outside that finite facet. Selection now caches this
necessary-condition counterexample from owned cells containing both endpoints,
using the existing exact PlanarReference::Contains. Per-cell direction bits and
per-edge reference counts update incrementally with added/removed cells; their
storage is included in existing map byte admission. Missing remote-only witnesses
can leave impossible candidates unfiltered; they cannot falsely reject a valid
collapse. Full dependency import, geometric reconstruction, metric validation and
atomic publication remain authoritative for every admitted candidate.

The filter activates collectively only after the entire frozen field is
admissible. Rejected collapses on an invalid field can still trigger useful
movement/reconnection, so these retain the existing queues/fallbacks. One
activation election/reduction occurs per coarsening call; field metrics are not
changed. Default512 and q>=0.20/J>=0.05/L<=1.8 remain unchanged. No 2D/MMG,
sensor interpolation, geometric BL, fade, finer-demand or Hessian-noise change.

Exported counters count actual facet predicates and queued directions removed.
The latter does not count every later forbidden enqueue suppressed. The geometry
timer is nested in selection-cache seed/update and is not a metric query timer.
A successful collective vote also now retains local geometric rejection text;
elected failures still set the common failure, and successful completion clears
all rejection text. The reviewed failure return leaves its timer scope before
computing final accounting.

## Qualification

The latest complete configured68-object test driver passes MPI1/2/4 exact
corner filtering, removable coplanar seams, insertion, automatic repair,
strict completion/budget/default rejection, native2D and MMG tests. A corner
control removes twelve impossible queued directions using24 exact predicates,
without reconstruction or changing the mesh. An invalid-field control confirms
no pruning and a nonblank immutable-facet movement rejection. Existing restoreC
checks retain successful rejected-collapse repair. Initial v1 passes too;
v2 adds rejection accounting/reason checks and is the final tested source.

`coarsen_filter_auto_audit_v1` independently validates324 automatic repair/refine/
coarsen frames across unit/thin1e-8/sheared fixtures and nine missing/tampered
source/final-metric controls. It audits retained records without adaptation reruns.
Four actual manufactured lifecycle cases pass three events each: worker return,
ReplaceMesh, current/BDF2 transfer, original-P1 tensors, immutable finite geometry,
markers/embedding/positive volume and frozen metric residuals, plus actual SU2/
CGNS output/readback. No CFD iterations were run.

| Folder | Accepted tetrahedra at three events | Coarsening rounds at three events |
|---|---|---|
| coarsen_filter_aligned_n3_n1_v1 | 288 / 50 / 312 | 124 / 482 / 243 |
| coarsen_filter_driver_n1_v1 | 88 / 36 / 94 | 40 / 147 / 69 |
| coarsen_filter_aligned_n3_n4_m2_v1 | 288 / 47 / 277 | 82 / 294 / 160 |
| coarsen_filter_aligned_n3_n4_m4_v1 | 288 / 51 / 264 | 92 / 278 / 113 |

All four use the production factory default512. MPI trajectories differ and are
independently audited. Across these cases qmin0.2029350071, Jmin0.06217637445,
Lmax1.799641466, state relative error<=5.80681e-16 and transported sensor tensor
error zero. The manifest verifies stable source/object/archive identities,
artifact hashes and340 closed exclusive/nested rows in coarsen_filter_rank_phases.csv.

## Serial budget failure resolved; timing limits

The previous serial default n3 accepted its first event then rejected second
coarsening at512 rounds. Its diagnostic1024 version completed at619 rounds,
66 accepted/553 rejected. The updated default completes the same target at482
rounds,66 accepted/416 rejected:137 fewer rounds (22.1%). All seven input,
first accepted mesh/state/sidecar and second-input files are byte-identical to
the failed baseline. New final sizes288/50/312 match the diagnostic counts.
All historical failures and diagnostic records remain retained.

At that second coarsening the filter makes2933 exact predicates, prunes443 queued
directions and spends0.03144 s in nested geometry tests. Field requests2717 and
hits10 are unchanged. The old/new engine pilot walls4.18517/4.10262 s are single
contended measurements, not a repeat-qualified speedup. Nested edge tracing
2.14006 s and donor filtering1.41236 s remain large; their time overlaps exclusive
phases and must not be added to those totals. Eliminating cheap rejected proposals
resolves the round limit but does not remove the dominant source/metric work.

Per-event parent/remesh maxima, accepted-work rates, max/mean and requested native
bounds/lifetime RSS are retained in the manifest. N1 parents are
1.84433/4.94916/1.79497 s with useful rates53.68/13.34/71.31 edits/s;
N4M2 parents1.87919/4.35040/2.14365 s; N4M4 parents2.62990/7.11259/3.28839 s.
These include manufactured metric preparation/observation/replacement but exclude
separately recorded requested output and independent audits. Different partition
trajectories prevent fixed-work scaling comparisons. Maximum native requested
bound8MiB and maximum lifetime RSS32,444,416B are distinct measurements.

Accepted grids are in coarsen_filter_aligned_n3_n1_v1 and the N4M2/N4M4 folders:
envelope_s0.vtu through envelope_s2.vtu, actual SU2/event immutable sidecars and
final CGNS. Exact source/failure evidence is preserved; no executables/objects
are committed. Foreign work/AdapNoExt/submodules/ClusterResults are untouched.

Next target: measured exact donor filtering/edge tracing, followed by stronger/
direction-changing/thin/wedge/restart/larger cases and complete MPI wait/traffic/
memory calibration. Instrumentation overhead, repeated/scaling cluster evidence
and closed SGE packages remain open. No curved/robust BL/full-unsteady3D or CFD
affordability claim is made. No overnight cluster launch is requested.
