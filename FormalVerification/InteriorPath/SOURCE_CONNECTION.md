# Source connection and actual execution trace

Numerical baseline: `d162016382f0c122433000c114e88d808b48f7a7`, without production patches. Live upstream develop was rechecked as `103442f30ca2fba1ff48ef12aa1ae93480256a30`; only two CI files differ. `baseline.json` records this gate. `source-manifest.json` and `fixtures/` contain regenerated full-file identities, exact byte spans and complete selected bodies. Extraction is unverified tooling and must be independently checked against Git.

| Source operation | Formal model/evidence | Correspondence status |
|---|---|---|
| `CFVMFlowSolverBase::Init` all-point LocalCFL stores | `initializeCFL`, all-row value/frame proofs | Manual source mapping; actual constructor observations |
| CEuler preprocessing, common primitive loop and old snapshot | Protected Preprocessing candidate | Preserved source gate and memory proofs; independent cold acceptance pending |
| `CEulerSolver::SetTime_Step` → base `SetTime_Step_impl` | `executeTimeStep`, `timestep_refines` | Complete selected-branch manual model; store statement Lean parsed |
| Actual projected velocity, acoustic/CFL/lambda/dt accessors, geometry volume getter | Option loads/stores and contracts | Accessor interpretation/manual mapping plus actual-method native observations |
| `CGeometry::SetEdges`, `GetEdgeColoring`, `createNaturalColoring` | Typed endpoint/neighbor inputs, natural traversal proofs | Construction/ownership are unverified source obligations; serial source trace/native fixture checks |
| `CPhysicalGeometry::SetControlVolume`, CEdge 2D helpers | Volume/normal helper and repair proofs | Helper mathematics/manual mapping; whole mesh/control-volume implementation not refined |
| `CEulerSolver::Upwind_Residual` → `EdgeFluxResidual` | `packets`, gathers, active updates, diagnostic fold | Serial natural-color branch; complete k-loop selected; manual loop representation |
| SIMD factory → ideal-gas Roe → no-viscous decorator | Explicit selected arithmetic in `RoeModel` | Manual implementation model; full method/tensor source selected; native finite comparison |
| `reconstructPrimitives`, `compressibleConservatives`, averages, P/inverse P, finalizeFlux | Six-field first-order reads and literal arithmetic | No MUSCL/viscous/low-Mach/implicit branch claim; full independent Roe refinement pending |
| `CSysVector::UpdateBlocks` and `UnpackBlock` | Active component-interleaved memory execution and assembly | Complete scatter method parsed by retained closed Lean grammar; unpack/accessor/ABI mapping remains qualified |
| CIntegration calls between interior and weak boundaries | `executeInterstitial selectedInterstitial` | Explicit false source guards and empty inviscid viscous dispatch; source/config mapping, scratch heap semantics open |
| Far field, fixed-value dispatch, walls, explicit update | Preserved BoundaryMemory and BoundaryPath | Source-level review statuses retained; mathematical characteristic/Roe formulation provisional |

Actual selected first-iteration trace:

1. Constructor/initialization, geometric and numerical setup; all-point CFL stores. The theorem begins with already allocated source-decoded storage; it does not prove all setup.
2. Multigrid pre-smoothing calls preprocessing, then old-solution snapshot, then SetTime_Step.
3. CIntegration calls Upwind_Residual. Ideal-gas ROE without low-Mach correction selects EdgeFluxResidual. The serial strategy uses one natural color, no reducer.
4. Each packet gathers endpoint indices, coordinates and normals, then first six primitive fields at stride 11. MUSCL=false uses first-order rows, no nonphysical-counter writes. Coordinate-distance and unused temperature reads are included in source context; the numerical model uses only operands relevant to the selected arithmetic. Their safe C++ loads remain heap-decoding obligations.
5. All lanes compute flux. UpdateLinearSystem's COLORING branch calls the SIMD vector update. Active lane/component writes occur in source order; inactive gathers still affect reconstruction diagnostics if their preexisting counters are positive. First-order initialized counters are zero in the selected native configuration.
6. CIntegration calls the empty inviscid virtual viscous routine, Source_Residual with all applicable guards false, then (dual-time false) skips that source. Source_Residual still loads a numerical-object table slot; valid table/pointer storage is an explicit C++ obligation. No body force, rotating frame, axisymmetry, gravity, harmonic balance, vorticity confinement, MMS or Python source executes.
7. BC_Fluid_Interface allocates/frees scratch but visits no FLUID_INTERFACE marker. Giles preprocessing is disabled. Weak-boundary marker order executes far field; Euler walls have no weak-case contribution.
8. Impose_Fixed_Values resolves to the empty base method. No selected strong-BC/periodic branch executes. Final marker pass executes stationary Euler walls in actual marker/vertex order. The protected boundary model retains repeated visits.
9. Time_Integration selects ExplicitEuler_Iteration and its complete point/component loop. It reads the newly stored local pseudo-timestep, old, residual and truncation buffers, and geometry volume/zero periodic volume.

Only the complete SIMD scatter method and complete capped timestep-store statement have new/reused restricted Lean source grammars here. All admitted tokens are consumed; unsupported characters, extra statements or unknown syntax fail. Other complete selected source bodies are **not silently parsed as identities**: they are explicitly manual mappings with remaining correspondence obligations. Numerical mutants compile and violate unchanged contracts; parser/hash/compiler failures are not counted as detections.

The imported M2b average compatibility proves velocity, enthalpy and sound under explicit row/coordinate conventions and positive densities; sound positivity additionally uses independent admissibility. It does not certify SU2's P/inverse-P normalization, complete Roe dissipation, characteristic far field, floating point or C++ execution. The scalar boundary and SIMD interior Roe models remain distinct.
