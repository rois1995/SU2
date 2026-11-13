# Local request for Agent A — user forwarding required

No delivery, receipt or new approval is claimed. The existing request `SPEC_REQUEST_A.md` remains unchanged. This file is provided locally for the user to forward.

Agent B has observed independent acceptance of CFDLean M2b `c92c0f66360f85c0a3339ab9b30fc00c39ee5f93` (record `b4fc959ba820a7a4752fff5d824f57f199b73d05`) and pins that exact mathematical revision. Old SU2 candidates retain their M1 environment. M1 state/physical-flux/toolchain/manifest bytes are unchanged at the new pin.

Compatibility proofs embed 2D conservative state in the 3D coordinates with zero third momentum and connect SU2's positive-density ratio averages to the approved weights, averaged velocity, averaged enthalpy and sound speed/positivity. They do not assert approval of full Roe or characteristic far field.

Please provide independently approved, literature-based specifications and exact revisions for:

1. Full 3D Roe flux, eigenbasis/absolute dissipation, repeated acoustic/contact eigenvalues, eigenvector scaling and entropy-fix conventions, plus a 2D embedding/restriction interface.
2. Treatment of positive-density/admissibility, sound-square degeneracy, normal/area conventions and conservative energy/enthalpy inputs.
3. Stationary characteristic far-field selection including sonic/equality branches, entropy/advected-state reconstruction and boundary-flux contract.

The selected SIMD interior source uses sqrt(rho_R/rho_L) without abs or a c²<=0 zero-flux shortcut; the preserved scalar boundary model includes its own source branches. These are implementation variants to compare against independent specifications, not specifications Agent A should copy or silently adopt. Report discrepancies and compatibility domains explicitly. Agent B will not modify approved CFDLean specifications to fit SU2.

Private CFDLean source input stays local and off the public SU2 release. User/evaluator must arrange authorized private access separately.
