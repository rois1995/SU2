# Timestep and interior-path candidate

This is a proposed frozen continuation of preprocessing `2ffc490435e57ebb1c59bc6cc706f24c2ce9b86e`. The parent and all earlier mathematical/source candidates are unchanged. The user reports external **source-level** review of preprocessing, including its 96 explicit theorem declarations. Independent cold Lean/kernel, dependency and full native acceptance of that candidate remains pending. This candidate does not confer acceptance on its parent.

The new exact-real implementation-model composition executes constructor CFL stores, preprocessing and snapshot, steady inviscid timestep calculation, natural-order SIMD interior gathers/flux/scatter, the configured inactive interstitial stages, the preserved boundary memory transformations and the complete explicit update loop. Its memory premises concern the pre-preprocessing input. Neither final timestep values nor an already assembled interior residual are supplied as expected memory.

`SU2Interior.connected_iteration` is the principal composition; `preprocessing_to_boundary_memory`, `timestep_refines` and `interior_refines` are reusable contracts. `CFDCompatibility` proves limited compatibility with independently accepted CFDLean M2b averages. This is **not complete Euler execution-path verification**, certified C++ translation, or floating-point correctness.

Read [SPECIFICATION.md](SPECIFICATION.md), [SOURCE_CONNECTION.md](SOURCE_CONNECTION.md), [COVERAGE.md](COVERAGE.md), [REPORT.md](REPORT.md) and [REPRODUCE.md](REPRODUCE.md). `candidate.json` freezes source files and elaborated theorem types/dependencies. External evaluators must approve the specifications and independently control final acceptance. No production patch is included.
