# N22 fixture and helper corrections

- The first no-launch import of the ordered controller resolved the older N16
  module named run_scoped. The controller now loads the exact N22 script by
  path. The draft-pin self-check verified four Part M and eight Part C files
  without starting a numerical job.

- Part M's two-event workflow and all scoped checks passed. At the checkpoint,
  the archived scoped stdout was a snapshot taken before its writer exited.
  The initial archived content had SHA256
  `bf50f43f1efe34234754a20c1d0a2863032ddbfb31db1b86a8991233f3c62525`.
  The full scoped stdout had SHA256
  `0a2411fa62f4d778f2e80a1935a59a3a6c4077dabd7608e73e3ed993c23b85af`.
  The earlier top-level archive was also written while its log was open.
  The recovery preserved the initial archives in `R3_M_archive/`, verified
  the original process identities terminal, and repacked the complete stdout.
  The scoped helper now defers archiving its active log to the terminal
  checkpoint. The checkpoint recovery reuses the numerical receipts without
  rerunning a gate.
