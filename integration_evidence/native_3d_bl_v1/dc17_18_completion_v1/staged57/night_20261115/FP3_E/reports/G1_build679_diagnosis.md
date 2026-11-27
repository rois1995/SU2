# FP3-E build679 G1 disposition

The initial G1 runs failed the exact assertion-identity gate. The first differing number in the serial cohort is the assertion location for `Bytes(result.cells)==saved` in `CNativeDistributed3D_tests.cpp`, reference line 3683 and candidate line 3684. Both MPI cohorts first differ on `strict.Query(rounded)`, reference line 3544 and candidate line 3545.

The read-only R5 diagnosis checked every unexpected case in the three preserved comparison receipts. All 375 / 114 / 228 case records were present. The 3 / 3 / 5 unexpected cases contain 11 / 19 / 37 assertion records, respectively. Each differed only by +1 in its line number. Case results, assertion contents and diagnostic values agreed. The new hidden fixture include was inserted before the existing test cases, shifting their source lines.

The one wiring correction moves that include to the start of the appended hidden fixtures. The comparator and its references stay intact. After the current scoped controller is terminal, rebuild the test driver under build680, verify the solver hash directly, and run Step 1 again on the resulting frozen pair. The first receipts and the independent build679 checks remain evidence of the initial attempt.

- [Serial receipt](../controls/build679/G1-serial/case_summary.json)
- [Two-rank receipt](../controls/build679/G1-mpi2/case_summary.json)
- [Four-rank receipt](../controls/build679/G1-mpi4/case_summary.json)
