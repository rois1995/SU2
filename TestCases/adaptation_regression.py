#!/usr/bin/env python3

## \file adaptation_regression.py
#  \brief Serial mesh-adaptation capability regressions (TestCases/adaptation/capability), judged by mesh and metric
#         gates. Needs SU2_CFD built with MMG (-Denable-mmg=true); for that reason it is not part of
#         serial_regression.py, whose CI binaries are built without MMG.
#  \version 8.5.0 "Harrier"
#
# SU2 Project Website: https://su2code.github.io
#
# The SU2 Project is maintained by the SU2 Foundation
# (http://su2foundation.org)
#
# Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md)
#
# SU2 is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# SU2 is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with SU2. If not, see <http://www.gnu.org/licenses/>.

import argparse
import os
import shutil
import subprocess
import sys


def main():
    parser = argparse.ArgumentParser(description="Serial adaptation capability regressions (SU2 with MMG).")
    parser.add_argument("--binary", help="SU2_CFD built with MMG (default: SU2_CFD on the PATH)")
    parser.add_argument("--output", default="adaptation_capability", help="output folder (default: %(default)s)")
    parser.add_argument("--cases", nargs="*", help="case names or tags of run_capability.py (default: all)")
    args = parser.parse_args()
    binary = args.binary or shutil.which("SU2_CFD")
    if not binary:
        sys.exit("SU2_CFD not found: give --binary or put it on the PATH.")
    runner = os.path.join(os.path.dirname(os.path.abspath(__file__)), "adaptation", "capability", "run_capability.py")
    command = [sys.executable, runner, "--binary", binary, "--output", args.output, "--ranks", "1"]
    if args.cases:
        command += ["--cases"] + args.cases
    sys.exit(subprocess.call(command))


if __name__ == "__main__":
    main()
