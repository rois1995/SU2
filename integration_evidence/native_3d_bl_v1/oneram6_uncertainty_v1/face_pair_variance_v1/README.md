# Fixed-point-budget reflected face experiment — completed and withdrawn

Base0ad73b87450a22035ae119d58548a4a740f0d7fe; full Goal3 ACTIVE/incomplete.
No production estimator, tolerance, target or mesh acceptance change. Experimental
DrawFacePairs reflects the sign of the wall-face proposal's normal displacement;
other component kinds duplicate their point. Each marginal keeps the FULL mixture
PDF, reevaluated at its own position. Pair correlation is intentional; independent
units are pair means. The old scalar Correction replay's IID SEM is NOT valid for
this input and will not authorize any normalization or mesh acceptance.

Compare8,192 paired points /4,096 independent pairs with the saved8,192-point IID
prefix at the SAME query budget and disjoint24,576-point heldout correction.
Check two original M6 streams, MPI2/4 exact rows, actual original P1/direct BL, and
the embedded planar control where positive covariance can make pairing worse.
Default Draw must reproduce saved32,768-point streams bit for bit. Existing work/
storage ceilings stay fixed. No new source fitting, domain resampling, geometry/
height/q/J/length/transfer change, CFD solve or remesher run. One owned heavy job,
nice19/build-j1/thread1/MPI<=4; inspect resources before starting.

## Outcome and decision

| Case | Paired correction | Correct pair SEM | Saved IID8,192 SEM | Pair / IID SEM |
|---|---:|---:|---:|---:|
|M6 seed20261017|816936.5004|7901.4973|9376.9951|0.84265|
|M6 seed9919|809768.3779|8016.9806|9416.5831|0.85137|
|Embedded planar seed449|10.67709028|0.04710745|0.03259951|1.44504|

M6 empirical SEM is15–16% smaller (about28–29% less empirical variance) at the
same8,192 POINTS, each stream having4,096 independent PAIRS. This is no accuracy
proof: M6 has no independent exact whole-field integral. Discrepancies with the
separate24,576-point holdouts are2.330/0.835 combined empirical SE. Pair covariance
is negative for these M6 samples, but positive for the planar sample. The planar
pair SEM is about45% worse. Its total13.85907079 differs from coarea13.90130035 by
0.303781%, within its heuristic3pairSEM but above the retained0.05% reference
accuracy criterion. The distinct actual strict planar solver control still passes.

The legacy native scalar replay prints an IID SEM:9481.62/9417.66/.03330797 here.
Those are INVALID for dependent points. All reported pair estimates use complete
pair means from owner-routed rows, including outside zeros; no normalization or
acceptance consumes the legacy IID indicator. The first M6 stream's full point/
donor/sensor/composed/correction/weight rows AND pair-aware assessment are exactly
equal at MPI2/4. This is offline global assessment, not implemented native pair
covariance or a production normalization path.

Full PDFs on all8,192 points per paired stream agree with independent Python.
Every first member matches its saved original draw; source scalars match the saved
anchors. Independent sensor density covers every inside point, with exact rational
containment/weights on first32 and worst32 conditions. Default Draw produces the
previous32,768-point M6 query files BIT FOR BIT for both seeds.25 focused serial
cases /13,570 assertions and the affected MPI4 dynamic-estimator controls pass,
including independent unchanged strict planar integration. No mesh/flow run.

The fixed POINT budget is not an exact equal-cost budget: measured PDF candidates
are14.284M/14.772M, below existing80M total work, and all work/times remain in native
metadata. No repeated timing/scaling or complete adaptation speedup is claimed.
The separate background interval remains far above a stringent root
request, and pairing worsens a legitimate geometry/domain. A default would need a
new independently piloted covariance choice and native MPI pair accounting. That
additional mechanism is not justified by these modest results.

**Withdraw the experimental API/replay flag/tests from production.** Keep the
independent sampler, no new tolerance/sample budget/target/correction behavior.
executed_source.patch.gz and executed_source_capsule.json.gz preserve the exact
executed C++ addition against0ad73b87450a22035ae119d58548a4a740f0d7fe, along with all
pins, logs and source/data checks. executed_summary.json is the original completed
experiment receipt; summary.json adds disposition only. After restoring the three
owned C++ files and rebuilding, EVERY pre-experiment source/build/test-binary pin
matches exactly. restoration_verification.json proves reuse of prior native checks;
no repeat native regression or full M6 attempt was necessary after withdrawal.
The standalone CFD executable was also rebuilt and fingerprinted, not rerun.

## Resources and reproduction

One owned heavy job, nice19/thread1/build-j1/MPI<=4. Experimental build1m39.02s,
pilot39.35s, resumed connected campaign1m04.33s, restored build1m38.02s. Peak build
process2.40GiB; replay process~667MiB, neither an aggregate job peak. Host available
RAM stayed40–41GiB, host swap did not grow, GNU-time swaps0. The process-tree sample
was taken after the pilot exited (zero live descendants); it is not a zero-memory
claim. All owned handles terminal. The full-PDF audit is reused only after actual
query/anchor/independent-helper hash and pass checks; the original pilot helper is
saved separately from that verifier-only reuse change. No source cap or work limit
was raised and no source donor export/large historical binary was duplicated.

The current branch intentionally cannot regenerate the paired data without the
saved experimental patch. Replay in a separate checkout of the base with that
patch, rebuild and use a fresh evidence folder. Existing execution receipts reject
changed source/binary identity; never rewrite old cases to make them pass. Shared
original.bin is restored with the existing digest-checked support_correction.py
domain facility. All geometric/source fixtures and complete point data are pinned.

This experiment does NOT test whether an actually accepted finer mesh improves
integration. That hypothesis remains open. Stop this sampler follow-up; next couple
candidate-mesh reassessment of the SAME original P1/geometric field to bounded
normalization stages before publication. Do not refit donors, rescale clipped/
graded tensors, change target during local operations or add another sampling/
subdivision ladder. Full Goal3 remains active/incomplete: global error/work policy,
actual reassessment/correction, difficult curved/M6 acceptance, transfer/viscous
accuracy and complete-event affordability are still required.
