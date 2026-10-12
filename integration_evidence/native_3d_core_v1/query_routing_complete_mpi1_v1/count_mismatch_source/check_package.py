"""Lightweight orchestration checks using fake files/processes; no build, MPI, CFD or real qsub."""
from pathlib import Path
import contextlib,gzip,io,json,os,subprocess,sys,tempfile,unittest
from unittest.mock import patch
import run,restore,submit,prepare,analyze,profile_summary,mpi_profile
from types import SimpleNamespace

class PackageChecks(unittest.TestCase):
    def test_coarsening_counters_validate_nested_costs_and_integer_work(self):
        from audit_lifecycle import coarsening_counters
        self.assertFalse(coarsening_counters({}))
        row=dict(collapse_orientation_checks=9,collapse_orientation_filtered=3,collapse_reactivated=2,
                 changed_star_scans=20,changed_vertices_sent=4,changed_vertices_received=7,
                 nested_change_notifications=.2,nested_collapse_orientation=.1,publication=.3,selection=.15)
        self.assertTrue(coarsening_counters(row))
        for key,value in [('collapse_orientation_checks',-1),('collapse_orientation_filtered',10),
                          ('collapse_reactivated',.5),('changed_vertices_sent',21),
                          ('changed_vertices_received',float('nan')),('nested_change_notifications',.31),
                          ('nested_collapse_orientation',.16)]:
            with self.subTest(key=key),self.assertRaises(AssertionError):coarsening_counters(dict(row,**{key:value}))
        incomplete=dict(row);del incomplete['changed_star_scans']
        with self.assertRaises(AssertionError):coarsening_counters(incomplete)

    def test_termination_counters_distinguish_pending_queue_from_cached_violations(self):
        from audit_lifecycle import termination_counters
        self.assertFalse(termination_counters(dict(complete=1)))
        row=dict(complete=0,final_owned_cells=1536,cached_measured_cells=1536,
                 cached_invalid_cells=0,remaining_edges=500,remaining_repairs=0)
        self.assertTrue(termination_counters(row))
        self.assertTrue(termination_counters(dict(row,cached_measured_cells=100)))
        complete=dict(row,complete=1,remaining_edges=0)
        self.assertTrue(termination_counters(complete))
        bad=dict(row);del bad['remaining_repairs']
        for corrupted in (bad,dict(row,remaining_edges=-1),dict(row,remaining_edges=.5),
                          dict(row,cached_invalid_cells=1537),dict(row,remaining_repairs=float('nan')),
                          dict(complete,cached_invalid_cells=1),dict(complete,cached_measured_cells=1535),
                          dict(complete,remaining_edges=1),dict(complete,remaining_repairs=1)):
            with self.assertRaises(AssertionError):termination_counters(corrupted)
    def native(self,ranks,workers,detail=1):
        return [dict(rank=r,ranks=ranks,workers=workers,complete=1,detailed=detail,total=1,other=0,
                     cpu=.5,idle_cpu=0,**{k:float(k=='refine') for k in profile_summary.NATIVE},
                     refine_accepted=2,coarsen_accepted=0,native_bound=4096,process_peak_rss=8192)
                for r in range(ranks) for _ in range(3)]
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
            with patch.dict(os.environ,{'PATH':'/fake/compiler/bin:/usr/bin','LD_LIBRARY_PATH':'/fake/mmg/lib:/fake/mpi/lib'}),patch.object(submit,'ROOT',root),patch.object(Path,'cwd',return_value=root),patch.object(sys,'argv',['submit.py',str(bundle)]),patch.object(subprocess,'check_output',side_effect=[str(100+i)+'\n' for i in range(7)]) as qsub,contextlib.redirect_stdout(io.StringIO()):submit.main()
            commands=[c.args[0] for c in qsub.call_args_list]
            self.assertNotIn('-hold_jid',commands[0])
            self.assertTrue(all(cmd[cmd.index('-v')+1]=='PATH,LD_LIBRARY_PATH' for cmd in commands))
            self.assertTrue(all('-V' not in cmd for cmd in commands))
            for i,cmd in enumerate(commands[1:],1):self.assertEqual(cmd[cmd.index('-hold_jid')+1],str(99+i))
            self.assertTrue(all(cmd.count('--gate')==2 for cmd in commands[4:]))
            receipt=json.loads(next((root/'ClusterResults').glob('native3d_submission_*.json')).read_text())
            self.assertEqual(receipt['status'],'SUBMITTED');self.assertEqual(len(receipt['jobs']),7)
    def test_correctness_bridge_audits_gate_later_cases(self):
        with tempfile.TemporaryDirectory() as d:
            root=Path(d);bundle=self.bundle(root);machine=root/'machinefile'
            tools=bundle/'integration_evidence/native_3d_core_v1';tools.mkdir(parents=True)
            for name in prepare.TOOLS:(tools/name).write_text('mock auditor; never executed')
            manifest=json.loads((bundle/'manifest.json').read_text())
            manifest['files_sha256']={str(p.relative_to(bundle)):run.sha(p) for p in bundle.rglob('*') if p.is_file() and p.name!='manifest.json'}
            (bundle/'manifest.json').write_text(json.dumps(manifest))
            machine.write_text('mock-node slots=4\n');calls=[]
            def fake(args,cwd,label,env,seconds):
                calls.append((cwd.name,label));out='';exit_code=0
                if label=='case':
                    ranks=int(args[args.index('-n')+1]);selection=args[-3]
                    result='48 assertions in 2 test cases' if selection.startswith('MMG interface:') else '1 assertion in 1 test case'
                    if selection==run.SELECTION:result='10255 assertions in '+('73' if cwd.parent.name=='wrong_count' else '94')+' test cases'
                    out='\n'.join('All tests passed ('+result+')' for _ in range(ranks))
                elif label=='audit':
                    if cwd.name.startswith('unit_mpi'):
                        ranks=int(cwd.name[len('unit_mpi'):])
                        self.assertEqual(Path(args[1]).name,'audit_geometry_bridge.py')
                        self.assertEqual(args[2:],[str(cwd),str(ranks),str(cwd/'case.stdout')])
                        exit_code=7 if cwd.parent.name=='failed' and ranks==2 else 0
                    else:self.assertEqual(Path(args[1]).name,'audit_envelope.py')
                    (cwd/'independent_audit.json').write_text(json.dumps(dict(status='FAIL' if exit_code else 'PASS')))
                elif label in {'audit_distributed','audit_donors','audit_anisotropic','audit_cycles','audit_repairs','audit_engine_repairs','audit_coplanar'}:
                    self.assertTrue(cwd.name.startswith('unit_mpi'))
                    self.assertEqual(Path(args[1]).stem,label)
                    certificate=Path(args[-1]) if label=='audit_donors' else Path(args[3])/'independent_audit.json'
                    certificate.parent.mkdir(parents=True,exist_ok=True)
                    exit_code=9 if cwd.parent.name=='failed_distributed' and cwd.name=='unit_mpi2' and label=='audit_donors' else 0
                    certificate.write_text(json.dumps(dict(status='FAIL' if exit_code else 'PASS')))
                (cwd/(label+'.stdout')).write_text(out);(cwd/(label+'.stderr')).write_text('')
                return dict(exit_code=exit_code,seconds=.001,label=label,command=list(map(str,args)))
            def invoke(name):
                with patch.object(sys,'argv',['run.py','--package',str(bundle),'--results',str(root/name),'--mode','correctness','--ranks','4','--machinefile',str(machine)]),patch.object(run,'command',side_effect=fake),contextlib.redirect_stdout(io.StringIO()):run.main()
            invoke('passed');receipt=json.loads((root/'passed/result.json').read_text())
            self.assertEqual(receipt['status'],'PASS');self.assertEqual(len(receipt['cases']),6)
            for ranks in (1,2,4):
                case=next(c for c in receipt['cases'] if c['folder']=='unit_mpi'+str(ranks))
                self.assertEqual([s['label'] for s in case['stages']],['case','audit','audit_distributed','audit_donors','audit_anisotropic','audit_cycles','audit_repairs','audit_engine_repairs','audit_coplanar'])
                self.assertEqual(json.loads((root/'passed'/case['folder']/'distributed_audit.json').read_text())['status'],'PASS')
                self.assertIn(case['folder']+'/independent_audit.json',receipt['artifact_sha256'])
            calls.clear()
            with self.assertRaisesRegex(RuntimeError,'Audit failure'):invoke('failed')
            failure=json.loads((root/'failed/result.json').read_text())
            self.assertEqual(failure['status'],'FAIL');self.assertEqual(len(failure['cases']),2)
            self.assertEqual(calls[-1],('unit_mpi2','audit'))
            self.assertFalse((root/'failed/unit_mpi4').exists())
            self.assertFalse(list((root/'failed').glob('restart_*')))
            self.assertEqual(json.loads((root/'failed/unit_mpi2/audit_stage.json').read_text())['exit_code'],7)
            calls.clear()
            with self.assertRaisesRegex(RuntimeError,'audit_donors.py failed'):invoke('failed_distributed')
            failure=json.loads((root/'failed_distributed/result.json').read_text())
            self.assertEqual(failure['status'],'FAIL')
            self.assertFalse((root/'failed_distributed/unit_mpi4').exists())
            failed=json.loads((root/'failed_distributed/unit_mpi2/distributed_audit.json').read_text())
            self.assertEqual(failed['status'],'FAIL')
            self.assertEqual([s['label'] for s in failed['stages']],['audit_distributed','audit_donors'])
            self.assertEqual(failed['stages'][-1]['exit_code'],9)
            calls.clear()
            with self.assertRaisesRegex(RuntimeError,'Unexpected configured regression selection/count'):invoke('wrong_count')
            self.assertFalse((root/'wrong_count/unit_mpi2').exists())
            self.assertEqual(json.loads((root/'wrong_count/result.json').read_text())['status'],'FAIL')



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
                    native=self.native(1,1,detail)
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
            self.assertEqual(len(summary['rank_profiles']),6)
            self.assertEqual(summary['rank_profiles'][0]['native_events'][0]['profile']['dominant_exclusive_phase'],'refine')
            last=folder/cases[-1]['folder']/'independent_audit.json';changed=json.loads(last.read_text());changed['native_rows'][0]['refine_accepted']=3;last.write_text(json.dumps(changed))
            with self.assertRaisesRegex(ValueError,'Artifact drift'):analyze.main([folder])
            record['artifact_sha256'][str(last.relative_to(folder))]=run.sha(last);(folder/'result.json').write_text(json.dumps(record))
            with self.assertRaisesRegex(ValueError,'Accepted work changed'):analyze.main([folder])
    def mpip_text(self,ranks):
        text='@ Version : test\n'+''.join(f'@ MPI Task Assignment : {r} mock-node\n' for r in range(ranks))
        text+='@--- MPI Time (seconds) ---\nTask AppTime MPITime MPI%\n'
        text+=''.join(f'{r} 10.00 {r+1}.000 10\n' for r in range(ranks))
        text+=f'* {10*ranks}.00 {ranks*(ranks+1)/2:.3f} 10\n'
        text+='@--- Callsites: 1 ---\nID Lev File/Address Line Parent_Funct MPI_Call\n'
        text+='1 0 native.cpp 1 test Allreduce\n1 1 driver.cpp 2 main\n'
        text+=f'@--- Callsite Time statistics (all, milliseconds): {ranks} ---\nName Site Rank Count Max Mean Min App% MPI%\n'
        text+=''.join(f'Allreduce 1 {r} 100 {10*(r+1)}.00 {10*(r+1)}.00 {10*(r+1)}.00 10 100\n' for r in range(ranks))
        return text+f'Allreduce 1 * {100*ranks} {10*ranks}.00 {5*(ranks+1)}.00 10.00 10 100\n'

    def test_mpip_report_and_application_only_preload(self):
        with tempfile.TemporaryDirectory() as d:
            root=Path(d);bundle=root/'bundle';machine=root/'machinefile'
            off=run.mpi_command(2,machine,bundle,'[NativeEnvelope3D]')
            on=run.mpi_command(2,machine,bundle,'[NativeEnvelope3D]',True)
            self.assertNotIn('env',off);self.assertEqual(on[0],'mpirun')
            self.assertEqual(on[on.index('env')+1],'LD_PRELOAD='+str(bundle/'bin/libmpiP.so'))
            self.assertIn('MPIP=-k 4 -y -p -n -t 0',on)
            with self.assertRaises(ValueError):run.mpip_report(root,2)
            report=root/'test_driver.2.123.mpiP';report.write_text(self.mpip_text(2))
            self.assertEqual(run.mpip_report(root,2)['sha256'],run.sha(report))
            report.write_text(self.mpip_text(1))
            with self.assertRaisesRegex(ValueError,'all CFD ranks'):run.mpip_report(root,2)
            report.write_text(self.mpip_text(2).replace('Allreduce',''))
            with self.assertRaisesRegex(ValueError,'Incomplete'):run.mpip_report(root,2)

    def test_mpip_pairing_summary_and_failed_preload_evidence(self):
        with tempfile.TemporaryDirectory() as d,patch.dict(os.environ,{'LD_PRELOAD':'','MPIP':''}):
            root=Path(d);bundle=self.bundle(root);machine=root/'machinefile';machine.write_text('mock-node slots=2\n')
            library=bundle/'bin/libmpiP.so';library.write_bytes(b'fake, never loaded')
            info=json.loads((bundle/'manifest.json').read_text());info['files_sha256']['bin/libmpiP.so']=run.sha(library)
            (bundle/'manifest.json').write_text(json.dumps(info));calls=[]
            def fake(args,cwd,label,env,seconds):
                out=''
                if label=='case':
                    p=json.loads((cwd/'parameters.json').read_text());on=p['mpi_profile'];calls.append((p['workers'],p['repeat'],on))
                    self.assertEqual(p['detail'],1);self.assertEqual(env['SU2_NATIVE3D_ENVELOPE_DETAIL'],'1')
                    self.assertEqual(env.get('LD_PRELOAD',''),'');self.assertEqual('env' in args,bool(on))
                    for name in run.RAW:(cwd/name).write_text(json.dumps({'workers':p['workers']}))
                    out='\n'.join('All tests passed (1 assertion in 1 test case)' for _ in range(2))
                    if on and cwd.parent.name!='failed':(cwd/'test_driver.2.123.mpiP').write_text(self.mpip_text(2))
                elif label=='audit':
                    p=json.loads((cwd/'parameters.json').read_text());scale=1+.1*p['mpi_profile']+.01*p['repeat']
                    parents=[dict(rank=r,cycle=c,parent_wall=scale*(c+1)) for c in range(3) for r in range(2)]
                    (cwd/'independent_audit.json').write_text(json.dumps(dict(status='PASS',rank_rows=parents,native_rows=self.native(2,p['workers']),rank_stages=[])))
                (cwd/(label+'.stdout')).write_text(out);(cwd/(label+'.stderr')).write_text('')
                return dict(exit_code=0,seconds=.001,label=label,command=list(map(str,args)))
            def invoke(name):
                with patch.object(sys,'argv',['run.py','--package',str(bundle),'--results',str(root/name),'--mode','mpi-profile','--ranks','2','--machinefile',str(machine)]),patch.object(run,'command',side_effect=fake),contextlib.redirect_stdout(io.StringIO()):run.main()
            invoke('passed');passed=root/'passed';receipt=json.loads((passed/'result.json').read_text())
            self.assertEqual(receipt['status'],'PASS');self.assertEqual(len(receipt['cases']),14)
            self.assertEqual(calls[:7],[(1,0,1),(1,0,0),(1,0,1),(1,1,1),(1,1,0),(1,2,0),(1,2,1)])
            self.assertEqual(receipt['mpi_profile_library_sha256'],run.sha(library))
            self.assertFalse((passed/'package/bin').exists())
            with contextlib.redirect_stdout(io.StringIO()):analyze.main([passed])
            summary=json.loads((root/'native3d_summary.json').read_text())
            self.assertEqual(len(summary['campaigns']),6)
            self.assertEqual(len(summary['mpi_api_profiles']),8)
            self.assertTrue(all(r['summary']['observed_api_calls']==200 for r in summary['mpi_api_profiles']))
            self.assertTrue(all(row['mode']=='mpi-profile' for row in summary['campaigns']))
            self.assertAlmostEqual(summary['campaigns'][0]['on_off_ratio'],1.11/1.01)
            self.assertEqual(summary['campaigns'][0]['process_on_off_ratio'],1)
            (root/'native3d_summary.json').unlink()
            changed=receipt['cases'][1];changed['fixed_work_sha256']={'mesh':'changed'}
            (passed/'result.json').write_text(json.dumps(receipt))
            with self.assertRaisesRegex(ValueError,'Unmatched fixed work'):analyze.main([passed])
            calls.clear()
            with self.assertRaisesRegex(ValueError,'Expected one full mpiP report'):invoke('failed')
            failed=root/'failed';failure=json.loads((failed/'result.json').read_text())
            self.assertEqual(failure['status'],'FAIL');self.assertEqual(len(calls),1)
            self.assertEqual(len(failure['cases']),0)
            self.assertTrue((failed/'n2_m1_r-1_p1'/run.RAW[0]).is_file())
            self.assertFalse(list((failed/'n2_m1_r-1_p1').glob('*.gz')))

    def test_mpip_full_count_time_and_stack_summary(self):
        result=mpi_profile.summary(self.mpip_text(4),4)
        self.assertEqual(result['observed_api_calls'],400)
        self.assertEqual(result['reported_rank_work_mpi_seconds'],10)
        self.assertEqual(result['per_function']['Allreduce']['estimated_rank_work_seconds'],10)
        self.assertEqual(result['mpi_time_maximum_over_mean'],1.6)
        self.assertEqual(len(result['callsites'][0]['stack']),2)
        self.assertEqual(result['per_rank'][3]['estimated_api_seconds'],4)

    def test_mpip_worker_subset_calls_are_admitted(self):
        text=self.mpip_text(4).replace('0 10.00 1.000','0 10.00 2.000').replace('1 10.00 2.000','1 10.00 3.000').replace('* 40.00 10.000','* 40.00 12.000')
        text=text.replace('Callsites: 1','Callsites: 2')
        text=text.replace('@--- Callsite Time','2 0 worker.cpp 7 exchange Waitall\n@--- Callsite Time')
        text=text.replace('(all, milliseconds): 4','(all, milliseconds): 6')
        text+='Waitall 2 0 2 500.00 500.00 500.00 10 100\nWaitall 2 1 2 500.00 500.00 500.00 10 100\nWaitall 2 * 4 500.00 500.00 500.00 10 100\n'
        result=mpi_profile.summary(text,4)
        self.assertEqual(result['observed_api_calls'],404)
        self.assertEqual(result['per_function']['Waitall']['observed_api_calls'],4)
        self.assertEqual([r['observed_api_calls'] for r in result['per_rank']],[102,102,100,100])
        with self.assertRaisesRegex(ValueError,'declared callsite'):mpi_profile.summary(text.replace('Callsites: 2','Callsites: 3'),4)

    def test_mpip_numeric_and_incomplete_corruptions(self):
        text=self.mpip_text(2)
        for old,new in [('Allreduce 1 * 200','Allreduce 1 * 201'),
                        ('Allreduce 1 0 100 10.00','Allreduce 1 0 100 nan'),
                        ('Allreduce 1 0 100 10.00','Allreduce 1 0 100 -1'),
                        ('0 10.00 1.000 10','0 10.00 8.000 10'),
                        ('1 0 native.cpp 1 test Allreduce','2 0 native.cpp 1 test Allreduce')]:
            with self.subTest(old=old),self.assertRaises(ValueError):mpi_profile.summary(text.replace(old,new),2)
        row='Allreduce 1 1 100 20.00 20.00 20.00 10 100\n'
        with self.assertRaisesRegex(ValueError,'rank-record count'):mpi_profile.summary(text.replace(row,''),2)
        with self.assertRaisesRegex(ValueError,'Duplicate mpiP callsite rank'):mpi_profile.summary(text+row,2)

    def test_mpip_rounding_accumulates_and_top_twenty_is_not_total(self):
        value,error=mpi_profile.total(iter([(1,.1),(2,.2)]))
        self.assertEqual(value,3);self.assertAlmostEqual(error,.3)
        for a,b in [((float('inf'),0),(1,0)),((1,float('nan')),(1,0))]:
            with self.assertRaisesRegex(ValueError,'derived total'):mpi_profile.closes(a,b)
        text=self.mpip_text(4)+'@--- Aggregate Time (top twenty, descending, milliseconds) ---\nAllreduce 1 99999 100 100 99999 0\n'
        self.assertEqual(mpi_profile.summary(text,4)['observed_api_calls'],400)

    def test_mpip_printed_rounding_is_explicit(self):
        text=self.mpip_text(1).replace('1.000','0.3333').replace('10.00 10.00 10.00 10 100','3.33 3.33 3.33 10 100')
        result=mpi_profile.summary(text,1)
        self.assertAlmostEqual(result['per_rank'][0]['estimated_api_seconds'],.333)
        self.assertGreater(result['per_rank'][0]['printed_time_error'],.0003)
        with self.assertRaisesRegex(ValueError,'time totals'):mpi_profile.summary(text.replace('0.3333','0.3350'),1)

    def test_mpip_preparation_requires_license(self):
        with patch.object(sys,'argv',['prepare.py','--mpip-library','/unloaded/libmpiP.so']):
            with self.assertRaisesRegex(ValueError,'both --mpip-library and --mpip-license'):prepare.main()

    def test_mpip_submission_requires_frozen_library_and_is_gated(self):
        with tempfile.TemporaryDirectory() as d:
            root=Path(d);bundle=self.bundle(root)
            with patch.object(submit,'ROOT',root),patch.object(Path,'cwd',return_value=root),patch.object(sys,'argv',['submit.py',str(bundle),'--mpi-profile']),patch.object(subprocess,'check_output') as qsub:
                with self.assertRaisesRegex(ValueError,'mpip-library'):submit.main()
                qsub.assert_not_called()
            library=bundle/'bin/libmpiP.so';library.write_bytes(b'fake, never loaded')
            info=json.loads((bundle/'manifest.json').read_text());info['files_sha256']['bin/libmpiP.so']=run.sha(library)
            (bundle/'manifest.json').write_text(json.dumps(info))
            with patch.object(submit,'ROOT',root),patch.object(Path,'cwd',return_value=root),patch.object(sys,'argv',['submit.py',str(bundle),'--mpi-profile']),patch.object(subprocess,'check_output',side_effect=[str(100+i)+'\n' for i in range(10)]) as qsub,contextlib.redirect_stdout(io.StringIO()):submit.main()
            commands=[c.args[0] for c in qsub.call_args_list];self.assertEqual(len(commands),10)
            for i,cmd in enumerate(commands[7:],7):
                self.assertIn('mpi-profile',cmd);self.assertEqual(cmd[cmd.index('-hold_jid')+1],str(99+i))
                self.assertEqual(cmd.count('--gate'),2)
            self.assertEqual(commands[-1][-1],str(root/'ClusterResults/native3d_103_lifecycle_n4'))

    def test_sge_launcher_preserves_inherited_search_paths(self):
        with tempfile.TemporaryDirectory() as d:
            root=Path(d);bundle=root/'bundle';script=bundle/'integration_evidence/native_cluster_3d_core_v1/run.py'
            script.parent.mkdir(parents=True)
            script.write_text('import json,os,sys\nfrom pathlib import Path\np=Path(sys.argv[sys.argv.index("--results")+1]);p.mkdir(parents=True)\n(p/"probe.json").write_text(json.dumps({k:os.environ[k] for k in ("PATH","LD_LIBRARY_PATH")}))\n')
            machine=root/'machinefile';machine.write_text('mock-node slots=4\n')
            env=dict(os.environ,SGE_O_WORKDIR=str(root),JOB_ID='73',NSLOTS='4',
                     MACHINEFILE_PATH=str(machine),PATH_GCC='/fake/compiler/bin',
                     LD_LIBRARY_PATH_GCC='/fake/compiler/lib',
                     LD_LIBRARY_PATH='/fake/mmg/lib:/fake/mpi/lib')
            launch=subprocess.run(['bash',str(Path(__file__).resolve().parent/'RunNative3DSGE.sh'),str(bundle),'correctness'],
                                  cwd=root,env=env,capture_output=True,text=True,timeout=10)
            self.assertEqual(launch.returncode,0,launch.stderr)
            data=json.loads((root/'ClusterResults/native3d_73_correctness_n4/probe.json').read_text())
            self.assertEqual(data['PATH'],'/fake/compiler/bin:'+env['PATH'])
            self.assertEqual(data['LD_LIBRARY_PATH'],'/fake/compiler/lib:/fake/mmg/lib:/fake/mpi/lib')
            self.assertEqual((root/'ClusterResults/launcher/native3d_73/exit_code.txt').read_text(),'0\n')

    def test_submission_without_library_path_exports_only_path(self):
        with tempfile.TemporaryDirectory() as d:
            root=Path(d);bundle=self.bundle(root)
            with patch.dict(os.environ,{'PATH':'/fake/compiler/bin:/usr/bin'},clear=True),patch.object(submit,'ROOT',root),patch.object(Path,'cwd',return_value=root),patch.object(sys,'argv',['submit.py',str(bundle)]),patch.object(subprocess,'check_output',side_effect=[str(i)+'\n' for i in range(100,107)]) as qsub,contextlib.redirect_stdout(io.StringIO()):
                submit.main()
            self.assertTrue(all(c.args[0][c.args[0].index('-v')+1]=='PATH' for c in qsub.call_args_list))

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
                    native=self.native(p['ranks'],p['workers'])
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

    def test_transport_scope_closure_and_legacy_coverage(self):
        from audit_lifecycle import transport_counters
        row=dict(tracked_bytes=60,tracked_calls=6,nested_tracked_collectives=.6,
                 tracked_import_bytes=10,tracked_import_calls=1,nested_tracked_import=.1,
                 tracked_parent_bytes=20,tracked_parent_calls=2,nested_tracked_parent=.2,
                 tracked_worker_bytes=30,tracked_worker_calls=3,nested_tracked_worker=.3)
        self.assertTrue(transport_counters(row))
        for key in row:
            bad=dict(row);bad[key]=-1
            with self.assertRaises(AssertionError):transport_counters(bad)
            bad=dict(row);bad[key]=float('nan')
            with self.assertRaises(AssertionError):transport_counters(bad)
            bad=dict(row);del bad[key]
            with self.assertRaises(AssertionError):transport_counters(bad)
        for key in ('tracked_bytes','tracked_calls','nested_tracked_collectives'):
            bad=dict(row);bad[key]+=1
            with self.assertRaises(AssertionError):transport_counters(bad)
        bad=dict(row);bad['tracked_calls']=6.5
        with self.assertRaises(AssertionError):transport_counters(bad)
        incomplete=dict(tracked_bytes=60,tracked_calls=6,nested_tracked_collectives=.6,refine_accepted=0)
        with self.assertRaises(AssertionError):transport_counters(incomplete)
        self.assertFalse(transport_counters(dict(tracked_bytes=99)))
        self.assertFalse(transport_counters({}))
        current=dict(row,rank=0,ranks=1,total=1,other=0,work=1)
        self.assertEqual(profile_summary.summarize([current],('work',))['transport_accounting'],'wrapper_totals')
        legacy=dict(rank=0,ranks=1,total=1,other=0,work=1,tracked_bytes=99)
        self.assertEqual(profile_summary.summarize([legacy],('work',))['transport_accounting'],'legacy_bytes_only')
        for k in ('directory_elections','nested_directory_election'):
            bad=dict(row);bad[k]=1
            with self.assertRaises(AssertionError):transport_counters(bad)
        engine=dict(tracked_bytes=10,tracked_calls=2,nested_tracked_collectives=.1,directory_elections=3,nested_directory_election=.2)
        self.assertTrue(transport_counters(engine))
        engine['directory_elections']=.5
        with self.assertRaises(AssertionError):transport_counters(engine)

    def test_critical_rank_phases_do_not_sum_rank_maxima(self):
        rows=[dict(rank=0,ranks=2,total=5,other=0,left=4,right=1,cpu=3),
              dict(rank=1,ranks=2,total=4,other=0,left=1,right=3,cpu=2)]
        result=profile_summary.summarize(rows,('left','right'))
        self.assertEqual(result['critical_rank'],0)
        self.assertEqual(result['elapsed_seconds'],5)
        self.assertEqual(sum(result['exclusive_seconds'].values()),5)
        self.assertEqual(result['measures']['left']['maximum']+result['measures']['right']['maximum'],7)
        self.assertEqual(result['measures']['cpu']['rank_sum'],5)
        self.assertNotIn('mpi_wait',result)
        for bad in (rows[:1],[rows[0],rows[0]],[rows[0],dict(rows[1],ranks=3)]):
            with self.assertRaises(ValueError):profile_summary.summarize(bad,('left','right'))
        with self.assertRaises(ValueError):profile_summary.summarize([rows[0],dict(rows[1],cpu=float('nan'))],('left','right'))
        with self.assertRaises(AssertionError):profile_summary.summarize([rows[0],dict(rows[1],left=2)],('left','right'))
        with self.assertRaises(ValueError):profile_summary.events([dict(r,stage='unknown') for r in rows],{})

    def test_subset_wait_is_separate_from_active_worker_cost(self):
        rows=self.native(4,2)[::3]
        for r in rows:
            r['total']=4 if r['rank']==0 else 3 if r['rank']==1 else 5
            r['refine']=0;r['coarsen']=r['total'] if r['rank']<2 else 0
            r['wait']=r['total'] if r['rank']>=2 else 0
        result=profile_summary.summarize(rows,profile_summary.NATIVE)
        self.assertEqual(result['critical_rank'],2)
        self.assertEqual(result['dominant_exclusive_phase'],'wait')
        self.assertEqual(sum(result['exclusive_seconds'].values()),5)
        worker=result['active_worker_exclusive']
        self.assertEqual(worker['rank'],0)
        self.assertEqual(worker['dominant_phase'],'coarsen')
        self.assertEqual(sum(worker['seconds'].values()),4)
        self.assertEqual(worker['rank_wall_seconds'],4)

    def test_engine_work_imbalance_uses_only_active_workers(self):
        rows=[]
        for rank in range(4):
            active=rank<2
            rows.append(dict(stage='refine',rank=rank,ranks=4,workers=2,complete=1,
                total=(4 if rank==0 else 2) if active else 0,other=0,
                **{k:((4 if rank==0 else 2) if active else 0) if k=='reconstruction' else 0 for k in analyze.ENGINE},
                private_reconstruction_cpu=((3 if rank==0 else 1) if active else 0),
                accepted=((4 if rank==0 else 2) if active else 0),
                requests=10 if active else 0,hits=5 if active else 0,
                nested_search=.1 if active else 0))
        result=profile_summary.events(rows,profile_summary.DETAIL,('refine','coarsen'))[0]['profile']
        self.assertEqual(result['measured_ranks'],[0,1])
        self.assertEqual(result['measures']['private_reconstruction_cpu']['maximum_over_mean'],1.5)
        self.assertEqual(result['accepted_per_second'],1.5)
        self.assertEqual(result['cache_hit_fraction'],.5)
        self.assertAlmostEqual(result['measures']['nested_search']['rank_sum'],.2)
        self.assertEqual(sum(result['exclusive_seconds'].values()),4)
        rows[-1]['accepted']=1
        with self.assertRaises(ValueError):profile_summary.events(rows,profile_summary.DETAIL,('refine','coarsen'))

    def test_lifecycle_analysis_keeps_closed_cpu_and_paired_timings(self):
        with tempfile.TemporaryDirectory() as d:
            root=Path(d);folder=root/'lifecycle';work=folder/'case';work.mkdir(parents=True)
            cycle=dict(stage='cycle',rank=0,ranks=1,time_iter=0,total=5,other=0,cpu=3,cpu_other=0,
                       metric_remesh=3,metric_remesh_cpu=2,replace=2,replace_cpu=1,process_peak_rss=8192)
            audit=dict(status='PASS',rank_stages=[cycle],nested_rank_details=[],
                       fixed_work_timings=[dict(workers=1,off_seconds=[1,2,3],on_seconds=[2,3,4],
                                               off_median=2,on_median=3,on_off_median_ratio=1.5)])
            (work/'lifecycle_audit.json').write_text(json.dumps(audit))
            native=self.native(1,1)[0]
            (work/'case.stdout').write_text('NATIVE3D '+' '.join(str(k)+'='+str(v) for k,v in native.items())+'\n')
            record=dict(status='PASS',mode='lifecycle',ranks=1,binary_sha256='same',package_manifest_sha256='same',cases=[dict(folder='case')])
            record['artifact_sha256']={str(p.relative_to(folder)):run.sha(p) for p in folder.rglob('*') if p.is_file()}
            (folder/'result.json').write_text(json.dumps(record))
            with contextlib.redirect_stdout(io.StringIO()):analyze.main([folder])
            summary=json.loads((root/'native3d_summary.json').read_text())
            self.assertEqual(summary['campaigns'][0]['fixed_work_timings'][0]['on_off_median_ratio'],1.5)
            result=summary['rank_profiles'][0]['exclusive_stage_events'][0]['profile']
            self.assertEqual(result['exclusive_seconds'],dict(metric_remesh=3,replace=2,other=0))
            self.assertEqual(result['measures']['metric_remesh_cpu']['rank_sum'],2)
            self.assertEqual(result['measures']['replace_cpu']['rank_sum'],1)
            self.assertIsNone(result['workers'])
            bad=dict(cycle,metric_remesh_cpu=3)
            with self.assertRaises(AssertionError):profile_summary.summarize([bad],('metric_remesh','replace'))
            with self.assertRaises(FileExistsError):analyze.main([folder])

if __name__=='__main__':unittest.main()
