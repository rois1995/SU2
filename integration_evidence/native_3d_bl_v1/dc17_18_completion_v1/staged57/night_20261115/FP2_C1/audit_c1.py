"""Verify recorded FP2-C process identities and live workloads on the host."""

from argparse import ArgumentParser
from datetime import datetime
import os
from pathlib import Path
import sys

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parent / "FX2"))
from audit_fx2 import audit as audit_fx2, cut, identity_rows


def audit(root=HERE):
    prior = audit_fx2()
    receipts, seen = [], set()
    kinds = ("launch", "wait", "status", "receipt", "process", "preflight", "measurement", "case_paths")
    for path in sorted(root.rglob("*.json")):
        if not any(kind in path.stem for kind in kinds):
            continue
        identities = list(identity_rows(cut.read(path)))
        if identities:
            receipts.append(str(path.relative_to(root)))
            seen.update((row["pid"], row["start_ticks"]) for row in identities)
    identities = [{"pid": pid, "start_ticks": ticks} for pid, ticks in sorted(seen)]
    active = [row for row in identities if not cut.terminal(row)]
    assert not active, active

    ancestors = set()
    pid = os.getpid()
    while pid > 1:
        ancestors.add(pid)
        pid = int(Path(f"/proc/{pid}/stat").read_text().rsplit(")", 1)[1].split()[1])
    marker = os.fsencode(f"/{root.name}/")
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
            match = any(marker in arg and (arg.endswith(b".py") or b"/G5/build" in arg) for arg in argv)
            if match or (path / "cwd").resolve(strict=True).is_relative_to(root):
                live.append(int(path.name))
        except OSError:
            continue
    assert not live, live
    return {"verified_at": datetime.now().astimezone().isoformat(), "actual_host_proc": True,
            "prior_fx2_identities_checked": len(prior["identities"]), "receipts": receipts,
            "all_owned_handles_terminal": True, "identities": identities, "unrecorded_live_matches": live}


if __name__ == "__main__":
    parser = ArgumentParser(description=__doc__)
    parser.add_argument("--record", type=Path)
    result = audit()
    args = parser.parse_args()
    if args.record:
        cut.save(args.record, result)
    print(f'{len(result["identities"])} FP2-C1 identities terminal. No live FP2-C1 workload matched.')
