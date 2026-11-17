# B4 unapplied proposal status

No production source was changed. The generator passed its source hash checks, exact main translation unit reversal, alias/free-capture checks, and actual removed-lambda token checks. No build, test, or numerical job was run.

Primary review found a C++ declaration-order issue: the non-template
`EngineQueues::Coalesce` called `CoalesceOrder` before its template definition,
with no prior declaration in `EngineQueues.hpp`. The generator now emits the
`CoalesceOrder` definition before `EngineQueues::Coalesce`, retaining the
existing feature closure by reference and without runtime callback dispatch.
The generator was rerun; token/reversal checks passed again. This is a draft
ordering correction, not compile evidence.

The proposal is bound to the current build552 snapshot and must be regenerated against the accepted post-B3 source before any B4 edit:

- `CNativeEngine3D.cpp`: `7545edb4cea518fffc7e0e5cd4f763891fbe2a002b2d26b177b04b76e0d3b848`
- `EngineQueues.hpp`: `359d3ef321fa1a6d2722f9ad4fe1667d970f031c5c51c6a1266510b580fc5a55`
- `EngineQueues.cpp`: `701f2fe570f59a2d289611f504ca59205cdc14fe98e67855dc50fea301c784e5`

The two existing lambdas are extracted to `EngineQueues::Coalesce` and `EngineQueues::ServiceFrontier`. The former's post-exchange order/dedup block moves to a local function template that accepts the existing `feature` closure by reference, preserving its identity and exact expression. The latter's receipt-service loop moves to `EngineQueues::ServiceReceipts`, borrowing the same receipt vector and `CLocalFailure` by reference. No state fields or payload copies are added. Call counts are 2 and 7 respectively.

All four formatted functions are below 80 lines: Coalesce 45, CoalesceOrder 51 (including template line), ServiceFrontier 63, ServiceReceipts 33. No duplicate aliases, unused aliases, or known unbound Adapt captures were found. The manifest proves body token equality against each actual removed lambda after inlining the helper and stripping only alias declarations. The main translation unit reverses byte-for-byte.

Proposed SHA256:

- `CNativeEngine3D.cpp.proposed`: `64896501553cd89cfca84ea09c7f14dba72df6301f70113cde7cc375d6926671`
- `EngineQueues.hpp.proposed`: `6d4c201a248f5c2f5524a3cf460319ab61ea5f6587c29e07508733d049e4c575`
- `EngineQueues.cpp.proposed`: `e82a0e05c2843a547abea16a04f347c0ce8b9d4feb43675207c48e8814955278`
- `binding_manifest.json`: `a3e9152c34c5e1fc4e1ca8cd3d5b4d71d8aef016a1d696350958d5a000949d27`
- `generate_b4_proposal.py`: `a44eec84e1f2751bf699d3fe968224766cec14fa091738e2d7f5204b720f70a4`

Token checks are not a compile check. The proposal files are under `B4_preparation/proposal/generated/`.
