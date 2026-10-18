# Exact shared-simplex contact validation

Full Goal3 ACTIVE/incomplete; `codex/native-3d-bl`, NativeIntegrated. This connected
change addresses the dominant measured geometry-validation cost. It preserves the
user's opt-in BL budget policy, original sensor-only P1/actual-query composition,
profile/fade and every existing mesh/solution acceptance gate.

## What changed and why it is exact

`ConformingDisjoint` still checks positive finite tetrahedra and consistent shared
coordinates first. It then uses coordinate comparisons to settle a strict bounding-
box gap, or a supporting-plane contact when one complete extremal simplex consists
of common mesh vertices. All possible intersection is in that plane; that common
extremal simplex is contained in the other, proving the permitted common contact.
Every unresolved pair retains the original exact SAT/rational clipping path.
No tolerance, approximate coplanarity, embedding shortcut, new cache, public ABI or
collective is introduced. This shared predicate reaches region construction and
local/remote publication validation; no testcase or solver-transition dispatch.

## Actual identical-field comparisons

| Scope | Previous native seconds | Current native seconds | Actual result |
|---|---:|---:|---|
|320-cell contact-only validation|44.874014|12.220260|All51040pairs pass; exact orientations3482000->853706|
|Closed-sphere BL construction|202.612478|103.576070|8->5440cells,5604work|
|Saved BL removal|57.383686|20.120511|5440->320cells,483work|
|Combined native mesh stages|259.996164|123.696581|6087/6144work, unchanged300-second total ceiling|

The observed reductions are72.77% for contact-only,48.88% for construction and
64.94% for removal; combined52.42%. These are contended single local comparisons,
not statistical/scaling/affordability claims. Construction validation still costs
96.198820seconds; removal validation13.309833seconds versus50.326636previously.
There is substantial unresolved exact-contact work; this is not an optimized
complete unsteady adaptation event. Repartition/return/solver/transfer/output and
CFD cost must still enter an actual BL lifecycle assessment.

The5440-cell contact-only replay hit its90-second ceiling without reporting an
invalid pair. Its failed receipt is retained; partial work is not a pass. The new
complete native construction subsequently validates the whole mesh under its
unchanged6144-work/300-second limits. No replay cap was raised to obtain a pass.

## Qualification and evidence reuse

Serial focused contact/sphere/existing primitive lifecycle checks pass4cases with
2511assertions. Current four-rank checks pass4cases/rank, and the independent15-lifecycle/
30-event oracle passes. Results are recorded in the linked evidence. Controls exercise support
subsets, rotated fallback, subnormal/large values, unshared/partial contact, one-ULP
separation/overlap and invalid input. All gates remain enabled.

Both new accepted meshes, reference sidecars, original32donors, full sampled tensors,
all native metric edges and configs must match the earlier accepted outputs BYTE-
FOR-BYTE before the verifier reuses their independent exact geometry/height/shape
proofs and complete identity-field release oracle. It validates current executed
sources/build/archive/binary and current lifecycle pins separately. No231-second
duplicate geometry audit; no assertion that the old build is the current build.
Identical scientific output may be represented by a closed repository symlink
only after comparison and recording; timing/receipt/source evidence remains new.

The big BL sphere is still serial native construction/removal, without CFD advance,
Hessian or normalization recomputation. The small actual primitive Euler lifecycles
cover N1M1 and N4M1/2/3/4, conservative current+BDF/domain accounting, transported
metrics, SU2/CGNS/reference persistence and restart; they are not MPI BL-sphere
construction/transfer or viscous convergence. No independent general spherical BL
metric oracle or composed-gradation completion is claimed. M6's exact-facet/
binary64 obstruction and budget uncertainty remain separate and unchanged.

## Resources and next capability

One owned heavy job, nice19, single build worker/library threads and local MPI<=4.
Build peak process RSS2.44GiB; construction observed~33MiB process RSS with~40GiB
host RAM available. Native GNU-time receipts retain their peaks; these are not an
aggregate multi-process MPI peak. No new retained cache or communication overhead.
Only owned helper intermediates are removed; executed binaries stay compressed
locally and failures/source capsules/receipts stay preserved.

The full goal remains open: complete enabled curved/interacting-wall budget
workflows, global uncertainty calibration/error allocation, general curved BL/MPI
construction/physical CFD, composed gradation, viscous validation and full-event
cost. Continue capability-first; do not start another unchanged M6 run or a tensor
micro-optimization ladder. The explicit policy contract is in
[GOAL3_BUDGET_AWARE_BL.md](GOAL3_BUDGET_AWARE_BL.md).

[Receipts, failed replay, inspectable grids and verifier](../integration_evidence/native_3d_bl_v1/contact_validation_v2/README.md).
