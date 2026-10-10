"""Run the shared fake-file regression suite against the new comparison configuration."""
import copy
import importlib.util
import sys
import unittest
from unittest.mock import patch
from pathlib import Path
import prepare
import analyze
from run import runner

SHARED = prepare.ROOT / prepare.SHARED
spec = importlib.util.spec_from_file_location('reuse_checks', SHARED / 'check_package.py')
checks = importlib.util.module_from_spec(spec)
spec.loader.exec_module(checks)
checks.prepare, checks.analyze, checks.runner = prepare, analyze, runner
checks.REAL_ROOT = prepare.ROOT

class ReuseChecks(checks.PackageChecks):
    def test_preserved_control_cannot_escape_checkout(self):
        escape = self.root / 'escape'
        escape.symlink_to('/etc', target_is_directory=True)
        with patch.multiple(prepare, ROOT=self.root, CONTROL=escape/'native-control'):
            checks.rejects(prepare.preserve)
        with patch.multiple(prepare, ROOT=self.root, CHECKPOINT=escape/'native-checkpoint.json'):
            checks.rejects(prepare.prepare)

    def test_paired_profiles_and_work_counts(self):
        checks.write_profile(self.root, 2)
        balance = analyze.profile(self.root, 2)
        keys = ('native_frozen_adapted.su2',) + tuple(f'native_frozen_target_rank_{i}.csv' for i in range(4))
        pair = [dict(case=role, role=role, kind='case', workers=2, repartition='YES', repeat=1,
                     status='PASS', remesh_seconds=2 if role=='control' else 1,
                     output_sha256=dict.fromkeys(keys, 'identical'), balance=copy.deepcopy(balance))
                for role in ('control', 'profile')]
        pair[1]['balance'][0]['operations'][0]['evaluations'] = 1
        result = analyze.compare(pair)[0]
        self.assertTrue(result['operation_counts_identical'])
        self.assertEqual(result['remesh_change_percent'], -50)
        self.assertEqual(result['control_costs']['evaluations'], 4)
        self.assertEqual(result['candidate_costs']['evaluations'], 3)
        self.assertEqual(result['cost_change_percent']['evaluations'], -25)
        self.assertTrue(runner.PROFILE_CONTROL)
        self.assertIn('[NativeMesh2D]', runner.UNITS)
        pair[1]['balance'][0]['operations'][0]['selected'] += 1
        checks.rejects(lambda: analyze.compare(pair))
        del pair[0]['balance']
        checks.rejects(lambda: analyze.compare(pair), (KeyError,))


if __name__ == '__main__':
    unittest.main()
