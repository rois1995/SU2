"""Lightweight orchestration checks using fake files/processes; no build, MPI, CFD or real qsub."""
from pathlib import Path
import contextlib,gzip,io,json,os,subprocess,sys,tempfile,unittest
from unittest.mock import patch
import run,restore,submit,prepare,analyze
from types import SimpleNamespace

class PackageChecks(unittest.TestCase):
    def bundle(self,root):
        p=root/'bundle';(p/'bin').mkdir(parents=True);(p/'bin/test_driver').write_bytes(b'fake, never executed')
        info=dict(status='PREPARED_NOT_VALIDATED',files_sha256={'bin/test_driver':run.sha(p/'bin/test_driver')})
        (p/'manifest.json').write_text(json.dumps(info));return p
    def test_frozen_closure_and_drift(self):
        with tempfile.TemporaryDirectory() as d:
            root=Path(d);bundle=self.bundle(root);run.verify_package(bundle)
            outside=root/'outside';outside.write_text('external')
            link=bundle/'escape';link.symlink_to(outside)
            with self.assertRaises(ValueError):run.verify_package(bundle)
            link.unlink();(bundle/'untracked.py').write_text('injection')
            with self.assertRaises(ValueError):run.verify_package(bundle)
            (bundle/'untracked.py').unlink();(bundle/'bin/test_driver').write_bytes(b'changed')
            with self.assertRaises(ValueError):run.verify_package(bundle)
            info=json.loads((bundle/'manifest.json').read_text());info['files_sha256']={'../outside':run.sha(outside),'bin/test_driver':run.sha(bundle/'bin/test_driver')}
            (bundle/'manifest.json').write_text(json.dumps(info))
            with self.assertRaises(ValueError):run.verify_package(bundle)
    def test_compression_restoration_and_no_overwrite(self):
        with tempfile.TemporaryDirectory() as d:
            folder=Path(d);case=folder/'case';case.mkdir();source=case/'envelope_s0.json';source.write_text('{"exact":1.0000000000000002}\n')
            expected=source.read_bytes();packed=run.compress_json(case)
            self.assertFalse(source.exists());self.assertEqual(gzip.decompress(source.with_suffix('.json.gz').read_bytes()),expected)
            record=dict(cases=[dict(folder='case',packed=packed)],artifact_sha256={str(p.relative_to(folder)):run.sha(p) for p in folder.rglob('*') if p.is_file()})
            (folder/'result.json').write_text(json.dumps(record))
            with contextlib.redirect_stdout(io.StringIO()):restore.restore(folder);restore.restore(folder)
            self.assertEqual(source.read_bytes(),expected);source.write_text('keep my changed data')
            with self.assertRaises(ValueError):restore.restore(folder)
            self.assertEqual(source.read_text(),'keep my changed data')
    def test_existing_compressed_output_preserves_input(self):
        with tempfile.TemporaryDirectory() as d:
            folder=Path(d);p=folder/'envelope_s0.json';p.write_text('original');p.with_suffix('.json.gz').write_bytes(b'keep')
            with self.assertRaises(FileExistsError):run.compress_json(folder)
            self.assertEqual(p.read_text(),'original')
    def test_failed_and_timed_out_owned_process_logs(self):
        with tempfile.TemporaryDirectory() as d:
            folder=Path(d)
            failed=run.command([sys.executable,'-c','print("retained");raise SystemExit(3)'],folder,'fail',os.environ.copy(),10)
            self.assertEqual(failed['exit_code'],3);self.assertIn('retained',(folder/'fail.stdout').read_text())
            timed=run.command([sys.executable,'-c','import time;time.sleep(30)'],folder,'timeout',os.environ.copy(),.1)
            self.assertTrue(timed['timeout']);self.assertNotEqual(timed['exit_code'],0)
    def test_serial_scheduler_chain_and_result_gates(self):
        with tempfile.TemporaryDirectory() as d:
            root=Path(d);bundle=self.bundle(root)
            with patch.object(submit,'ROOT',root),patch.object(Path,'cwd',return_value=root),patch.object(sys,'argv',['submit.py',str(bundle)]),patch.object(subprocess,'check_output',side_effect=[str(100+i)+'\n' for i in range(7)]) as qsub,contextlib.redirect_stdout(io.StringIO()):submit.main()
            commands=[c.args[0] for c in qsub.call_args_list]
            self.assertNotIn('-hold_jid',commands[0])
            for i,cmd in enumerate(commands[1:],1):self.assertEqual(cmd[cmd.index('-hold_jid')+1],str(99+i))
            self.assertTrue(all(cmd.count('--gate')==2 for cmd in commands[4:]))
            receipt=json.loads(next((root/'ClusterResults').glob('native3d_submission_*.json')).read_text())
            self.assertEqual(receipt['status'],'SUBMITTED');self.assertEqual(len(receipt['jobs']),7)
    def test_stale_build_refuses_preparation_before_copy(self):
        with tempfile.TemporaryDirectory() as d:
            root=Path(d);build=root/'build';(build/'UnitTests').mkdir(parents=True)
            binary=build/'UnitTests/test_driver';binary.write_bytes(b'old build')
            (build/'build.ninja').write_text('pending build')
            output=root/'prepared'
            with patch.object(prepare,'ROOT',root),patch.object(sys,'argv',['prepare.py','--build-dir',str(build),'--output',str(output)]),patch.object(subprocess,'run',return_value=SimpleNamespace(returncode=0,stdout='[1/1] Compile pending.cpp',stderr='')) as dry:
                with self.assertRaises(RuntimeError):prepare.main()
            self.assertIn('-n',dry.call_args.args[0]);self.assertFalse(output.exists());self.assertEqual(binary.read_bytes(),b'old build')
    def test_disabled_required_build_options_refuse_bundle(self):
        with tempfile.TemporaryDirectory() as d:
            root=Path(d);build=root/'build';(build/'UnitTests').mkdir(parents=True)
            (build/'UnitTests/test_driver').write_bytes(b'fake build');(build/'build.ninja').write_text('-DHAVE_MPI')
            metadata=build/'meson-info/intro-buildoptions.json';metadata.parent.mkdir()
            metadata.write_text(json.dumps([dict(name=n,value=(n!='enable-mmg')) for n in ('enable-tests','enable-mmg','enable-cgns','enable-normal')]))
            output=root/'prepared'
            with patch.object(prepare,'ROOT',root),patch.object(sys,'argv',['prepare.py','--build-dir',str(build),'--output',str(output)]),patch.object(subprocess,'run',return_value=SimpleNamespace(returncode=0,stdout='ninja: no work to do.',stderr='')):
                with self.assertRaisesRegex(ValueError,'enable-mmg'):prepare.main()
            self.assertFalse(output.exists())
    def test_timing_summary_checks_medians_and_accepted_work(self):
        with tempfile.TemporaryDirectory() as d:
            root=Path(d);folder=root/'timing';folder.mkdir();cases=[]
            for repeat in range(3):
                for detail in (0,1):
                    work=folder/('r'+str(repeat)+'d'+str(detail));work.mkdir()
                    parent=[dict(rank=0,cycle=c,parent_wall=(1+.1*detail+.01*repeat)*(c+1)) for c in range(3)]
                    native=[dict(rank=0,refine_accepted=2,coarsen_accepted=0,native_bound=4096,process_peak_rss=8192) for c in range(3)]
                    (work/'independent_audit.json').write_text(json.dumps(dict(status='PASS',rank_rows=parent,native_rows=native,rank_stages=[])))
                    (work/'case.stdout').write_text('')
                    cases.append(dict(folder=work.name,parameters=dict(ranks=1,workers=1,repeat=repeat,detail=detail),warmup=False,fixed_work_sha256={'mesh':'same'}))
            record=dict(status='PASS',mode='timing',ranks=1,binary_sha256='same',package_manifest_sha256='same',cases=cases)
            record['artifact_sha256']={str(p.relative_to(folder)):run.sha(p) for p in folder.rglob('*') if p.is_file()}
            (folder/'result.json').write_text(json.dumps(record))
            with contextlib.redirect_stdout(io.StringIO()):analyze.main([folder])
            summary=json.loads((root/'native3d_summary.json').read_text())
            self.assertAlmostEqual(summary['campaigns'][0]['off_median'],1.01)
            self.assertAlmostEqual(summary['campaigns'][0]['on_median'],1.11)
            last=folder/cases[-1]['folder']/'independent_audit.json';changed=json.loads(last.read_text());changed['native_rows'][0]['refine_accepted']=3;last.write_text(json.dumps(changed))
            with self.assertRaisesRegex(ValueError,'Artifact drift'):analyze.main([folder])
            record['artifact_sha256'][str(last.relative_to(folder))]=run.sha(last);(folder/'result.json').write_text(json.dumps(record))
            with self.assertRaisesRegex(ValueError,'Accepted work changed'):analyze.main([folder])
    def test_partial_submission_receipt_is_retained(self):
        with tempfile.TemporaryDirectory() as d:
            root=Path(d);bundle=self.bundle(root)
            with patch.object(submit,'ROOT',root),patch.object(Path,'cwd',return_value=root),patch.object(sys,'argv',['submit.py',str(bundle)]),patch.object(subprocess,'check_output',side_effect=['200\n',RuntimeError('scheduler unavailable')]),contextlib.redirect_stdout(io.StringIO()):
                with self.assertRaises(RuntimeError):submit.main()
            receipt=json.loads(next((root/'ClusterResults').glob('native3d_submission_*.json')).read_text())
            self.assertEqual(receipt['status'],'FAIL');self.assertEqual(receipt['jobs'][0]['job'],'200')
            self.assertIsNotNone(receipt['pending']);self.assertIn('scheduler unavailable',receipt['failure'])

    def test_optional_limit_submission_is_serialized_and_gated(self):
        with tempfile.TemporaryDirectory() as d:
            root=Path(d);bundle=self.bundle(root)
            with patch.object(submit,'ROOT',root),patch.object(Path,'cwd',return_value=root),patch.object(sys,'argv',['submit.py',str(bundle),'--limits']),patch.object(subprocess,'check_output',side_effect=[str(i)+'\n' for i in range(100,108)]) as qsub,contextlib.redirect_stdout(io.StringIO()):
                submit.main()
            commands=[c.args[0] for c in qsub.call_args_list]
            self.assertEqual(len(commands),8)
            self.assertEqual(commands[-1][commands[-1].index('-hold_jid')+1],'106')
            self.assertEqual(commands[-1][-4:],['--gate',str(root/'ClusterResults/native3d_100_correctness_n4'),'--gate',str(root/'ClusterResults/native3d_103_lifecycle_n4')])
            self.assertIn('limits',commands[-1])
    def test_limit_matrix_summary_and_failure_evidence(self):
        with tempfile.TemporaryDirectory() as d:
            root=Path(d);bundle=self.bundle(root)
            recipe=bundle/'README.md';recipe.write_text('fake frozen recipe')
            manifest=json.loads((bundle/'manifest.json').read_text());manifest['files_sha256']['README.md']=run.sha(recipe)
            (bundle/'manifest.json').write_text(json.dumps(manifest))
            machine=root/'machinefile';machine.write_text('fake-node slots=4\n')
            calls=[]
            def fake(args,cwd,label,env,seconds):
                out='';exit_code=0
                if label=='case':
                    ranks=int(args[args.index('-n')+1]);workers=int(env['SU2_NATIVE3D_ENVELOPE_WORKERS'])
                    calls.append((ranks,workers))
                    self.assertEqual(env['SU2_NATIVE3D_ENVELOPE_SIZE'],'8')
                    self.assertEqual(env['SU2_NATIVE3D_ENVELOPE_ROUNDS'],'512')
                    self.assertEqual(env['OMP_NUM_THREADS'],'1')
                    self.assertTrue((cwd/'parameters.json').is_file())
                    out='\n'.join('All tests passed (1 assertion in 1 test case)' for _ in range(ranks))
                    for name in run.RAW:(cwd/name).write_text(json.dumps(dict(ranks=ranks,workers=workers)))
                    if cwd.parent.name=='failed' and ranks==2:exit_code=7;out='synthetic rejected case'
                elif label=='audit':
                    p=json.loads((cwd/'parameters.json').read_text())
                    parents=[dict(rank=r,cycle=c,parent_wall=float(c+1+r)) for c in range(3) for r in range(p['ranks'])]
                    native=[dict(rank=r,refine_accepted=2,coarsen_accepted=0,native_bound=4096,process_peak_rss=8192) for r in range(p['ranks']) for c in range(3)]
                    (cwd/'independent_audit.json').write_text(json.dumps(dict(status='PASS',rank_rows=parents,native_rows=native,rank_stages=[])))
                (cwd/(label+'.stdout')).write_text(out);(cwd/(label+'.stderr')).write_text('')
                return dict(exit_code=exit_code,seconds=.001,label=label,command=list(map(str,args)))
            def invoke(name):
                with patch.object(sys,'argv',['run.py','--package',str(bundle),'--results',str(root/name),'--mode','limits','--ranks','4','--machinefile',str(machine)]),patch.object(run,'command',side_effect=fake),contextlib.redirect_stdout(io.StringIO()):run.main()
            invoke('passed')
            self.assertEqual(calls,[(1,1),(2,2),(4,2),(4,4)])
            passed=root/'passed';receipt=json.loads((passed/'result.json').read_text())
            self.assertEqual(receipt['status'],'PASS');self.assertEqual(len(receipt['cases']),4)
            with contextlib.redirect_stdout(io.StringIO()):analyze.main([passed])
            summary=json.loads((root/'native3d_summary.json').read_text())
            self.assertEqual(len(summary['campaigns']),12)
            self.assertTrue(all(row['mode']=='limits' and row['size']==8 and 'off_median' not in row for row in summary['campaigns']))
            calls.clear()
            with self.assertRaises(RuntimeError):invoke('failed')
            self.assertEqual(calls,[(1,1),(2,2)])
            failed=root/'failed';receipt=json.loads((failed/'result.json').read_text())
            self.assertEqual(receipt['status'],'FAIL');self.assertEqual(len(receipt['cases']),1)
            work=failed/'limit_size8_n2_m2'
            self.assertEqual(json.loads((work/'parameters.json').read_text())['size'],8)
            self.assertEqual(json.loads((work/'case_environment.json').read_text())['SU2_NATIVE3D_ENVELOPE_WORKERS'],'2')
            self.assertEqual(json.loads((work/'stages.json').read_text())[0]['exit_code'],7)
            self.assertTrue((work/run.RAW[0]).is_file());self.assertFalse(list(work.glob('*.gz')))
            self.assertFalse((failed/'limit_size8_n4_m2').exists())

    def test_cache_counters_close_and_reject_corruption(self):
        from audit_lifecycle import field_counters,nested_stage
        good=dict(requests=10,hits=3,misses=5,invalid_queries=2,failures=3,evictions=1,compositions=7)
        field_counters(good)
        for key in good:
            bad=dict(good);bad[key]=-1
            with self.assertRaises(AssertionError):field_counters(bad)
        for key in good:
            bad=dict(good);bad[key]=float('nan')
            with self.assertRaises(AssertionError):field_counters(bad)
        for key in ('misses','invalid_queries','failures','evictions','compositions'):
            bad=dict(good);bad[key]=11
            with self.assertRaises(AssertionError):field_counters(bad)
        bad=dict(good);del bad['misses']
        with self.assertRaises(AssertionError):field_counters(bad)
        bad=dict(good);bad.update(invalid_queries=2,failures=1)
        with self.assertRaises(AssertionError):field_counters(bad)
        for label in ('refine','coarsen','import','weights','return'):
            phases=analyze.ENGINE if label in ('refine','coarsen') else ('points','cells','faces','source','query','reader')
            row=dict(good,stage=label,total=0,other=0,**{p:0 for p in phases})
            nested_stage(row);row['hits']=11
            with self.assertRaises(AssertionError):nested_stage(row)
if __name__=='__main__':unittest.main()
