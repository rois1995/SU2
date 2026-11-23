"""Audit RCFD1 process handles with the original host observations preserved."""
from argparse import ArgumentParser
from pathlib import Path
import sys

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parent / 'FP2_C1'))

import audit_c1

BASE_ROWS = audit_c1.identity_rows
OBSERVATIONS = {'busy_processes_observed', 'other_busy_observations'}


def owned_values(value):
    if isinstance(value, list):
        return [owned_values(item) for item in value]
    if isinstance(value, dict):
        return {key: owned_values(item) for key, item in value.items() if key not in OBSERVATIONS}
    return value


def audit(root=HERE):
    audit_c1.identity_rows = lambda value: BASE_ROWS(owned_values(value))
    try:
        return audit_c1.audit(root)
    finally:
        audit_c1.identity_rows = BASE_ROWS


if __name__ == '__main__':
    parser = ArgumentParser(description=__doc__)
    parser.add_argument('--record', type=Path)
    args = parser.parse_args()
    result = audit()
    if args.record:
        audit_c1.cut.save(args.record, result)
    print(f"{len(result['identities'])} RCFD1 identities terminal. No live RCFD1 workload matched.")
