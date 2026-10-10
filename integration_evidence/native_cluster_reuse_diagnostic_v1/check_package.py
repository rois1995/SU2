"""Fake-file diagnostic regressions; no build, MPI, solver or scheduler execution."""
import contextlib
import importlib.util
import io
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch
import prepare
import analyze
from run import runner

spec = importlib.util.spec_from_file_location('reuse_shared_checks', prepare.ROOT / prepare.SHARED / 'check_package.py')
shared = importlib.util.module_from_spec(spec);spec.loader.exec_module(shared)
REAL_ROOT = prepare.ROOT


class DiagnosticChecks(unittest.TestCase):
    def test_preserve_checkpoint_and_closed_paths(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary);pack = root / 'package';pack.mkdir()
            control = root / 'control';control.write_bytes(b'control')
            candidate = root / 'candidate';candidate.write_bytes(b'previous')
            archived = root / 'archived';checkpoint = root / 'checkpoint.json'
            expected = prepare.sha(candidate)
            (pack / 'control.json').write_text(json.dumps(dict(test_driver_sha256=prepare.sha(control))))
            metadata = root / 'build-native/meson-info/intro-buildoptions.json';metadata.parent.mkdir(parents=True);metadata.write_text('[]')
            with patch.multiple(prepare, ROOT=root, PACK=pack, CONTROL=control, CANDIDATE=candidate,
                                ARCHIVED=archived, ARCHIVED_HASH=expected, CHECKPOINT=checkpoint), \
                 patch.multiple(prepare.base, ROOT=root, PACK=pack, CONTROL=control, CANDIDATE=candidate), \
                 patch.object(prepare, 'pins', return_value={'source':'same'}), contextlib.redirect_stdout(io.StringIO()):
                prepare.preserve();prepare.preserve()
                self.assertEqual(archived.read_bytes(), b'previous')
                shared.rejects(prepare.prepare)
                candidate.write_bytes(b'diagnostic');prepare.prepare()
                record = json.loads(checkpoint.read_text());prepare.verify(record)
                shared.rejects(prepare.prepare)
                with patch.object(prepare, 'pins', return_value={'source':'changed'}): shared.rejects(lambda:prepare.verify(record))
                archived.write_bytes(b'changed');shared.rejects(prepare.preserve)
                escape = root / 'escape';escape.symlink_to('/etc/passwd');shared.rejects(lambda:prepare.closed(escape))

    def campaign(self, unit_failure=False, matrix=False, changed=False):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary);pack = root / 'package';pack.mkdir()
            for name in runner.TOOLS:
                dest = root / name;dest.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(REAL_ROOT / name, dest)
            for kind in runner.KINDS:
                fixture = root / 'integration_evidence/native_cluster_campaign_v1/inputs' / kind;fixture.mkdir(parents=True)
                for name in ('input.su2','frozen_sensor.csv','frozen_sensor_source_flow.vtu'): (fixture / name).write_text(name)
                (fixture / 'run.cfg').write_text('MESH_FILENAME= input.su2\n')
            binaries = {}
            for role in ('control','archived','profile'):
                p = root / ('build-native/'+role+'/test_driver');p.parent.mkdir(parents=True);p.write_text(role)
                binaries[role] = dict(path=str(p.relative_to(root)),sha256=prepare.sha(p))
            checkpoint = root / 'checkpoint.json';checkpoint.write_text(json.dumps(dict(binaries=binaries)))
            machine = root / 'machinefile';machine.write_text('localhost slots=4\n')
            calls = []
            def execute(command, case, environment, timeout):
                n = int(command[2]);frozen = command[-3]=='[NativeFrozenAirfoil2D]'
                calls.append((n,frozen));log = 'All tests passed\n'*n
                if frozen:
                    import re
                    workers = int(re.search(r'ADAP_NATIVE_RANKS= (\d+)',(case/'run.cfg').read_text())[1])
                    mode = environment['SU2_NATIVE_REUSE'];audit = environment['SU2_NATIVE_REUSE_AUDIT']
                    log += 'Native reuse diagnostic mode: '+mode+'; audit='+audit+'\n'
                    if audit=='YES': log += 'Native reuse audit metric checks/mismatches score checks/mismatches: 8 1 8 0; maximum relative: 1e-15 0\n'
                    for label in ('selection','round protocol','round dependency import','round donor import and IDs',
                                  'round reconstruction','round validation','round commit','private reconstruction','adapt unclassified'):
                        log += 'Native rank cost '+label+' seconds min/mean/max: 1 1 1\n'
                    log += 'Native rank cost engine adapt seconds min/mean/max: 8 8 8\nNative operations (same order): '+str(workers)+' 0 0 0 0 0 0 0; conflicts=0\n'
                    output = ('changed' if changed and mode=='BOTH' else 'identical') if matrix else mode
                    (case / 'native_frozen_adapted.su2').write_text(output)
                    for rank in range(4):
                        (case / ('native_frozen_target_rank_'+str(rank)+'.csv')).write_text(output)
                        (case / ('native_frozen_timing_rank_'+str(rank)+'.csv')).write_text('remesh_max_seconds\n2\n')
                    shared.write_profile(case,workers)
                (case / 'solver.log').write_text(log)
                (case / 'discard.vtk').write_text('discard')
                return dict(exit_code=int(unit_failure and n==2),wall_seconds=2,command=command,machine_samples=[])
            def external(command, **kwargs):
                if command[0]==sys.executable: (Path(command[2]) / 'independent_frozen_metric_audit.json').write_text('{"status":"PASS"}')
                return subprocess.CompletedProcess(command,0,stdout='',stderr='')
            with patch.multiple(runner, ROOT=root, PACK=pack, CHECKPOINT=checkpoint), patch.object(prepare.base,'ROOT',root), \
                 patch.object(runner,'verify'), patch.object(runner,'execute',side_effect=execute), \
                 patch.object(runner.subprocess,'run',side_effect=external), \
                 patch.dict(os.environ,NSLOTS='4',JOB_ID='123'), \
                 patch.object(sys,'argv',['run.py','--machinefile',str(machine)]), contextlib.redirect_stdout(io.StringIO()):
                code = runner.main()
            out = root / ('ClusterResults/'+runner.RESULT_PREFIX+'_123');record = json.loads((out / 'validation.json').read_text())
            self.assertFalse((out / 'objects').exists())
            self.assertFalse(any(p.is_symlink() for p in out.rglob('*')))
            if unit_failure:
                self.assertEqual(code,1);self.assertEqual(record['status'],'FAIL');self.assertEqual(record['cases'],[])
            elif changed:
                self.assertEqual(code,1);self.assertEqual(record['status'],'FAIL');self.assertEqual(len(record['cases']),2)
                self.assertFalse(record['pairs'][0]['numerical_outputs_identical'])
            else:
                self.assertEqual(code,0);self.assertEqual(record['status'],'PASS' if matrix else 'DIAGNOSTIC_COMPLETE')
                self.assertEqual(len(record['cases']),16 if matrix else 7);self.assertEqual(len(record['pairs']),8 if matrix else 6)
                self.assertEqual([n for n,frozen in calls[:3]],[1,2,4])
                self.assertEqual([r['role'] for r in record['cases']],list(runner.ROLES)*(8 if matrix else 1))
                if matrix: self.assertTrue(all(p['numerical_outputs_identical'] for p in record['pairs']))
                else:
                    self.assertTrue(any(not p['numerical_outputs_identical'] for p in record['pairs']))
                    audit = record['cases'][-1]['diagnostic'];self.assertFalse(audit['sampled_hits_bit_identical'])
                self.assertFalse(list(out.rglob('discard.vtk')))
                for row in record['cases']:
                    evidence = json.loads((out / 'cases' / row['case'] / 'run_evidence.json').read_text())
                    self.assertIn('diagnostic_environment',evidence)
                    self.assertEqual(evidence['binary_sha256'],binaries[runner.ROLE_BINARIES.get(row['role'],row['role'])]['sha256'])
                shared.rejects(lambda:runner.compare(record['cases'][:-1]))
                self.assertTrue(all(p['operation_counts_identical'] for p in record['pairs']))
                record['cases'][2]['balance'][0]['operations'][0]['selected'] += 1
                if matrix: shared.rejects(lambda:runner.compare(record['cases']))
                else: self.assertFalse(analyze.compare(record['cases'])[0]['operation_counts_identical'])

    def test_complete_diagnostic_reports_differences(self): self.campaign()
    def test_failed_units_stop_real_cases(self): self.campaign(True)
    def matrix_campaign(self, changed=False):
        import matrix_run
        names = ('CHECKPOINT','RESULT_PREFIX','KINDS','VARIANTS','ROLES','ROLE_BINARIES',
                 'ROLE_ENVIRONMENTS','CASE_DETAILS','compare','REQUIRE_IDENTITY','SCOPE')
        with patch.multiple(runner, **{name:getattr(runner,name) for name in names}):
            matrix_run.configure()
            self.campaign(matrix=True,changed=changed)

    def test_strict_matrix_completes_all_pairs(self): self.matrix_campaign()
    def test_strict_matrix_stops_at_output_difference(self): self.matrix_campaign(True)

    def test_matrix_build_and_source_guards(self):
        import matrix_prepare
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary);binary = root/'candidate';binary.write_bytes(b'tested')
            metadata = root/'build-native/meson-info/intro-buildoptions.json';metadata.parent.mkdir(parents=True);metadata.write_text('[]')
            digest = prepare.sha(binary)
            record = dict(binaries={'profile':{'sha256':digest}},files_sha256={'Common/native.hpp':'same'},build_options=[])
            with patch.multiple(prepare,ROOT=root,CANDIDATE=binary),patch.object(prepare.base,'ROOT',root), \
                 patch.object(matrix_prepare,'EXPECTED',digest),patch.object(prepare,'pins',return_value=record['files_sha256']):
                matrix_prepare.check_build(record)
                with patch.object(prepare,'pins',return_value={'Common/native.hpp':'changed'}):shared.rejects(lambda:matrix_prepare.check_build(record))
                with patch.object(prepare,'pins',return_value={'Common/native.hpp':'same','Common/new.hpp':'new'}):shared.rejects(lambda:matrix_prepare.check_build(record))
                metadata.write_text('[1]');shared.rejects(lambda:matrix_prepare.check_build(record));metadata.write_text('[]')
                binary.write_bytes(b'rebuilt');shared.rejects(lambda:matrix_prepare.check_build(record))

    def test_original_gate_still_defaults_to_strict(self):
        source = (REAL_ROOT / prepare.SHARED / 'run.py').read_text()
        self.assertIn('REQUIRE_IDENTITY = True',source)
        self.assertIn("'PASS' if REQUIRE_IDENTITY else 'DIAGNOSTIC_COMPLETE'",source)


if __name__ == '__main__': unittest.main()
