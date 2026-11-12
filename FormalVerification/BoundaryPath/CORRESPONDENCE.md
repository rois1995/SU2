# What the source connection establishes

The source manifest fixes original files and selected byte spans at upstream
`d162016382f0c122433000c114e88d808b48f7a7`. Selection by Python is unverified;
reviewers can compare each span against Git. Generated Lean contains the raw source
strings, not a Python AST. Parse success is kernel checked; failure cannot become
a default implementation. The generator rejects missing/ambiguous method anchors.

| Operation | Production location | Formal connection and limit |
|---|---|---|
| Boundary ordering | `CIntegration.cpp`, `Space_Integration` | Complete routine inspected: weak far field precedes final Euler wall. Ordered visit lists are supplied, not extracted by a verified caller semantics. |
| Stationary Euler wall | `CFVMFlowSolverBase.hpp::BC_Euler_Wall`, forwarding to `.inl::BC_Sym_Plane` | Three complete reduction/correction fragments parsed. Two-dimensional serial semantics proved to match the accepted wall projection. Stationary branch, iVel=1, pointer bindings, ownership, normals and vertex traversal are manual caller connections. |
| Far-field state/cache | `CEulerSolver.cpp::BC_Far_Field` | All characteristic comparisons and selected arithmetic manually represented, with formal reconstruction/geometric/admissibility proofs. No certified translation of the complete routine. Its owned-loop inputs, normal negation and cache mapping are manually inspected/native tested. |
| Far-field flux | `CDriver.cpp` selects `CUpwRoe_Flow(...,false)`; `roe.cpp::ComputeResidual/FinalizeResidual`; `CNumerics.hpp::GetPMatrix/GetPMatrix_inv`; projected Euler flux helper | Closed real model, explicit matrices, zero-c² branch, arithmetic dissipation-loop proof and conservative reconstruction proof. Matrix entries and scalar-Roe selection are manual source representations. No claim of verified complete scalar C++ Roe implementation or eigensystem derivation. |
| Far-field accumulation | `CEulerSolver.cpp::BC_Far_Field`, `LinSysRes.AddBlock` | Full call parsed; AddBlock versus SubtractBlock controls its stated semantics. The called vector method's memory/object semantics are inspected/native tested. |
| Full numerical update | `CEulerSolver.cpp::ExplicitEuler_Iteration` -> `CFVMFlowSolverBase.hpp::Explicit_Iteration<EULER_EXPLICIT>` -> `Explicit_Iteration_impl` | Point and component loop headers parsed, finite traversal and complete memory update proved. Reuses preserved source gates for Identity.apply, volume/dt scale statements, explicit Euler switch arm and scalar AddSolution. Linking those fragments into the C++ body is inspected/native tested, not a verified C++ frontend. |
| Geometry/time getters | `CPoint.hpp::GetVolume/GetPeriodicVolume` (both overloads), `CVariable.hpp::GetDelta_Time` | Complete getter methods parsed to storage-member loads. Memory representation theorem connects to supplied volume, periodic volume and timestep arrays. Native calls test real getters. Allocation, initial values, volume geometry and timestep computation remain open. |

The grammar is deliberately closed: exact loop initialization/increment forms,
known bounds or bound-minus-one, specified getter method forms and members, wall
reduction/store loops and boundary block-add/subtract calls. Prefix/postfix increment
refactoring is allowed because the value of the increment expression is discarded.
The bound-minus-one diagnostic fails explicitly when the point bound is zero;
its accepted semantic witness has two points and no unsigned underflow. Original
bounds admit an empty domain. No general expression parser or full-method C++ verifier is claimed. Unknown tokens,
extra statements, extra conditions and trailing content in a selected fragment fail.
New syntax in surrounding unparsed routines changes source identity and requires
review; identity checking alone is not a semantics proof.

## Five distinct evidence levels

1. Independent mathematics: real-valued boundary, flux and update specifications and contracts.
2. Formal implementation semantics: manually assigned closed-fragment memory semantics,
   manual characteristic/Roe arithmetic, finite loops; the listed Lean refinements apply here.
3. Actual source identity: Git commit, full-file hashes, byte spans, regenerated raw strings.
4. Native evidence: unchanged compiled methods and selected-fragment mutations/refactor tests.
5. Genuine C++ semantic correspondence: **not established**. A verified C++ frontend,
   C++ object/heap refinement, native floating-point/library semantics, compiler guarantees
   and actual dispatch/caller proof remain required.

The new boundary model has no unconstrained finish or boundary-flux callback. This
closes that interface in the mathematical model; it does not certify all boundary
C++ or whole-path execution. The final theorem's explicitly supplied boundary
visits, primitive snapshots, normals and ready memory are unverified production
interfaces, not disguised output assumptions.
