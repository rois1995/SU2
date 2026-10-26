"""Reuse diagnostic56's bounded runner for the production boundary-optimizer regression."""
import os
import sys
from pathlib import Path
ROOT = Path(__file__).resolve().parent
sys.path.insert(0, str(ROOT.parent / "diagnostic56"))
import offline

offline.ROOT = ROOT
os.environ["SU2_NATIVE_TRANSACTION_BOUNDARY_OPTIMIZER"] = "1"
if __name__ == "__main__":
    offline.main()
