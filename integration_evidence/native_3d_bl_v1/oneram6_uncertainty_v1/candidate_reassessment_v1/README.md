# Original-field candidate reassessment: qualified local integration, not full Goal3

2026-10-17; branch codex/native-3d-bl, base f6b4417. Full Goal3 remains ACTIVE.
[Implementation, measurements and limitations](../../../../Handbook/GOAL3_CANDIDATE_REASSESSMENT.md).

- Actual accepted RANS/SA N4/M2 mesh:6 ->24 cells, same original P1/geometric BL;
 8192 samples. Indicator .165326 ->.0155528 (10.63x lower), observed error .7910%
 ->.05619% (14.08x lower). Reference3.6941577302; fine estimate3.6962333832.
 Reassessment .02094s; original index reused for return. Budget1 remains infeasible
 for this constrained field; this is not budget-normalization success.
- Smooth6 ->384-cell integration control: indicator4.30x lower; original source
 unchanged. This is subdivision, distinguished from the actual adaptation above.
- Frozen complete M6 RAW9 coarse negative control: centroid596350.33, sampled
 979024.48 +/-101701.02 HEURISTIC. Coarse centroids miss substantial BL mass;
 equal-budget complete-support sampling has much lower variance. No accepted M6.
- MPI1/2/4 clouds/global controls pass; every M6 row matches exactly MPI2/4.
 Independent original P1/PDF/containment and accepted planar composed-field audits
 pass. Affected curved reader, full RANS current+BDF/SA/restart/SU2+CGNS pass.
- No normalization correction, target relaxation, full regression, CFD rerun or
 cluster job. Geometry/h0/positivity/q/J/full composed-edge/transfer gates retained.

`summary.json`, `accepted_partition_control/comparison.json` and per-case
`result.json`/`provenance.json`/`inputs.json` contain measurements and identities.
`executed_campaign.py` is the exact runtime orchestrator matching these receipts.
`report_helper_transition.json` verifies all nine completed cases and documents the
report-only NumPy scalar correction in current `campaign.py`, without rerunning SU2.
A direct replay must respect that helper transition or use a fresh versioned folder;
never overwrite original provenance. `compare_accepted_partition.py` separately
pins the added coarse/accepted comparison. The original123MiB M6 export is shared
in ../correction_domain_v1, SHA391d190e6c177d5c71f6c5bd544bfec265637ef142cfcfcafeb7e68b87e95e5a.

Failures are retained: build field-access typos; `rejected_initial_controls_np1`
and `initial_batch128_source.cpp.gz` record an invalid batch above the source API's
64-point cap; diagnosis logs identify it. The fixed caller uses MaximumCavityCells.
The runtime campaign passed before NumPy Boolean summary serialization failed;
finalization/reuse logs preserve that distinction. One owned heavy job, nice19,
build-j1/thread1/MPI<=4. Largest build process2.41GiB/replay667MiB, observed host
available RAM~39–41GiB/no swap growth; process maxima are not aggregate job peaks.
