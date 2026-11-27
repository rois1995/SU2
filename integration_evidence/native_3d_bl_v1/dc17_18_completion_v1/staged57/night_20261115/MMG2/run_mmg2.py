"""Launch the serial MMG2 capability run under the copied CL1 lifecycle."""
from pathlib import Path
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'CL2'))
from run_cl2 import main

if __name__ == '__main__':
    main('MMG2')
