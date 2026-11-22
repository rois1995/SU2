"""Check FX2 process identities and live workload paths against the real host."""

from argparse import ArgumentParser
from datetime import datetime
import os
from pathlib import Path
import sys

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parent / "FP2_B_D1"))
from run_d1 import audit as audit_prior
from audit_b import cut, identity_rows


def audit():
    prior = audit_prior()
    receipts, seen = [], set()
    kinds = ("launch", "wait", "status", "receipt", "process", "preflight", "measurement")
    for path in sorted(HERE.rglob("*.json")):
        if not any(kind in path.stem for kind in kinds):
            continue
        identities = list(identity_rows(cut.read(path)))
        if identities:
            receipts.append(str(path.relative_to(HERE)))
            seen.update((row["pid"], row["start_ticks"]) for row in identities)
    rows = [{"pid": pid, "start_ticks": ticks} for pid, ticks in sorted(seen)]
    active = [row for row in rows if not cut.terminal(row)]
    assert not active, active

    ancestors = set()
    pid = os.getpid()
    while pid > 1:
        ancestors.add(pid)
        pid = int(Path(f"/proc/{pid}/stat").read_text().rsplit(")", 1)[1].split()[1])
    live = []
    for path in Path("/proc").iterdir():
        if not path.name.isdigit() or int(path.name) in ancestors:
            continue
        try:
            command = (path / "comm").read_text().strip()
            if command not in {"SU2_CFD", "test_driver", "mpirun", "orterun", "ninja", "cc1plus", "time", "timeout"} \
                    and not command.startswith("python"):
                continue
            argv = (path / "cmdline").read_bytes().split(b"\0")
            match = any(b"/FX2/" in arg and (arg.endswith(b".py") or b"/G5/build" in arg) for arg in argv)
            if match or (path / "cwd").resolve(strict=True).is_relative_to(HERE):
                live.append(int(path.name))
        except OSError:
            continue
    assert not live, live
    return {"verified_at": datetime.now().astimezone().isoformat(), "actual_host_proc": True,
            "prior_fp2_b_identities_checked": prior["prior_fp2_b_identities_checked"],
            "prior_d1_identities_checked": len(prior["identities"]), "receipts": receipts,
            "all_owned_handles_terminal": True, "identities": rows, "unrecorded_live_matches": live}


if __name__ == "__main__":
    parser = ArgumentParser(description=__doc__)
    parser.add_argument("--record", type=Path)
    args = parser.parse_args()
    result = audit()
    if args.record:
        cut.save(args.record, result)
    print(f'{len(result["identities"])} FX2 identities terminal. No live FX2 workloads matched.')
