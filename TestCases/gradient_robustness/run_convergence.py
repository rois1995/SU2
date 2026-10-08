#!/usr/bin/env python3
"""Run an explicit, serial MGLEVEL=0 convergence experiment plan in fresh directories."""
import argparse
import csv
import json
import math
import os
from pathlib import Path
import re
import subprocess
import time

from run_fixes import configure, digest


def read_options(text):
    return dict(re.findall(r"^\s*([A-Z][A-Z0-9_]*)\s*=\s*([^%\n]*)", text, re.MULTILINE))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("plan", type=Path, help="JSON list: name, binary, config, options")
    parser.add_argument("output", type=Path)
    parser.add_argument("--timeout", type=int, default=1200)
    args = parser.parse_args()
    plan = json.loads(args.plan.read_text())
    if args.timeout <= 0 or not plan or len({case["name"] for case in plan}) != len(plan):
        parser.error("Require a positive timeout and uniquely named experiments")
    # Validate the entire plan before starting a solver.
    prepared = []
    for case in plan:
        if not re.fullmatch(r"[A-Za-z0-9_-]+", case["name"]):
            parser.error("Case names must be simple directory names")
        binary = Path(case["binary"]).resolve(strict=True)
        template = Path(case["config"]).resolve(strict=True)
        if not binary.is_file() or not os.access(binary, os.X_OK):
            parser.error("Solver must be executable")
        text = configure(template.read_text(), case["options"])
        opts = {key: value.strip() for key, value in read_options(text).items()}
        if (opts.get("MGLEVEL") != "0" or opts.get("TIME_DOMAIN", "NO") != "NO"
                or opts.get("TIME_MARCHING", "NO") != "NO" or opts.get("ADAP_LOOP", "NO") != "NO"):
            parser.error("These experiments require steady fixed meshes and MGLEVEL=0")
        mesh = Path(opts["MESH_FILENAME"]).resolve(strict=True)
        inputs = {"mesh": {"path": str(mesh), "sha256": digest(mesh)}}
        if opts.get("RESTART_SOL") == "YES":
            restart = Path(opts["SOLUTION_FILENAME"] + (".dat" if opts.get("READ_BINARY_RESTART", "YES") == "YES" else ".csv"))
            inputs["restart"] = {"path": str(restart), "sha256": digest(restart)}
        prepared.append((case, binary, text, inputs))
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    results = []
    env = dict(os.environ, OMP_NUM_THREADS="1", OPENBLAS_NUM_THREADS="1")
    for case, binary, text, inputs in prepared:
        directory = output / case["name"]
        directory.mkdir()
        (directory / "case.cfg").write_text(text)
        record = dict(case, binary_sha256=digest(binary), inputs=inputs,
                      config=text, config_sha256=digest(directory / "case.cfg"), threads=1, nice=19)
        start = time.monotonic()
        with (directory / "run.log").open("w") as log:
            try:
                run = subprocess.run(["nice", "-n", "19", str(binary), "case.cfg"], cwd=directory,
                                     env=env, stdout=log, stderr=subprocess.STDOUT, timeout=args.timeout)
                record.update(exit_code=run.returncode, status="finished" if run.returncode == 0 else "failed")
            except subprocess.TimeoutExpired:
                record.update(exit_code=None, status="timeout")
        record["wall_seconds"] = time.monotonic() - start
        rows = []
        if (directory / "history.csv").exists():
            with (directory / "history.csv").open() as history:
                rows = [{key.strip().strip('"'): float(value) for key, value in row.items()}
                        for row in csv.DictReader(history)]
        record["history_rows"] = len(rows)
        record["last_history"] = {key: value if math.isfinite(value) else None
                                  for key, value in rows[-1].items()} if rows else {}
        residuals = [value for row in rows for key, value in row.items() if key.startswith("rms[")]
        record["finite_residuals"] = bool(residuals) and all(math.isfinite(value) for value in residuals)
        if record["status"] == "finished" and not record["finite_residuals"]:
            record["status"] = "invalid_history"
        results.append(record)
        (output / "results.json").write_text(json.dumps(results, indent=2, allow_nan=False) + "\n")
        print(case["name"], record["status"], len(rows), record["last_history"], flush=True)


if __name__ == "__main__":
    main()
