"""Lightweight comparison orchestration checks; no actual MPI, CFD, build, qsub or performance measurements."""
from pathlib import Path
import contextlib,importlib.util,io,json,os,shutil,sys,tempfile,types,unittest
from unittest.mock import patch
from common import sha,verify
def module(name):
    path=Path(__file__).resolve().parent/(name+'.py')
    spec=importlib.util.spec_from_file_location('query_comparison_'+name,path)
    value=importlib.util.module_from_spec(spec);spec.loader.exec_module(value);return value
campaign,prepare,submit,analyze=[module(n) for n in ('campaign','prepare','submit','analyze')]
ROOT=Path(__file__).resolve().parents[2]
sys.path[:0]=[str(ROOT/'integration_evidence/native_cluster_3d_core_v1'),str(ROOT/'integration_evidence/native_3d_core_v1')]
import run as core_run
import audit_envelope

class Checks(unittest.TestCase):
    def package(self,root,side):
        p=root/side;(p/'bin').mkdir(parents=True);(p/'bin/test_driver').write_text(side)
        (p/'recipe.py').write_text('fixture only')
        role='baseline' if side in ('base','baseline') else 'candidate'
        sources=dict(prepare.SOURCE_PINS['original' if role=='baseline' else 'routing'])
        (p/'manifest.json').write_text(json.dumps(dict(status='PREPARED_NOT_VALIDATED',source_sha256=sources,
            build_options=[dict(name='enable-tests',value=True),dict(name='prefix',value=str(p))],
            files_sha256={str(f.relative_to(p)):sha(f) for f in p.rglob('*') if f.is_file()})))
        return p
    def comparison(self,root):
        p=root/'comparison';p.mkdir()
        for side in ('baseline','candidate'):self.package(p,side)
        for name in ('suite','shared/core','shared/audits'):(p/name).mkdir(parents=True,exist_ok=True)
        (p/'suite/dummy.py').write_text('fixture only')
        record=dict(status='PREPARED_NOT_VALIDATED',packages={s:dict(manifest_sha256=sha(p/s/'manifest.json'),binary_sha256=sha(p/s/'bin/test_driver')) for s in ('baseline','candidate')},files_sha256={str(f.relative_to(p)):sha(f) for f in p.rglob('*') if f.is_file()})
        (p/'comparison_manifest.json').write_text(json.dumps(record));return p
    def test_source_and_build_mismatch_rejected(self):
        with tempfile.TemporaryDirectory() as t:
            root=Path(t);b=self.package(root,'baseline');c=self.package(root,'candidate')
            manifests={side:json.loads((root/side/'manifest.json').read_text()) for side in ('baseline','candidate')}
            prepare.matched_sources(manifests)
            for field in ('donor','allowed_header','unrelated','options'):
                mutated=json.loads(json.dumps(manifests))
                if field=='donor':mutated['baseline']['source_sha256'][prepare.DONOR]='wrong'
                elif field=='allowed_header':mutated['candidate']['source_sha256']['Common/include/adaptation/CNativeEngine3D.hpp']='changed guard'
                elif field=='unrelated':
                    for side in mutated:mutated[side]['source_sha256']['Common/other.cpp']=side
                else:mutated['candidate']['build_options'][0]['value']=False
                with self.assertRaises(ValueError):prepare.matched_sources(mutated)
    def test_known_source_pairs_and_reversed_comparison(self):
        options=[dict(name='enable-tests',value=True)]
        for old,new in (('original','routing'),('original','packed'),('routing','packed'),('original','box'),('routing','box'),('packed','box'),('original','box_diagnostics'),('routing','box_diagnostics'),('packed','box_diagnostics'),('box','box_diagnostics')):
            manifests={side:dict(source_sha256=dict(prepare.SOURCE_PINS[version]),build_options=options)
                       for side,version in [('baseline',old),('candidate',new)]}
            self.assertEqual(prepare.matched_sources(manifests),dict(baseline=old,candidate=new))
            with self.assertRaises(ValueError):prepare.matched_sources(dict(baseline=manifests['candidate'],candidate=manifests['baseline']))

    def test_shared_donor_version_requires_complete_engine_and_test_pins(self):
        self.assertEqual(prepare.SOURCE_PINS['box'][prepare.DONOR],prepare.SOURCE_PINS['box_diagnostics'][prepare.DONOR])
        options=[dict(name='enable-tests',value=True)]
        manifests={side:dict(source_sha256=dict(prepare.SOURCE_PINS[version]),build_options=options)
                   for side,version in [('baseline','box'),('candidate','box_diagnostics')]}
        self.assertEqual(prepare.matched_sources(manifests),dict(baseline='box',candidate='box_diagnostics'))
        manifests['candidate']['source_sha256']['UnitTests/Common/adaptation/CNativeDistributed3D_tests.cpp']=prepare.SOURCE_PINS['box']['UnitTests/Common/adaptation/CNativeDistributed3D_tests.cpp']
        with self.assertRaises(ValueError):prepare.matched_sources(manifests)

    def test_schedule_pairs(self):
        for ranks,count in ((1,14),(2,28),(4,28)):
            rows=list(campaign.schedule(ranks));self.assertEqual(len(rows),count)
            for m in {r[0] for r in rows}:
                for repeat in (0,1,2):
                    for detail in (0,1):self.assertEqual({r[3] for r in rows if r[:3]==(m,repeat,detail)},{'baseline','candidate'})
    def test_schedule_alternates(self):
        rows=list(campaign.schedule(1));self.assertNotEqual(rows[2][3],rows[4][3])
    def test_verify_mutation_extra_link(self):
        for kind in ('changed','extra','link'):
            with self.subTest(kind=kind),tempfile.TemporaryDirectory() as t:
                p=self.comparison(Path(t));verify(p)
                if kind=='changed':(p/'suite/dummy.py').write_text('changed')
                elif kind=='extra':(p/'unexpected').write_text('extra')
                else:(p/'link').symlink_to(p/'suite/dummy.py')
                with self.assertRaises(ValueError):verify(p)
    def test_prepare_regular_closed_copies(self):
        with tempfile.TemporaryDirectory() as t:
            root=Path(t);b=self.package(root,'base');c=self.package(root,'cand');out=root/'closed'
            for directory,names in [('native_cluster_3d_core_v1',('run.py','mpi_profile.py','profile_summary.py')),
                                    ('native_3d_core_v1',('audit_cavities.py','audit_coupled.py','audit_geometry_bridge.py','audit_lifecycle.py','audit_envelope.py','audit_restart.py'))]:
                target=root/'integration_evidence'/directory;target.mkdir(parents=True)
                for name in names:shutil.copyfile(ROOT/'integration_evidence'/directory/name,target/name)
            with patch.object(prepare,'ROOT',root),patch.object(sys,'argv',['prepare','--baseline',str(b),'--candidate',str(c),'--output',str(out)]),contextlib.redirect_stdout(io.StringIO()):prepare.main()
            verify(out);self.assertFalse(any(f.is_symlink() for f in out.rglob('*')))
            self.assertEqual(sha(out/'baseline/bin/test_driver'),sha(b/'bin/test_driver'))
    def test_submit_serial_gate_and_inherited_environment(self):
        with tempfile.TemporaryDirectory() as t:
            root=Path(t);p=self.comparison(root);calls=[]
            def qsub(cmd,**kwargs):calls.append(cmd);return str(100+len(calls))
            with patch.object(submit,'ROOT',root),patch.object(Path,'cwd',return_value=root),patch.object(submit.subprocess,'check_output',side_effect=qsub),patch.object(sys,'argv',['submit',str(p),'--hold-jid','99']),patch.dict(os.environ,{'LD_LIBRARY_PATH':'fixture'}),contextlib.redirect_stdout(io.StringIO()):submit.main()
            self.assertEqual(len(calls),4)
            for i,cmd in enumerate(calls):
                self.assertEqual(cmd[cmd.index('-hold_jid')+1],str(99 if not i else 100+i))
                self.assertIn('PATH,LD_LIBRARY_PATH',cmd)
                if i:self.assertIn('--gate',cmd)
    def fake_campaign(self,root,changed=False,profiling=False):
        bundle=self.comparison(root);machine=root/'machinefile';machine.write_text('fixture slots')
        if profiling:
            for side in ('baseline','candidate'):
                q=bundle/side;(q/'bin/libmpiP.so').write_text('same fixture library')
                m=json.loads((q/'manifest.json').read_text());m['files_sha256']['bin/libmpiP.so']=sha(q/'bin/libmpiP.so');(q/'manifest.json').write_text(json.dumps(m))
            m=json.loads((bundle/'comparison_manifest.json').read_text());m['files_sha256']={str(f.relative_to(bundle)):sha(f) for f in bundle.rglob('*') if f.is_file() and f.name!='comparison_manifest.json'}
            (bundle/'comparison_manifest.json').write_text(json.dumps(m))
        gate=root/'gate';gate.mkdir();(gate/'result.json').write_text(json.dumps(dict(status='PASS',mode='correctness',manifest_sha256=sha(bundle/'comparison_manifest.json'))))
        def mpi(ranks,machinefile,package,selection,profile=False):return ['fixture-profile' if profile else 'fixture',str(package),selection]
        def command(args,cwd,label,env,seconds):
            cwd=Path(cwd);(cwd/(label+'.stdout')).write_text('');(cwd/(label+'.stderr')).write_text('')
            if label=='case':
                if args[0]=='fixture-profile':(cwd/'fixture.mpiP').write_text('fixture API report')
                (cwd/'case.stdout').write_text('All tests passed (1 assertions in 1 test case)\n')
                if 'ENVELOPE' in args[-1].upper():
                    for name in core_run.RAW:(cwd/name).write_text(json.dumps(dict(nodes=[],cells=[],marker_names=[])))
                    if changed and 'candidate' in cwd.name:(cwd/'envelope_s0_states.json').write_text('changed')
                    for cycle in range(3):
                        (cwd/f'envelope_mesh_{cycle+1:05d}.su2').write_text('fixture grid')
                        (cwd/f'envelope_mesh_{cycle+1:05d}.su2.native_ref').write_text('fixture reference')
                    for fmt,ext in [('SU2','su2'),('CGNS','cgns')]:(cwd/f'envelope_s2_written_{fmt}.{ext}').write_text('fixture output')
            elif label=='full_audit':(cwd/'independent_audit.json').write_text(json.dumps(dict(status='PASS')))
            return dict(label=label,command=list(map(str,args)),exit_code=0,seconds=1.)
        tools=types.SimpleNamespace(command=command,mpi_command=mpi,RAW=core_run.RAW,compress_json=core_run.compress_json,verify_package=core_run.verify_package,mpip_report=lambda w,r:dict(file='fixture.mpiP',sha256=sha(w/'fixture.mpiP'),scope='mocked API only'))
        geometry=types.ModuleType('audit_geometry_bridge');geometry.signature=lambda *a:();geometry.su2_signature=lambda *a:();geometry.cgns_signature=lambda *a:()
        measured=dict(samples=[dict(cycle=c,seconds=1.,accepted=10,accepted_per_second=10.) for c in range(3)],profiles={},native_rows=[],envelope_rows=[])
        args=['campaign','--package',str(bundle),'--results',str(root/'results'),'--machinefile',str(machine),'--ranks','1','--mode','timing','--gate',str(gate)]
        if profiling:args+=['--mpi-profile']
        with patch.object(campaign,'core',return_value=tools),patch.object(campaign,'runtime',return_value=measured),patch.dict(sys.modules,{'audit_geometry_bridge':geometry}),patch.object(sys,'argv',args),contextlib.redirect_stdout(io.StringIO()):
            with self.assertRaises(SystemExit) as e:campaign.main()
        return root/'results',e.exception.code
    def test_compact_success_uses_one_reference(self):
        with tempfile.TemporaryDirectory() as t:
            results,code=self.fake_campaign(Path(t));self.assertEqual(code,0)
            record=json.loads((results/'result.json').read_text());self.assertEqual(record['status'],'PASS')
            cases=[c for c in record['cases'] if not c.get('lifecycle')];self.assertEqual(len(cases),14)
            refs=[c for c in cases if c['audit']['mode']=='fresh_full_geometry'];self.assertEqual(len(refs),1)
            for c in cases:
                f=results/c['folder']
                if c not in refs:self.assertFalse(list(f.glob('*.su2')))
            self.assertTrue((results/'provenance/suite/dummy.py').is_file())
    def test_changed_target_preserved_and_rejected(self):
        with tempfile.TemporaryDirectory() as t:
            results,code=self.fake_campaign(Path(t),changed=True);self.assertEqual(code,1)
            record=json.loads((results/'result.json').read_text());self.assertEqual(record['status'],'FAIL')
            failed=next(f for f in results.iterdir() if f.is_dir() and 'candidate' in f.name and '_r' in f.name)
            self.assertTrue((failed/'envelope_mesh_00001.su2').is_file())
    def test_analyze_pairs_and_overhead(self):
        with tempfile.TemporaryDirectory() as t:
            results,code=self.fake_campaign(Path(t));out=Path(t)/'summary.json'
            with patch.object(analyze,'runtime',side_effect=lambda w,p:json.loads((results/'result.json').read_text())['cases'][next(i for i,c in enumerate(json.loads((results/'result.json').read_text())['cases']) if c['folder']==w.name)]['runtime']),patch.object(sys,'argv',['analyze',str(results),'--output',str(out)]),contextlib.redirect_stdout(io.StringIO()):analyze.main()
            d=json.loads(out.read_text());self.assertEqual(len(d['comparisons']),6)
            self.assertEqual(len(d['instrumentation_overhead']),6)
            self.assertTrue(all(c['paired_candidate_baseline_ratio']['median']==1 for c in d['comparisons']))
    def test_profiler_pairs_keep_detail_on(self):
        with tempfile.TemporaryDirectory() as t:
            results,code=self.fake_campaign(Path(t),profiling=True);self.assertEqual(code,0)
            d=json.loads((results/'result.json').read_text());cases=[c for c in d['cases'] if not c.get('lifecycle')]
            self.assertTrue(all(c['parameters']['detail']==1 for c in cases))
            self.assertEqual({c['parameters']['mpi_profile'] for c in cases},{0,1})
            self.assertTrue(all(bool(c['mpi_profile_report'])==bool(c['parameters']['mpi_profile']) for c in cases))
            out=Path(t)/'profile_summary.json'
            original_api=core_run.mpip_report
            self.addCleanup(setattr,core_run,'mpip_report',original_api)
            core_run.mpip_report=lambda w,r:dict(file='fixture.mpiP',sha256=sha(w/'fixture.mpiP'),scope='mocked API only')
            with patch.object(analyze,'runtime',side_effect=lambda w,p:json.loads((results/'result.json').read_text())['cases'][next(i for i,c in enumerate(json.loads((results/'result.json').read_text())['cases']) if c['folder']==w.name)]['runtime']),patch.object(sys,'argv',['analyze',str(results),'--output',str(out)]),contextlib.redirect_stdout(io.StringIO()):analyze.main()
            report=json.loads(out.read_text());self.assertTrue(all(r['control_kind']=='mpi-profile' for r in report['comparisons']))
    def test_analyze_rejects_false_runtime_summary(self):
        with tempfile.TemporaryDirectory() as t:
            results,code=self.fake_campaign(Path(t))
            with patch.object(analyze,'runtime',return_value={}),patch.object(sys,'argv',['analyze',str(results),'--output',str(Path(t)/'summary.json')]):
                with self.assertRaisesRegex(ValueError,'actual log'):analyze.main()
    def test_analyze_rejects_artifact_drift(self):
        with tempfile.TemporaryDirectory() as t:
            results,code=self.fake_campaign(Path(t));(results/'scheduler_machinefile.txt').write_text('changed')
            with patch.object(sys,'argv',['analyze',str(results),'--output',str(Path(t)/'summary.json')]):
                with self.assertRaisesRegex(ValueError,'drift'):analyze.main()
if __name__=='__main__':unittest.main()
