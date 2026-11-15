# How to proceed

Step 2 is numerically complete, including both profiles: see [current handoff](HANDOFF_Codex.md), [checks](Handbook/ADAPTATION_M6_CHEAP_GROUP_20261115.md) and [profile tables](Handbook/ADAPTATION_M6_MEASUREMENT_CHEAP_GROUP_20261115.md). Both B adaptations passed and all six final payloads match P6c byte for byte.

Current task is step 3 F3: integrate the exclusive timer tree, validate the ledger and unchanged numerical behavior, and measure named coverage >=95%. Evidence: R/f3_exclusive_20261115. Initial build500 failed before numerical validation; preserve its receipts. Verify each live build/job handle before editing pinned sources or launching another heavy job. Run each numerical check once; stop the first failure without an independent numerical fix loop.

Keep 5400 s per native event, all strict mesh/geometry/height gates, and sum of rank process peak RSS <=20 GB decimal. Do not substitute sampled RSS or change modeled memory options/cache quotas. One owned heavy job, nice19, at most six heavy ranks, single-threaded libraries, 4TB build scratch.

After F3, reread the exact reviewer-owned ADAPTATION_ONE_ENGINE_DESIGN_V1.md path from the handoff. Source-check 5.9/5.10/5A/5B and M11/M15/M17; measure every requested number M1–M10/M12/M13/M14/M16, with missing values explained. No steps 8.2–8.7 before user acceptance. Curved BL readiness, dedicated BL MPI and RANS500k remain open.
