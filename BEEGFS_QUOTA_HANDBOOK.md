# BeeGFS disk-quota investigation handoff

Updated: 2026-10-11, Europe/Rome. This is a separate storage investigation; it does not change the native mesh-adaptation goal.

## Start here

The cluster compiler genuinely received quota/storage errors while writing build outputs. The reported user and group quotas, physical capacity, and target health do **not** explain those errors at the sampled times. Root cause is unresolved. Cached enforcement state is a plausible lead, not an established diagnosis.

The confirmed failing build directory is **build-native-3d-20261011_213938**, not the earlier build-native-3d-20261011_212000 used in one diagnostic lookup. The actual object-output directory was subsequently confirmed to use **storage pool 1 (Default)**. Do not attribute this failure to the almost-full group quota in pool 2 without server-side evidence.

The user wants to continue this investigation in a new Codex session and keep it separate from adaptation development. Do not run cluster jobs, contact administrators, delete files, or change filesystem policies without authorization. No cluster commands were executed remotely by this Codex session; cluster evidence below was supplied by the user.

## Environment and exact failing command

- User: arausa, UID 1050.
- Directory group: aero, GID 520.
- Cluster checkout: /global-scratch/bulk_pool/arausa/SU2_Native.
- Scheduler: SGE/qsub, not Slurm.
- Git on login environment: 1.8.3.1; compute nodes do not have git.
- Local development checkout: /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated.
- Development branch: codex/native-3d-transport-accounting. The quota documentation checkpoint before this handoff was 78291a634957f982bbaaf8d394ed91b91ea650db.
- Installed BeeGFS version, server settings, and underlying storage filesystem quotas are unknown.

```bash
NATIVE3D_BUILD=build-native-3d-20261011_213938
./ninja -C "$NATIVE3D_BUILD" -j2 UnitTests/test_driver
```

Despite requesting the test driver, its dependency graph builds SU2 core and dependencies: the reported build had 926 steps. It reached steps 672/926 and 673/926 before failing. Parallelism was already limited to two jobs.

## Failure evidence

At 2026-10-11 21:47:56 CEST the user reported these failures:

```text
FAILED: SU2_CFD/src/libSU2core.a.p/output_CAdjHeatOutput.cpp.o
fatal error: closing dependency file
SU2_CFD/src/libSU2core.a.p/output_CAdjHeatOutput.cpp.o.d:
Quota disco superata
Assembler: can't write 27 bytes to section .text ... Spazio esaurito sul device
Assembler: can't close ... Spazio esaurito sul device

FAILED: SU2_CFD/src/libSU2core.a.p/output_CBaselineOutput.cpp.o
fatal error: closing dependency file
SU2_CFD/src/libSU2core.a.p/output_CBaselineOutput.cpp.o.d:
Quota disco superata
Assembler: can't write 1 bytes to section ... Quota disco superata
Assembler: can't close ... Spazio esaurito sul device
ninja: build stopped: subcommand failed.
```

“Quota disco superata” indicates EDQUOT; “Spazio esaurito sul device” indicates ENOSPC. Both appeared in the same failed build. The preceding HDF5 warnings about discarding const qualifiers are unrelated to quota enforcement.

The user reports temporary inability to use the cluster after these incidents, and recovery after about ten minutes. When genuinely exceeding quota, freeing space also takes about ten minutes to restore access. This is a reported access problem; no evidence establishes that the entire cluster crashed.

## Quota snapshots supplied by the user

All displayed chunk-file hard limits were unlimited.

| Time (CEST, 2026-10-11) | Scope | Pool | Used | Hard limit | Chunk files |
| --- | --- | --- | --- | --- | --- |
| 21:38:22 | arausa / UID 1050 | Default / 1 | 252.06 GiB | 300.00 GiB | 1,456,769 |
| 21:38:22 | arausa / UID 1050 | flash / 2 | 0 Byte | 10.00 GiB | 0 |
| 21:38:22 | aero / GID 520 | Default / 1 | 5.15 TiB | 10.00 TiB | 8,993,164 |
| 21:38:22 | aero / GID 520 | flash / 2 | 799.87 GiB | 800.00 GiB | 1,128,222 |
| 21:47:56 | arausa / UID 1050 | Default / 1 | 252.88 GiB | 300.00 GiB | 1,458,391 |
| 21:47:56 | arausa / UID 1050 | flash / 2 | 0 Byte | 10.00 GiB | 0 |
| 21:47:56 | aero / GID 520 | Default / 1 | 5.15 TiB | 10.00 TiB | 8,994,786 |
| 21:47:56 | aero / GID 520 | flash / 2 | 799.87 GiB | 800.00 GiB | 1,128,222 |

These came from:

```bash
date
stat -c '%n owner=%U group=%G gid=%g' .
beegfs-ctl --getquota --uid arausa
beegfs-ctl --getquota --gid "$(stat -c %g .)"
```

At the second snapshot the user had approximately 47.12 GiB of reported Default-pool headroom. The increase between snapshots was about 0.82 GiB. The shared aero flash quota had about 0.13 GiB (133 MiB) of headroom, based on rounded output. No snapshot shows a displayed quota being exceeded. These snapshots cannot exclude an earlier transient exceedance or a discrepancy in enforcement state.

## Confirmed placement, capacity, and health

The checkout and the earlier build-native-3d-20261011_212000 directory both reported RAID0, 512K chunks, desired four targets, pool 1. A lookup of its SU2 object directory failed with “No such file or directory”; that was the wrong build path for the supplied Ninja failure.

The user then confirmed:

```text
NATIVE3D_BUILD=<build-native-3d-20261011_213938>
beegfs-ctl --getentryinfo "$NATIVE3D_BUILD/SU2_CFD/src/libSU2core.a.p"
Entry type: directory
EntryID: 6D2-6ACBE5C9-1
Metadata node: bee01 [ID: 1]
Type: RAID0
Chunksize: 512K
Number of storage targets: desired: 4
Storage Pool: 1 (Default)
```

Pool mapping from --liststoragepools:

- Default / 1: targets 101, 102, 203, 204.
- flash / 2: targets 301–306 and 401–406.

The supplied beegfs-df snapshot showed:

- Metadata target 1: 374.5 GiB total, 278.5 GiB free; 94.8 million free inodes out of 262.1 million.
- Default targets: each 25,598 GiB total, approximately 9,854–9,862 GiB free; about 99% of inodes free.
- Flash targets: each 1,432.8 GiB total, approximately 1,152–1,163 GiB free; about 99% of inodes free.
- --listtargets --longnodes --state: every listed target Online / Good.

This excludes simple physical exhaustion or offline targets **at the sampled time**, not necessarily at the exact failed write. Directory placement is established; inspect any surviving failed .o or .o.d file separately to confirm its actual file stripe targets/pool. Existing file placement need not be identical to the current directory pattern.

## What the documentation and source establish

[BeeGFS 7.4.7 quota documentation](https://doc.beegfs.io/7.4.7/advanced_topics/quota.html) describes per-user and per-group quotas, per-pool reporting, and periodic enforcement updates. The documented default quotaUpdateIntervalMin is ten minutes. Management gathers usage and distributes exceeded-quota information; a live --getquota report is not proof that every storage target's enforcement state has already refreshed.

The [7.4.7 storage write implementation](https://github.com/ThinkParQ/beegfs/blob/7.4.7/storage/source/net/message/session/rw/WriteLocalFileMsgEx.cpp) checks a target-specific exceeded-quota store using the write request's user ID, group ID, and size-limit type when quota enforcement is enabled. That path can return DQUOT. This supports checking the failed target's cached enforcement state, but does not establish which condition caused this incident or whether the installed version behaves identically.

[Storage-pool documentation](https://doc.beegfs.io/7.4.7/advanced_topics/storage_pools.html) explains directory patterns and file placement. [Target-management documentation](https://doc.beegfs.io/7.4.7/advanced_topics/target_management.html) covers target capacity and state inspection.

Important distinctions:

- User quota is not the only applicable quota; group quota must also be checked.
- Physical free space is not quota headroom.
- Displayed unlimited chunk-file limits do not explain the failure through the reported chunk counts.
- The shared flash group being close to its limit is a lead only. The confirmed failed output directory is in Default.
- Ten-minute recovery is consistent with periodic enforcement refresh, but not sufficient to prove that explanation.
- A successful MMG link configuration and a disk write failure are separate events; no causal link has been established.

## Next checks: small and read-only

Run these from the cluster checkout, preferably while the error is occurring. Keep outputs with timestamps. Do not scan the entire shared filesystem or launch another compilation merely to reproduce the failure.

```bash
pwd
printf 'NATIVE3D_BUILD=<%s>\n' "${NATIVE3D_BUILD:-UNSET}"
date
stat -c '%n owner=%U uid=%u group=%G gid=%g' . "$NATIVE3D_BUILD/SU2_CFD/src/libSU2core.a.p"
beegfs-ctl --getentryinfo "$NATIVE3D_BUILD/SU2_CFD/src/libSU2core.a.p"
beegfs-ctl --getquota --uid arausa
beegfs-ctl --getquota --gid 520
beegfs-df
beegfs-ctl --listtargets --longnodes --state
```

If the partial object or dependency file exists, inspect its entry information too:

```bash
beegfs-ctl --getentryinfo "$NATIVE3D_BUILD/SU2_CFD/src/libSU2core.a.p/output_CAdjHeatOutput.cpp.o"
beegfs-ctl --getentryinfo "$NATIVE3D_BUILD/SU2_CFD/src/libSU2core.a.p/output_CAdjHeatOutput.cpp.o.d"
```

A missing partial file is unsurprising after a failed compile. Do not substitute another timestamped build silently. Obtain the installed BeeGFS version using the site's supported package/version command; no version was captured yet. No --per-target quota option was verified, so do not invent one.

## Administrator handoff, if needed

Suggested text to provide manually; this session has not contacted anyone:

> On 11 October 2026 around 21:47:56 CEST, arausa (UID 1050, group aero GID 520) received EDQUOT and ENOSPC compiling in /global-scratch/bulk_pool/arausa/SU2_Native/build-native-3d-20261011_213938/SU2_CFD/src/libSU2core.a.p. This directory reports Default pool 1; its EntryID is 6D2-6ACBE5C9-1. Live quota reporting showed user 252.88/300 GiB and group 5.15/10 TiB for Default, with unlimited chunk-file limits. All targets later reported Online/Good and ample capacity/inodes. Access recovered after roughly ten minutes. Please check the failed writes' actual targets, storage/management logs and cached exceeded-quota user/group lists, quota refresh settings, and any underlying filesystem quota or allocation error. The shared group's flash pool was 799.87/800 GiB, but no evidence connects that separate pool to the Default writes.

Only administrators can establish server-side enforcement state from this evidence. Avoid declaring a BeeGFS bug or asking to disable quotas before that diagnosis.

## Preserve the partial build; do not multiply build trees

Keep the current build and its meson-logs/meson-log.txt, configuration, and failure output. After storage access recovers, resume the **same** Ninja command. Already-built valid objects can be reused; verify completion before claiming that the build or test driver is qualified.

Moving TMPDIR alone does not relocate the .o/.o.d outputs shown in the error. Lowering -j2 to -j1 may reduce transient demand but cannot correct stale enforcement state or an actual quota limit. Fresh timestamped build directories consume more space and are not a quota fix. No deletion, pool migration, or cleanup was performed for this investigation.

A fresh build was previously recommended for a **separate** corrupted Meson configuration (KeyError: 'options'); that is not a reason to discard this now-partially-compiled build.

## Earlier MMG configuration issue, already separated from quota

MMG prefix supplied by the user:

```text
/home/aero/arausa/global/Libs/mmg-5.8.0/install
include/mmg/libmmg.h
lib64/libmmg.a
lib64/libmmg2d.a
lib64/libmmg3d.a
lib64/libmmgs.a
```

The branch's lib64 discovery fix allowed “Library mmg found: YES”. The subsequent link probe failed on SCOTCH symbols because this static MMG build requires SCOTCH. The user's ldd of mmg2d_O3 located:

```text
/global-scratch/bulk_pool/aero/utils_gcc10/SCOTCH_INSTALL/lib64/libscotch.so.7.0
/global-scratch/bulk_pool/aero/utils_gcc10/SCOTCH_INSTALL/lib64/libscotcherrexit.so.7.0
```

That identifies the prefix for -Dmmg_scotch_root, alongside -Dmmg_root for MMG. Runtime library paths must be available on compute nodes. The later 926-step compilation demonstrates progress beyond configuration, but no completed cluster build/link/test run is certified by the supplied logs. Installation --prefix="$(pwd)" is separate from the Ninja build directory and TMPDIR; it does not itself explain these object-write errors.

## Prompt for the new Codex session

> Read /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated/BEEGFS_QUOTA_HANDBOOK.md. Investigate the intermittent BeeGFS EDQUOT/ENOSPC incident separately from SU2 adaptation development. The actual failed build is build-native-3d-20261011_213938, and its object directory is confirmed in Default pool 1. Reported quotas, physical capacity, and target health were below limits/healthy at the snapshots. Diagnose what evidence is still needed, especially target-specific cached enforcement versus live quota usage. Preserve the partial build and use small read-only checks. Do not attribute this to the shared flash quota without evidence, and do not launch jobs, delete files, or contact administrators without authorization.

Unverified: installed BeeGFS version/settings, server enforcement state at the failed writes, actual surviving-file stripe targets, and cluster build completion. Root cause remains open.
