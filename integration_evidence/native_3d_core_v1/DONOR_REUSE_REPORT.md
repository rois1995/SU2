# Native 3D donor reuse and segment filtering — 2026-10-11

Repository `/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated`, branch
`codex/native-3d-core`, predecessor `89225cf0cbc9215fd92080638274c602c5bd9005`.
Implementation/evidence publication pending. Full Goal 1 remains active.

## Change and invariants

Original donor import returns the union for each caller. Once an immutable donor
has been admitted into that union, later overlapping requests now reuse that
admission rather than rerunning tetrahedral or metric-footprint tests. The existing
bounded selected map is reused within this import; there is no persistent cache,
extra heap structure, source-epoch invalidation or changed payload order. Every
request is validated before routing; invalid duplicate tetrahedra still reject
atomically. First admission still uses the original exact geometric filters and
the 256-donor cap. New `donor_reused` counts the skipped second tests.

Segment tracing now skips a donor only when both endpoints have certified strictly
negative signs against one of its face planes. It first validates finite positive
donor orientation, so an invalid outside donor cannot be hidden by rejection.
The existing determinant error bound certifies the floating signs. Unresolved
signs return zero to this rejection filter and defer to the original exact clip.
Touching, crossing, coplanar and uncertain intervals retain rational clipping,
exact root ordering, P1 endpoint weights, coverage/overlap and conformal-tie checks.
All other determinant callers retain their exact fallback. New `edge_separated`
counts skipped rational clips. This adds fixed tetrahedral scratch only.

Targets, geometry, original sensor P1, actual-point geometric BL composition,
finer demands, fade, Hessian noise, operator ordering and production512 guards
q>=0.20/J>=0.05/L<=1.8 are unchanged. No 2D or MMG production changes.

## Qualification

`DONOR_REUSE_MATRIX.json` records the frozen cases, guards and diagnostic addenda.
`DONOR_REUSE_CHECKPOINT_VALIDATION.json` binds source/object/archive identities,
artifact hashes, 340 exclusive/nested accounting rows and independent audits.
Four actual manufactured driver cases pass three events each, including worker
return, ReplaceMesh, current/BDF2 transfer, original P1, finite-facet geometry,
markers, embedding, positive volume, residuals and SU2/CGNS write/readback.
All 44 mesh/source/state/readback files are byte-identical to the corresponding
coarsening-filter predecessors at each fixed N/M partition.

| Case folder | N/M | Accepted tetrahedra | Reused donor tests, all events | Separated segment donors, all events |
|---|---|---|---:|---:|
| donor_reuse_aligned_n3_n1_v2 | 1/1 | 288 / 50 / 312 | 299883 | 79757 |
| donor_reuse_driver_n1_v1 | 1/1 | 88 / 36 / 94 | 62271 | 16865 |
| donor_reuse_aligned_n3_n4_m2_v1 | 4/2 | 288 / 47 / 277 | 290789 | 97673 |
| donor_reuse_aligned_n3_n4_m4_v1 | 4/4 | 288 / 51 / 264 | 290337 | 109653 |

Across these cases qmin=0.2029350071, Jmin=0.06217637445, Lmax=1.799641466,
transported tensor error=0 and state relative error<=5.80681e-16. Different
partitions have different trajectories and are audited separately.

Final configured `donor_reuse_regression_v4` uses all68 actual test objects and
production archives. MPI1/2/4 each passes62 selected native/2D bridge tests,
including all NativeMesh3D arithmetic, tensor/P1, exact segment roots, touching,
gaps/overlaps, thin/cancellation, source reuse, invalid duplicate, filter,
insertion, recovery, budget/default and atomic rejection controls. The new donor
control has257 original donors, single/duplicate volume/metric imports, identical
payload/P1 and resident storage, and one original filter rather than two.
Serial MMG 2D/3D round trips pass separately (48 assertions, two tests).
`donor_reuse_auto_audit_v1` independently validates324 retained automatic frames
and nine missing/tampered source/final-metric controls. No adaptation reruns are
needed for that audit.

`donor_reuse_replay_original_n3_v1` preserves the known infeasible marker-junction
case:175 private rounds,57 accepted edits (42 cavity insertions),118 rejections,
420 private tetrahedra. It remains rejected with accepted CFD state preserved;
four initial/rejected JSON/SU2 files are byte-identical to the insertion replay.
The guard is not weakened to turn this input into a pass.

## Performance evidence and limits

At serial alignedn3 second coarsening, operation counts remain482 rounds,
66 accepted/416 rejected and2717 field requests/10 cache hits. Donor reuse skips
125165 tests and the segment filter skips21750 rational clips. Single contended
old/new engine walls are4.10262/2.61079 s; nested donor-filter walls1.41236/0.31636 s,
nested edge-trace walls2.14006/1.76457 s. This is fixed-work diagnostic evidence,
not a repeat-qualified total adaptation speedup.

| Case | Parent maxima at three events, seconds | Accepted edits per parent second |
|---|---|---|
| alignedn3 N1/M1 | 1.57579 / 3.25626 / 1.23969 | 62.83 / 20.27 / 103.25 |
| originaln2 N1/M1 | 0.42835 / 0.54274 / 0.45179 | 74.71 / 33.17 / 108.46 |
| alignedn3 N4/M2 | 1.52336 / 2.95652 / 1.47686 | 64.99 / 23.34 / 94.80 |
| alignedn3 N4/M4 | 1.79092 / 6.08260 / 2.22059 | 55.28 / 10.85 / 60.79 |

Parent accounting includes manufactured metric preparation, observation and
replacement; requested mesh output and independent audits are separately timed.
Nested query/import details are not added to exclusive parents. Maxima are taken
per event, not summed across ranks or phases. These are single locally contended
pilots, with four unrelated FSI ranks running; they establish no scaling claim.
Maximum admitted native bound8MiB and lifetime RSS32382976B are distinct measures,
not calibrated full CFD/ParMETIS/allocator peaks.

`segment_prefilter_probe_v3` compares saved baseline/current predicate kernels on
1547 actual initial/final mesh chords in three sets. All returned donor IDs,
interval roots, widths and weights match exactly. Three alternating O2 samples
per set show median kernel reductions29.5%,21.9%,36.4%; one baseline sample in
set2 is an outlier. No >256-candidate chords were skipped for this dataset.
The saved microprobe excludes import, ADT, MPI, complete adaptation and CFD;
these percentages cannot establish operational affordability.

## Retained experiments and verification correction

`segment_prefilter_probe_v1` failed before compilation because a relative script
path was compared with an absolute root; its original runner and failure record
are retained. The initial full-exact-sign prefilter in probe_v2 preserved values
but duplicated exact work: coarsening-set median was about2% worse and had5987
extra exact orientations. The final certified-sign-only revision avoids that
work; v2 and historical actual N1_v1/regression_v1 evidence remain intact.
Latest actual linked runtime is N1_v2, not N1_v1.

Review discovered that legacy runners selected nonexistent `[MMGRoundTrip]`.
Their PASS receipts do not prove MMG execution; they reported an unmatched
selector. Corrected regression_v3 passed serially but failed at MPI2 because the
legacy direct MMG fixture calls SetMesh on every rank, while ExtractMesh gathers
the complete mesh only on rank0. This is not an MPI-native production failure.
Its stdout/stderr, stable production pins and failure receipt are retained.
Final v4 runs the actual registered MMG tests serially and native controls at
MPI1/2/4; unmatched selectors now fail the runner. No historical receipt was
rewritten and no MMG code or fixture was changed.

Inspectable accepted grids are `envelope_s0.vtu` through `envelope_s2.vtu` in the
four case folders above, with JSON source/state, SU2/reference sidecars and final
CGNS beside them. Rejected grid: `donor_reuse_replay_original_n3_v1/replay_rejected.su2`.
No executables/objects are published, and no CFD iterations were performed.

Next: expand stronger/direction-changing/thin/wedge/restart/larger correctness
cases and locate practical limits, complete MPI wait/traffic/memory calibration,
and prepare closed SGE repeated/OFFON/scaling campaigns. No overnight cluster
launch is requested. Full Goal1 is not complete; curved geometry, robust 3D BL,
full unsteady3D and CFD affordability are not claimed. AdapNoExt, foreign work,
submodules and ClusterResults remain untouched.
