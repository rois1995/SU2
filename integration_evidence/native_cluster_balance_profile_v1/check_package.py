"""Fast fake-file checks; never compiles or invokes SU2/MPI/qsub."""
import contextlib
import csv
import errno
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

PACK = Path(__file__).resolve().parent
sys.path.insert(0, str(PACK))
import prepare
import analyze
spec=importlib.util.spec_from_file_location('balance_runner',PACK/'run.py')
runner=importlib.util.module_from_spec(spec);spec.loader.exec_module(runner)
REAL_ROOT=prepare.ROOT


def rejects(call, types=(AssertionError, ValueError, FileNotFoundError, FileExistsError)):
    try:call()
    except types:return
    raise AssertionError('Invalid input was accepted')


def write_profile(case, workers):
    op_keys='rank workers action selected attempts reconstructed committed private_wall_seconds private_cpu_seconds cpu_samples requests evaluations evictions cells max_cells longest_seconds phase_seconds selection_seconds selection_scans'.split()
    hot_keys='rank workers round action coordinated seed_a seed_b cells boundary_cells reconstructed committed private_wall_seconds private_cpu_seconds requests evaluations evictions xmin ymin xmax ymax'.split()
    for rank in range(workers):
        prefix='native_balance_1_rank_'+str(rank)
        with (case/(prefix+'_operations.csv')).open('w') as handle:
            writer=csv.DictWriter(handle,fieldnames=op_keys);writer.writeheader()
            for action in range(8):
                row=dict.fromkeys(op_keys,0);row.update(rank=rank,workers=workers,action=action)
                if action==0:row.update(selected=1,attempts=1,reconstructed=1,committed=1,private_wall_seconds=1,
                    private_cpu_seconds=.5,cpu_samples=1,requests=4,evaluations=2,cells=3,max_cells=3,longest_seconds=1)
                writer.writerow(row)
        with (case/(prefix+'_hotspots.csv')).open('w') as handle:
            writer=csv.DictWriter(handle,fieldnames=hot_keys);writer.writeheader()
            row=dict.fromkeys(hot_keys,0);row.update(rank=rank,workers=workers,round=1,seed_a=20,seed_b=21,cells=3,
                boundary_cells=1,reconstructed=1,committed=1,private_wall_seconds=1,private_cpu_seconds=.5,
                requests=4,evaluations=2,xmax=1,ymax=1);writer.writerow(row)


class PackageChecks(unittest.TestCase):
    def setUp(self):
        self.temporary=tempfile.TemporaryDirectory(prefix='native-balance-package-')
        self.root=Path(self.temporary.name)
    def tearDown(self):self.temporary.cleanup()

    def test_preserve_and_checkpoint_guards(self):
        pack=self.root/'integration_evidence/native_cluster_balance_profile_v1';pack.mkdir(parents=True)
        control=self.root/'build-native/balance-control/test_driver'
        candidate=self.root/'build-native/UnitTests/test_driver';candidate.parent.mkdir(parents=True);candidate.write_bytes(b'validated')
        expected=prepare.sha(candidate);(pack/'control.json').write_text(json.dumps(dict(test_driver_sha256=expected)))
        metadata=self.root/'build-native/meson-info/intro-buildoptions.json';metadata.parent.mkdir();metadata.write_text('[]')
        checkpoint=self.root/'build-native/balance_profile_checkpoint.json'
        with patch.multiple(prepare, ROOT=self.root,PACK=pack,CONTROL=control,CANDIDATE=candidate,CHECKPOINT=checkpoint),patch.object(prepare,'pins',return_value={'fixture':'same'}),contextlib.redirect_stdout(io.StringIO()):
            prepare.preserve();prepare.preserve()
            rejects(prepare.prepare)
            candidate.write_bytes(b'profile');prepare.prepare()
            record=json.loads(checkpoint.read_text());prepare.verify(record)
            rejects(prepare.prepare)
            with patch.object(prepare,'pins',return_value={'fixture':'changed'}):rejects(lambda:prepare.verify(record))
            candidate.write_bytes(b'stale');rejects(lambda:prepare.verify(record))
            control.write_bytes(b'wrong');rejects(prepare.preserve)
            escape=self.root/'external';escape.symlink_to('/etc/passwd');rejects(lambda:prepare.closed(escape))

    def test_profile_csv_and_pair_rejection(self):
        write_profile(self.root,2)
        rows=analyze.profile(self.root,2)
        self.assertEqual([r['attempts'] for r in rows],[1,1])
        rejects(lambda:analyze.profile(self.root,3))
        p=self.root/'native_balance_1_rank_0_operations.csv'
        text=p.read_text();p.write_text(text.replace('1,0.5','nan,0.5'));rejects(lambda:analyze.profile(self.root,2));p.write_text(text)
        p=self.root/'native_balance_1_rank_1_hotspots.csv';p.write_text(p.read_text().splitlines()[0]+'\n')
        rejects(lambda:analyze.profile(self.root,2))
        keys=('native_frozen_adapted.su2',)+tuple(f'native_frozen_target_rank_{i}.csv' for i in range(4))
        pair=[dict(case=role,role=role,kind='kind',workers=2,repartition='YES',repeat=1,status='PASS',
                   remesh_seconds=2,output_sha256=dict.fromkeys(keys,'hash'),balance=rows) for role in ('control','profile')]
        self.assertTrue(analyze.compare(pair)[0]['numerical_outputs_identical'])
        pair[1]['output_sha256'][keys[0]]='different'
        self.assertFalse(analyze.compare(pair)[0]['numerical_outputs_identical'])
        pair[0]['status']='FAIL';rejects(lambda:analyze.compare(pair))

    def test_export_deduplicates_and_never_deletes_unverified_work(self):
        fixture=self.root/'fixture';fixture.mkdir();objects=self.root/'objects';objects.mkdir()
        for name in ('input.su2','frozen_sensor.csv','frozen_sensor_source_flow.vtu'):(fixture/name).write_text(name)
        def work(name):
            case=self.root/name;case.mkdir()
            for p in fixture.iterdir():shutil.copy2(p,case/p.name)
            (case/'run.cfg').write_text('cfg');(case/'solver.log').write_text('log')
            (case/'native_frozen_adapted.su2').write_text('mesh');(case/'large_unused.vtu').write_text('prune')
            return case
        case=work('first');dest=self.root/'export1'
        first=runner.export_and_clean(case,dest,fixture,objects)
        self.assertFalse(case.exists());self.assertFalse((dest/'large_unused.vtu').exists())
        case=work('second');second=self.root/'export2';runner.export_and_clean(case,second,fixture,objects)
        for name,info in first.items():
            self.assertEqual(prepare.sha(dest/name),info['sha256'])
            self.assertEqual((dest/name).stat().st_ino,(second/name).stat().st_ino)
        before=(dest/'solver.log').read_bytes();case=work('third')
        rejects(lambda:runner.export_and_clean(case,dest,fixture,objects))
        self.assertTrue(case.exists());self.assertEqual((dest/'solver.log').read_bytes(),before)
        with patch.object(runner,'compact',side_effect=OSError('disk full')):
            rejects(lambda:runner.export_and_clean(case,self.root/'broken',fixture,objects),(OSError,))
        self.assertTrue(case.exists())
        with patch.object(runner.os,'link',side_effect=OSError(errno.EPERM,'no links')), \
             patch.object(runner.shutil,'copyfileobj',side_effect=OSError(errno.ENOSPC,'disk full')):
            rejects(lambda:runner.export_and_clean(case,self.root/'failed_fallback',fixture,objects),(OSError,))
        self.assertTrue(case.exists())
        self.assertFalse((self.root/'failed_fallback/collection_manifest.json').exists())
        (case/'input.su2').write_text('changed donor')
        rejects(lambda:runner.export_and_clean(case,self.root/'altered',fixture,objects))
        self.assertTrue(case.exists());self.assertFalse(any(p.is_symlink() for p in dest.iterdir()))

    def test_link_fallback_preserves_existing_files_and_propagates_copy_errors(self):
        source=self.root/'source';source.write_bytes(b'verified input')
        for error in (errno.EPERM,errno.EACCES,errno.EOPNOTSUPP,errno.EXDEV,errno.EMLINK):
            target=self.root/str(error)
            with patch.object(runner.os,'link',side_effect=OSError(error,'no links')):
                self.assertEqual(runner.link_or_copy(source,target),'copy')
                self.assertEqual(prepare.sha(source),prepare.sha(target))
                rejects(lambda:runner.link_or_copy(source,target))
                self.assertEqual(target.read_bytes(),source.read_bytes())
        for error in (errno.ENOSPC,errno.EIO,errno.ENOENT):
            target=self.root/('fatal_'+str(error))
            with patch.object(runner.os,'link',side_effect=OSError(error,'fatal link error')):
                rejects(lambda:runner.link_or_copy(source,target),(OSError,))
            self.assertFalse(target.exists())
        with patch.object(runner.os,'link',side_effect=OSError(errno.EPERM,'no links')), \
             patch.object(runner.shutil,'copyfileobj',side_effect=OSError(errno.ENOSPC,'disk full')):
            rejects(lambda:runner.link_or_copy(source,self.root/'partial'),(OSError,))
        self.assertEqual(source.read_bytes(),b'verified input')

    def fake_campaign(self, mode='pass'):
        root=self.root;pack=root/'integration_evidence/native_cluster_balance_profile_v1';pack.mkdir(parents=True)
        for name in runner.TOOLS:
            dest=root/name;dest.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(REAL_ROOT/name,dest)
        for kind in runner.KINDS:
            fixture=root/'integration_evidence/native_cluster_campaign_v1/inputs'/kind;fixture.mkdir(parents=True)
            for name in ('input.su2','frozen_sensor.csv','frozen_sensor_source_flow.vtu'):(fixture/name).write_text(kind+name)
            (fixture/'run.cfg').write_text('MESH_FILENAME= input.su2\n')
        bins={}
        for role in ('control','profile'):
            binary=root/f'build-native/{role}/test_driver';binary.parent.mkdir(parents=True);binary.write_text(role)
            bins[role]=dict(path=str(binary.relative_to(root)),sha256=prepare.sha(binary))
        checkpoint=root/'build-native/balance_profile_checkpoint.json';checkpoint.write_text(json.dumps(dict(binaries=bins)))
        machine=root/'machinefile.123';machine.write_text('localhost slots=4\n')
        executed=[]
        def execute(command,case,environment,timeout):
            n=int(command[2]);tag=command[-3];executed.append((n,tag))
            code=1 if mode=='unit_fail' and n==2 else 0
            log='All tests passed\n'*n
            (case/'unneeded_geometry.vtk').write_text('discard if successful')
            if tag=='[NativeFrozenAirfoil2D]':
                import re
                workers=int(re.search(r'ADAP_NATIVE_RANKS= (\d+)',(case/'run.cfg').read_text())[1])
                for label in ('selection','round protocol','round dependency import','round donor import and IDs',
                              'round reconstruction','round validation','round commit','private reconstruction','adapt unclassified'):
                    log+=f'Native rank cost {label} seconds min/mean/max: 1 1 1\n'
                log+='Native rank cost engine adapt seconds min/mean/max: 8 8 8\n'
                log+=f'Native operations (same order): {workers} 0 0 0 0 0 0 0; conflicts=0\n'
                enabled=environment['SU2_NATIVE_BALANCE_PROFILE']=='YES'
                (case/'native_frozen_adapted.su2').write_text('changed' if mode=='changed' and '/profile/' in command[-4] else 'identical')
                for rank in range(4):
                    (case/f'native_frozen_target_rank_{rank}.csv').write_text('tensor'+str(rank))
                    (case/f'native_frozen_timing_rank_{rank}.csv').write_text('remesh_max_seconds\n2\n')
                if enabled:write_profile(case,workers)
            (case/'solver.log').write_text(log)
            return dict(exit_code=code,wall_seconds=2,command=command,machine_samples=[])
        def external(command,**kwargs):
            if command[0]==sys.executable:
                case=Path(command[2]);(case/'independent_frozen_metric_audit.json').write_text('{"status":"PASS"}')
            return subprocess.CompletedProcess(command,0,stdout='fake evidence',stderr='')
        with contextlib.ExitStack() as fallback:
            if mode=='no_links':fallback.enter_context(patch.object(runner.os,'link',side_effect=PermissionError(errno.EPERM,'cluster disallows hardlinks')))
            with patch.multiple(runner,ROOT=root,PACK=pack,CHECKPOINT=checkpoint),patch.object(prepare,'ROOT',root),\
                 patch.object(runner,'verify'),patch.object(runner,'execute',side_effect=execute),\
                 patch.object(runner.subprocess,'run',side_effect=external),\
                 patch.dict(os.environ,NSLOTS='4',JOB_ID='123'),\
                 patch.object(sys,'argv',['run.py','--machinefile',str(machine)]),contextlib.redirect_stdout(io.StringIO()):
                code=runner.main()
        out=root/'ClusterResults'/(runner.RESULT_PREFIX+'_123');record=json.loads((out/'validation.json').read_text())
        self.assertFalse((out/'objects').exists())
        self.assertFalse(any(p.is_symlink() for p in out.rglob('*')))
        if mode in ('pass','no_links'):
            self.assertEqual(code,0);self.assertEqual(len(record['cases']),16);self.assertEqual(len(record['pairs']),8)
            self.assertEqual([n for n,tag in executed[:3]],[1,2,4])
            self.assertFalse(list(out.rglob('unneeded_geometry.vtk')))
            if mode=='pass':self.assertLess(record['retained_unique_file_bytes'],record['retained_logical_bytes'])
            else:
                self.assertEqual(record['retained_unique_file_bytes'],record['retained_logical_bytes'])
                self.assertEqual(record['export_storage_counts']['hardlink'],0)
                self.assertGreater(record['export_storage_counts']['copy'],0)
            for row in record['cases']:
                case=out/'cases'/row['case'];manifest=json.loads((case/'collection_manifest.json').read_text())
                self.assertTrue(all(prepare.sha(case/name)==info['sha256'] for name,info in manifest['files'].items()))
                if mode=='no_links':self.assertTrue(all(info['storage']=='copy' for info in manifest['files'].values()))
                self.assertTrue((case/'independent_profile_accounting.json').is_file())
            self.assertEqual(record['status'],'PASS')
        else:
            self.assertEqual(code,1);self.assertEqual(record['status'],'FAIL')
            self.assertEqual(len(record['cases']),0 if mode=='unit_fail' else 2)
            if mode=='unit_fail':self.assertTrue((out/'unit_n2/unneeded_geometry.vtk').is_file())
        return record

    def test_full_fake_campaign(self):self.fake_campaign()
    def test_full_fake_campaign_without_hardlinks(self):self.fake_campaign('no_links')
    def test_gate_failure_stops_cases(self):self.fake_campaign('unit_fail')
    def test_changed_pair_stops_campaign(self):self.fake_campaign('changed')


if __name__=='__main__':unittest.main()
