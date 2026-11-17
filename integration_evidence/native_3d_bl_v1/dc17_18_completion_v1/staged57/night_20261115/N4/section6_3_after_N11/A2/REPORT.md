# A2 setup ownership — scoped G1/G2 PASSED

Base is passedA1dfa2782e17. Original wholeN4sourcecheck ../source_check.md remains the ONE22-reading list. Three production paths changed: CNativeEngine3D.cpp and new private native_engine3d/EngineSetup.hpp/.cpp. No API/test/config/export/build-list change; pair is included in the original private namespace/TU. Both new files clang-format checked. Source hashes and exact owner order in source_change_manifest.json (historical pre-validation snapshot; acceptance is this report/acceptance_summary.json).

EngineSetup owns the original RAII resource holders, including D9/wall/lane roots added after the old inventory. Each optional owner is emplaced at its original declaration point; defaulted destructor releases them in the same reverse relative order, admission last. Copy/move deleted. RestoreBudgetWindow uses in-place construction plus primitive snapshot assignments, no prematurely destroyed temporary. AtlasBudget all scalar defaults checked. Original clocks/value captures, geometry selection, by-value options, conditionals/collectives/lambda bodies and direct early parameter rejection remain. No new timer operation, quota or sizeof(Context) charge. No raw decltype/typeid use of the changed reference names exists.

| Ordered check | State | Numbers |
|---|---|---|
| Build538 | PASSED |80.123401s/4actions/1611pins unchanged/all12buildIDs+controllerterminal|
| G1serial | PASSED identity |375cases/ZEROchanges/reportertotals identical/790.791954s|
| G1MPI2 | PASSED identity |57cases/rank/114records/ZEROchanges/15.855281s|
| G1MPI4 | PASSED identity |57cases/rank/228records/ZEROchanges/15.099789s|
| StrictG2 | PASSED identity |363.203256s plannedstop/371.619170s sealed; everyoldinteger/model/per-rowTIMERcall EXACT,12snapshots/6lawrows/noallowance|

All inputs/source/helper/binary pins unchanged. All G1 actual PID/start identities terminal; all9G2IDs+controller1720751 actualterminal. G1 retains24/19/37baselinefailedassertion records. No check repeated and no own fixture needed correction for this substep. Observed G2 prefix/seal SUMrankHWM maximum 6083411968B<19,999,490,048B, sampled prefix scope rather than a final full-event peak; no speedup claim.

A2 is a passed state substep; phaseA/N4/fullcapability notcomplete. Next A3Control then remainingunits/A4; phaseendG3/G4/GQ/G5 stillrequired. No BL/RANS capability claim.

Own report-artifact correction after validation: git diff --check flagged valid patch context blank lines as trailing whitespace in two newly staged .diff files. The pinned raw patches were NOT rewritten. Preserved them byte-for-byte locally, stored exact gzip copies with raw/compressed SHA manifest, and staged the compressed artifacts instead. Repeated the SAME git whitespace check; no source, numerical gate, tolerance, build or numerical run changed.
