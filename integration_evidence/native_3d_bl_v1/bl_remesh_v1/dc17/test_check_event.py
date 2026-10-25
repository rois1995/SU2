import copy
import json
from pathlib import Path
import tempfile
import unittest
from check_event import check


class ReceiptChecks(unittest.TestCase):
    def test_actual_rejected_c9_receipt(self):
        with self.assertRaisesRegex(ValueError, 'complete is not true'):
            check(Path(__file__).parent / 'chain_c9/event1/event1')

    def test_acceptance_and_artifacts_required(self):
        source = Path(__file__).parent / 'chain_c9/event0/event0_receipt.json'
        data = json.loads(source.read_text().replace(':inf', ':Infinity'))
        with tempfile.TemporaryDirectory() as directory:
            prefix = Path(directory) / 'event0'
            receipt = Path(str(prefix) + '_receipt.json')
            for suffix in ('.su2', '.su2.native_ref', '.su2.native_rows'):
                Path(str(prefix) + suffix).write_text('test artifact')
            receipt.write_text(json.dumps(data))
            check(prefix)
            for path, key in [((), 'complete'), ((), 'written'), (('construction',), 'accepted'),
                              (('construction', 'validity'), 'valid'),
                              (('construction', 'validity'), 'height_failures'),
                              (('construction', 'first_row_audit'), 'violations')]:
                bad = copy.deepcopy(data)
                obj = bad
                for part in path:
                    obj = obj[part]
                obj[key] = 1 if key in ('height_failures', 'violations') else 0
                receipt.write_text(json.dumps(bad))
                with self.assertRaises(ValueError):
                    check(prefix)
            receipt.write_text(json.dumps(data))
            Path(str(prefix) + '.su2.native_ref').unlink()
            with self.assertRaisesRegex(ValueError, 'missing or empty'):
                check(prefix)


if __name__ == '__main__':
    unittest.main()
