# DEF1 capability runs

Run CL2 after the committed DEF1 section-3 seal passes. Replace `681` below if the final section-3 build has another number. From the repository root:

```sh
N=integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115
standing="$N/DEF1/build681/section3.json"
nice -n 19 python3 -B "$N/CL2/run_cl2.py" prepare --standing "$standing"
nice -n 19 python3 -B "$N/CL2/run_cl2.py" launch --standing "$standing"
nice -n 19 python3 -B "$N/CL2/finish_cl2.py"
```

The launcher admits one six-rank run with a 21,600-second watchdog. It checks the reviewer units and uses the same host preflight, process ownership, signal escalation, reap and rank wait4 receipts as CL1. Run the postprocessor only after the launch process and all recorded handles have terminated. The solver configuration is derived from the sealed CL1 bytes. Its diff and input authentication are stored under `CL2/reports`. The output inventory gives the checksums of the files retained in `CL2/run`.

Commit the CL2 configuration, launch, terminal and validation JSON receipts, and CL2 reports before MMG2. The MMG2 launcher checks those committed files, the same section-3 seal and build, and the absence of a proven CL2 runtime-default defect. It derives MMG2 from the sealed CL2 configuration.

```sh
nice -n 19 python3 -B "$N/MMG2/run_mmg2.py" prepare --standing "$standing"
nice -n 19 python3 -B "$N/MMG2/run_mmg2.py" launch --standing "$standing"
nice -n 19 python3 -B "$N/MMG2/finish_mmg2.py"
```

Each validation JSON records `runtime_defaults.wiring_defect`. The report separates a runtime wiring defect from an observed capability result. Launch another solver only when R5 authorizes the one wiring correction. In that case, first commit the original evidence, pass the corrected section-3 seal to the launcher, and use `--repeat` on the affected run's commands to preserve the original in place. The repeat records the first check's fate under `runtime_repeat`. Use `--cl2-repeat` for both MMG2 launch commands if the corrected CL2 run supplies its prerequisite. The report files for repeats live below the affected run's `repeat/reports` directory.

The runners' `self-check` modes launch short sleep processes under MPI and exercise the receipt plumbing. The validator and report checks read existing or synthetic evidence and launch no solver.

```sh
python3 -B "$N/CL2/validate.py" --self-check
python3 -B "$N/CL2/report_cl2.py" --self-check
python3 -B "$N/MMG2/validate_mmg2.py"
```
