# Specification request to Agent A — boundary-memory milestone

Delivery: local request prepared at the user's direction for forwarding to Agent A.
This is a request, not evidence that Agent A received it or approved any specification.
Reply in a separate handoff with exact revision, statement identifiers and approval status.

We observed CFD_Lean M1 at `0a88e36b3810cd5adc3881db526ca8a35f81afbb`, with scientific
and independent proof acceptance pending. No CFD_Lean dependency is imported now.
The SU2 work preserves its frozen candidate and executes its selected boundary stores
in an exact-real logical memory model. Its far-field/scalar Roe mathematics remains
provisional; SU2 source is implementation evidence, not scientific authority.

Please provide or schedule the following independently literature-derived contracts:

1. M1 state and physical-flux interface: density/momentum/energy conventions,
   primitive conversion, total enthalpy and temperature identities, admissibility,
   zero/singular cases, normal and area-vector conventions, and approved revision.
2. Stationary slip-wall projection: assumptions for normal-only momentum correction,
   impermeability, unchanged density/energy, residual/truncation vector interpretation,
   and the limits of composing different wall normals at shared points.
3. Far-field characteristics: incoming/outgoing convention, subsonic/supersonic and
   sonic equality branches, advected quantities, reconstructed-state admissibility,
   and required thermodynamic domains. Please derive the requirements from the
   cited primary references, then identify supported variants explicitly.
4. Roe boundary flux: state averaging, consistency/linearization, left/right basis
   conventions, entropy modifications and invalid acoustic-state branches. Separate
   a mathematically justified domain restriction from behavior defined outside it.
5. Compatibility process: identify which statements are approved, proposed, or open;
   supply exact bibliography locators and a pinned Git commit plus approval record.
   We will propose representation/compatibility lemmas and a discrepancy matrix
   before importing an approved revision. Differences will be classified as proved
   agreement, intentional variant awaiting approval, potential defect requiring a
   witness, method limitation, or unresolved correspondence/literature gap.

No request asks you to copy or alter a mathematical contract to match SU2. The SU2
team will separately map the actual selected operations, cache layout and storage
semantics to the approved statements. We will not edit your repository or reinterpret
approved requirements. Please report boundaries/Roe as pending if they are outside
the current approved milestone; the memory-execution work can proceed with that
limitation explicit.
