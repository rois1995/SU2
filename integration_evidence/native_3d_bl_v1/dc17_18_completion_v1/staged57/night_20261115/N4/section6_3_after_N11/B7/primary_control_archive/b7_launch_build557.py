#!/usr/bin/env python3
"""Record owner identity and exec the isolated B7 build wrapper."""
from pathlib import Path
import hashlib
import json
import os
import time

P = Path('/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated/integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/N4/section6_3_after_N11')
BUILD = P / "build557"


def main():
    """Exec one already prepared build without a shell."""
    if not (BUILD / "build.py").is_file():
        raise FileNotFoundError(BUILD / "build.py")
    stat = Path("/proc/self/stat").read_text().rsplit(")", 1)[1].split()
    argv = ["/usr/bin/python3", str(BUILD / "build.py")]
    identity = BUILD / "primary_launch_identity.json"
    if identity.exists():
        raise FileExistsError(identity)
    identity.write_text(json.dumps({
        "actual_host_proc": True,
        "unix": time.time(),
        "controller": {"pid": os.getpid(), "start_ticks": int(stat[19])},
        "argv": argv,
        "source_pins_sha256": hashlib.sha256((BUILD / "source_pins.json").read_bytes()).hexdigest()
            if (BUILD / "source_pins.json").is_file() else None,
        "adapter": "B7_build557",
        "launched": True
    }, indent=2) + "\n")
    os.environ.update(OMP_NUM_THREADS="1", OPENBLAS_NUM_THREADS="1", MKL_NUM_THREADS="1")
    os.nice(19)
    os.execv(argv[0], argv)


if __name__ == "__main__":
    main()
