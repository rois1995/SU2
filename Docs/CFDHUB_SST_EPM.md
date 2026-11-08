# CFDHub execution and storage contract

The user selected CFDHub for the paper campaign on 2026-11-07. This replaces the
earlier blanket no-cluster instruction for this workflow. The message establishes
the execution destination and file policy; no production jobs have been launched.

## File locations

Keep all SU2 and postprocessing products **inside the modified SU2 checkout under
the user's global scratch area**. This includes generated meshes/configurations,
restarts, histories, sensitivities, solution fields, solver logs, comparison tables,
figures and job/provenance records. Run each job from its own directory there and
use explicit output paths for postprocessing. Resolve symlinks before checking
that application output paths remain under the checkout.

Planned layout, pending verification of the existing remote checkout:

```text
<user-global-scratch>/<modified-SU2-checkout>/
  runs/sst_epm_paper/<case>/<grid>/
    mesh/
    baseline/
    adjoint_drag/
    adjoint_lift/
    nonlinear/<mode>/
    results/
    scheduler/
```

`global_scratch` in the user's instruction refers to the cluster working area,
not a new subdirectory to create inside a home-directory checkout. The exact SU2
directory name must be discovered before transferring files; it is not assumed
to match another project's checkout.

**Leave MPI and scheduler runtime locations alone.** Do not redirect `TMPDIR`,
`TMP`, `TEMP`, `HOME`, Open MPI/PMIx session directories, shared-memory backing
files or scheduler spool files into the SU2 tree. Do not move or clean those files.
Keep scheduler-provided host/allocation files where the scheduler creates them.
This exception does not change the location of solver or analysis outputs.

## Existing access and scheduling conventions

The user's cluster-agent prompt, supplied on 2026-11-08, and its referenced
access/handoff documents are the authoritative operational inputs. Their private
connection details stay outside the published repository. Use the prompt's exact
gateway-to-login route, not the older jump-host route used in the failed check.
Follow its dedicated-folder rule: a new branch gets its own scratch study root
containing `src`, `build`, `bin`, `jobs`, `cases` and `results`; do not reuse another
study's binaries or output directories. The storage rule above applies to the
entire dedicated SU2 study directory, including these siblings of `src`.

Do not build or run SU2 locally under the updated operating instructions. Perform
builds only on the designated build host when requested; submit CFD through SGE.
Batch independent remote checks into one SSH call, monitor at most hourly, and stop
on any SSH/login failure without retries. Leave other sessions' jobs untouched;
launch a batch only when requested. Use only the queues authorized by the current
prompt, respect the shared job-count/quota budget, and retrieve only small result
files. Do not download meshes, restarts or volume outputs unless requested.

Use the user's existing local SSH configuration and transfer helpers. Resolve the
login/transfer aliases locally; do not embed account names, private host details
or credentials in published scripts.

The [CFDHub guide](https://cfdhub-userguide.readthedocs.io/en/latest/HowToSubmitAJob.html)
documents SGE. Existing local SU2 cluster scripts corroborate this workflow.
Verify current queue eligibility, parallel environment and resource limits on the
login node before preparing the final job request. Match the MPI launcher/toolchain
to the cluster-built reverse-AD executable without OpenMP. Do not transfer a local
binary and assume ABI or MPI compatibility.

Workflow:

1. Inspect the existing remote checkout, branch/revision, dirty state, storage and
   available scheduler resources. Preserve unrelated files and running jobs.
2. Transfer only the intended source/input changes with SCP or an explicit rsync
   file selection. Review rsync's dry-run listing first; avoid `--delete` and broad
   mirroring of a tree containing results. Do not copy local dependency symlinks
   that point outside the checkout. Verify revision/input hashes after transfer.
3. Prepare the case directories under the verified scratch checkout, retain the
   baseline-before-adjoint/endpoint dependencies and perform the pending MPI and
   numerical qualification gates before production interpretation.
4. Submit through SGE from a login node using the verified queue/parallel
   environment. Set the job working directory and requested stdout/stderr paths
   inside the campaign directory. Launch MPI only on the allocated resources.
   No CFD calculations run directly on master/login nodes.
5. Record the returned job ID; monitor `qstat -u "$USER"` and `qstat -j JOB_ID`,
   inspect application logs/histories, and use `qacct -j JOB_ID` after completion
   when accounting is available. A vanished queue entry or zero exit status alone
   does not establish convergence. Preserve failed-job evidence and avoid duplicate
   submission when a previous submission's outcome is uncertain.
6. Postprocess within the same scratch checkout. Retrieve selected results to the
   local project with checksums; keep the cluster products under the designated
   SU2 directory. Record job resources, source/binary/input hashes and actual costs.

The [working-area guide](https://cfdhub-userguide.readthedocs.io/en/latest/AccessToTheCluster.html)
identifies `/global-scratch/bulk_pool` as a production/data-processing area.
Current connectivity, retention/quota rules, remote checkout location and resource
availability must be checked live rather than inferred from the public guide.

## Verification status

Local SSH aliases, transfer helpers and an existing SU2 SGE script were inspected.
A read-only connection attempt failed while resolving
the configured SSH jump host, before reaching CFDHub. No remote path/queue inspection,
transfer, build, submission or monitoring has therefore been performed for this
branch. No runtime temporary-directory overrides were introduced.
