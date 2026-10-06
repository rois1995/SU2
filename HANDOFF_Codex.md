# Codex handoff — native integration

Updated 2026-10-06. Goal active; see NATIVE_INTEGRATION_GOAL.md.
Branch codex/native-integrated. Worktree /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated.
AdapNoExt remains feat_adap_noExt at 88828474db; its .gitignore change is preserved.
Do not write new artifacts into that checkout or alter other workers' branches.

Pinned branches have been reconciled; see BRANCH_RECONCILIATION.md. Stage G
uncommitted fixes were copied from an assessed, hashed snapshot, without
modifying the Stage G worktree. B0 remains standalone comparison research.

Previous completed native baseline: 57f0550db4; original evidence remains
read-only in AdapNoExt/BL_NATIVE_INTEGRATION_WORK. Do not reuse those passes
as proof of this integrated build. New evidence/builds live in this worktree.

Next: fresh primal MPI/MMG/CGNS build, sequential focused matrix, independent
mesh audits; then AD Stage G controls and controlled robustness/scaling sweeps.
One heavy job at a time; -j2, MPI <=4, OMP/OPENBLAS threads1. Never stop foreign
jobs. Check live process before restarting a supervisor; a tool observation
timeout is not job termination. /tmp has about7.4GB free, 4TB disk67GB at start.

Dependency symlinks externals/{codi,eigen,medi,mel,meson} point read-only to
installed main-checkout sources; leave them unstaged. Ninja is in the prior
scratchpad/ninjabin directory. Static MMG needs mmg_scotch_root=/home/rausa/Software/scotch.

Current validation state: pending fresh integrated build. No scaling limits
have yet been established for the reconciled branch.
