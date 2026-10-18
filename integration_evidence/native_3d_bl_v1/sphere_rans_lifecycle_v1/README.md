# Actual curved RANS lifecycle and bounded original-field queries

Checkpoint scope: actual initialized RANS/SA current+BDF transfer, CGNS restart,
and both mesh formats on N4/M2. Full Goal3 remains ACTIVE. No CFD timestep, Hessian,
complexity normalization or budget selection is performed by this sphere fixture.
The prescribed sensor is the original identity P1 field. This is neither viscous
accuracy nor unsteady robustness/scaling qualification.

## Results

| Event | Mesh | Native seconds | Result |
|---|---:|---:|---|
| Cold strict BL construction | 8 -> 5440 tets | 114.506 | Accepted historical producer |
| Historical restarted removal | 5440 -> unchanged | 2.81 | Atomic donor-cap rejection |
| Current restarted removal | 5440 -> 320 | 37.664 | Accepted, independently audited |
| Current unchanged-BL update | 5440 -> unchanged | 61.350 | Atomic donor-cap rejection in private coarsening |

The cold event and current removal are separate binaries with separately retained
executed-source capsules and provenance. `n4_m2_query_v1/construct` is a closed link
to the historical accepted `n4_m2_v2/construct`; do not repeat that construction.
Current restart states are checked against its actual checkpoint by coordinates.
Final output is in `n4_m2_query_v1/release/event_written_SU2.su2` and
`event_written_CGNS.cgns`; the accepted BL grid is in the producer's same filenames.

Cold maximum normalized actual flow/SA integral defect1.142e-14; removal2.060e-14;
transported original sensor error0. Cold projection changes the polyhedral domain:
volume1.333333333 ->4.047044680, fill2.713711347/lost0. The projection ledger
accounts for fill and solver bounds; this is not conservation on an unchanged
physical domain. Removal has zero fill/lost. Exact topology/height/geometry proof
is reused only for byte-exact geometry signatures; the slightly changed released
interior is freshly audited (50560 exact overlap pairs,480 shared-face certificates,
163 links, qmin.398233/Jmin.100437, full identity-field maximum edge1).
Full BL tensor/edge acceptance retains native scope; no independent spherical BL
oracle is claimed.

## Connected production fix

The preserved exact physical-core witness has320 cells touching an edge endpoint,
only6 sharing positive length. Regional point queries now use existing canonical
owner sampling; scalar edge integrals import every positive-length intersection,
including intervals on donor faces/edges. Isolated point contacts have zero edge
mass. Source sensors remain original P1 and geometric composition runs at the
actual query. Existing closed geometric/point import contracts and the256 cap
remain unchanged. Genuine257-interval demand still rejects. No whole-mesh gather,
new MPI thread, profile change or acceptance relaxation.

Focused serial controls pass11cases/828assertions; MPI2/4 pass11cases per rank.
Actual strict and opt-in planar N4/M2 RANS/SA/current+BDF/SU2+CGNS/restart controls
and independent strict/enabled audits pass. These are compatibility controls,
not enabled curved/interacting-wall policy validation.

The unchanged-BL update passes refinement with0 edits, then private coarsening
fails on the older closed `ImportMetricCells` path after1338work/630 attempts and
one private edit. All four CFD input states remain byte-identical. The last
height rejection is an intermediate diagnostic, not the cause of the donor cap.
The remaining fix must reconcile canonical actual/ideal-midpoint point queries
and complete positive-length edge support across private measurement/proxies;
simply replacing ideal midpoints by rounded point samples changes the field.
Do not raise caps, disable coarsening or weaken height/metric gates.

## Cost and reproducibility

All15 native phases plus solver replacement, history/output, per-rank bounded bytes,
RSS and communication are in `lifecycle_rank*.json`. Cold validation107.017s;
removal refinement12.550s/coarsening25.004s/replacement.0840s, complete fixture38.355s.
Rank0 removal communication38.18MB/147261calls and RSS64.56MB are process-local,
not aggregate peaks. One generation worker; contended correctness runs, no scaling
or affordability claim. Initialization bypasses normal metric generation.

`run.py construct|release|update M N [case_folder]` saves one resumable actual event.
Use a fresh continuation folder with a closed `construct` link. `audit.py folder
construct|release|update` audits native outputs; cold producer uses retained
`audit_producer_executed.py` and its historical binary pins. `qualify.py` reuses the
existing bounded runner. `audit_strict.py` adapts current completed receipts to the
existing strict oracle; the enabled oracle is unchanged. `verify.py` checks this
checkpoint's sealed bytes, historical capsules and current qualification pins,
without rerunning CFD or the exact geometry campaign.

Failures retained: empty-rank fixture initialization; old CGNS reader guard;
incomplete synthetic breakpoint input; historical removal donor cap; current
unchanged-BL private-query cap. Build/resource receipts and executed source remain.
No full regression, M6 retry, geometry-authority change or cluster run.
