"""Build the baseline test driver or both candidate targets for FP2-C1."""

from argparse import ArgumentParser
from pathlib import Path
import subprocess
import sys

HERE = Path(__file__).resolve().parent
NIGHT = HERE.parent
sys.path[:0] = [str(NIGHT / name) for name in ("FP1", "FX2")]
import build_fp1
from audit_fx2 import audit

S0 = "7e6953e721de0ebad3367a81f79c7d89efe74fbe"


def main():
    parser = ArgumentParser(description=__doc__)
    parser.add_argument("label", type=int)
    parser.add_argument("--baseline", action="store_true")
    args = parser.parse_args()
    audit()
    solver = build_fp1.BUILD / "SU2_CFD/src/SU2_CFD"
    if args.baseline:
        subprocess.run(["git", "diff", "--exit-code", S0, "--", "Common", "SU2_CFD"],
                       cwd=build_fp1.cut.REPO, check=True)
        reference = build_fp1.cut.read(NIGHT / "FX2/build645/build_status.json")
        assert build_fp1.sha(solver) == reference["binary_sha256"]
    return not build_fp1.build(HERE / f"build{args.label}", args.label, not args.baseline)


if __name__ == "__main__":
    raise SystemExit(main())
