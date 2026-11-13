# Connected path coverage and remaining obligations

| Stage | New or preserved evidence | Open obligation |
|---|---|---|
| Conservative state, thermodynamics, oriented conservation | Preserved checked foundations and M1 compatibility | Machine arithmetic correspondence |
| Selected constructor current/old/truncation/primitive/V²/dt | New projected constructor execution proof, M1 admissible-backup theorem, actual constructor observations | Real allocation/layout, complete auxiliary state, nondimensionalization and restart paths |
| Complete primitive preprocessing | New ordered read/write execution, both fallback branches, shared fluid/entropy scratch, all-row fold and counter refinement | C++ callee translation, aliases/lifetimes, serial macro/library semantics, FP/libm |
| Residual clear and old snapshot | New caller composition derives representations; preserved copy/clear proofs | Real bulk-container semantics and later-stage frames |
| Timestep, volume and geometry | Initial dt=0/frame proved; getters/update relationship preserved | Actual timestep computation, positive-volume geometry, old/current persistence through these stages |
| Interior SIMD Roe and edge traversal | Source trace only; physical Euler flux M1 compatible | Literature-approved Roe compatibility, SIMD lanes, geometry normals/edge order, scatter/reducer and ownership |
| Wall/far-field boundary memory | Frozen BoundaryMemory, source-level external pass | Independent cold/kernel/full native acceptance pending; far-field/Roe scientific approval and C++ interfaces open |
| Complete explicit update | Preserved loop and source gate | Readiness derived from actual timestep/interior stages; FP and admissibility across update |
| Whole iteration and future iterations | Preprocessing/snapshot prefix is now connected in the declared model | Compose intervening verified stages; prove old-buffer admissibility over later updates; communication/auxiliary frames |

The complete path is **not verified**. The new model removes primitive conversion and snapshot representation assumptions for this prefix; it retains initial allocated-buffer/parameter representations. It does not replace interior residuals or timestep generation with arbitrary functions and call that path completion.

Dependency graph: approved state/thermodynamics + protected point spec → Stage/Point execution refinement → complete all-row Loop refinement → CommonPreprocessing clear/counters → snapshot representation. Constructor initialization supplies DataRep and optional scientific backup admissibility. BoundaryMemory/update remain downstream separated by the explicit timestep/interior gap. These dependencies are also visible in Lean imports and theorem types; the new audit checks transitive axioms across all imported SU2/CFDLean logical declarations.

The next milestone should pin independently approved Roe specifications when available, then establish geometry/owned-point/edge enumeration and SIMD lane/scatter contracts. Prove residual-zero entry, coverage and conservation for the actual interior loop before supplying pre-boundary memory. In parallel, give SetTime_Step a restricted source model and prove its getter/storage/frame relationship. Neither stage may assume its own final output. Later connect preprocessing/snapshot → timestep → interior → existing boundary-memory → update, and address explicit-update admissibility for the following iteration.
