#!/usr/bin/env python3
"""Replay the saved rejected serial RANS transfer without running CFD or changing gates."""
import csv,hashlib,json,os,re,subprocess,sys,time
from collections import defaultdict
from fractions import Fraction as F
from pathlib import Path
from construct import HERE,ROOT,sha
sys.path.insert(0,str(ROOT/'integration_evidence/native_3d_core_v1'))
from audit_cavities import determinant

def main():
    source=HERE/'native_curved_rans_n1_m1_v5';folder=HERE/'sa_projection_replay_v1';record=HERE/'sa_projection_replay_v1.json'
    receiptPath=source.with_suffix('.json');old=json.loads(receiptPath.read_text());assert old['status']=='FAIL'
    assert all(sha(source/p)==h for p,h in old['artifacts'].items())
    build=ROOT/'integration_evidence/build-integrated-v2';binary=build/'UnitTests/test_driver'
    check=subprocess.run(['/tmp/native-build-tools/ninja','-C',str(build),'-n','UnitTests/test_driver'],capture_output=True,text=True,timeout=30)
    assert check.returncode==0 and 'ninja: no work to do.' in check.stdout,check.stdout+check.stderr
    objects={p:h for p,h in old['contract']['pins'].items() if p.startswith('object:')}
    assert objects and all(sha(Path(p[7:]))==h for p,h in objects.items()),'Production objects changed since saved CFD run'
    files=['UnitTests/SU2_CFD/adaptation/CConservativeTransfer_tests.cpp','UnitTests/SU2_CFD/adaptation/fixtures/native_saved_sa_projection.inc','SU2_CFD/include/adaptation/CConservativeTransfer.hpp','SU2_CFD/src/adaptation/CConservativeTransfer.cpp','SU2_CFD/src/adaptation/CTransferAdmissibility.cpp']
    contract={'saved_receipt_sha256':sha(receiptPath),'test_binary_sha256':sha(binary),'production_objects':objects,'sources':{p:sha(ROOT/p) for p in files},'helper_sha256':sha(Path(__file__).resolve()),'geometry_predicate_sha256':sha(ROOT/'integration_evidence/native_3d_core_v1/audit_cavities.py')}
    if record.exists():
        prior=json.loads(record.read_text())
        if prior.get('status')=='PASS_SA_BOUND_DIAGNOSIS' and prior['contract']==contract and all(sha(folder/p)==h for p,h in prior['artifacts'].items()):print('Reused complete saved-state diagnosis');return
        raise RuntimeError('Existing replay evidence differs or is incomplete; inspect its real process before creating another case.')
    assert not folder.exists();folder.mkdir();print('Case folder:',folder,flush=True)
    workers=old['contract']['environment']['SU2_NATIVE_POLICY_WORKERS'];prefix='curved_rans_m'+workers
    config=(source/(prefix+'.cfg')).read_text();assert 'KIND_TURB_MODEL= SA\n' in config and 'ADAP_HAUSD= .04\n' in config
    counts=re.findall(r'Turbulence values limited to the bounds of the solver: (\d+) \(after the recovery: 0\)',(source/'native.log').read_text());assert len(counts)==2
    stages=[];lines=['SU2_SA_BOUND_REPLAY 1 2 1e-16 .08'];scale=None
    def mesh(data,rows):
        nodes={n[0]:tuple(map(F,n[1:])) for n in data['nodes']};local={n:i for i,n in enumerate(nodes)};cv=defaultdict(F)
        for c in data['cells']:
            volume=determinant([nodes[n] for n in c['nodes']])/6;assert volume>0
            for n in c['nodes']:cv[n]+=volume/4
        values={tuple(r[:3]):r[3:] for r in rows};assert set(values)=={tuple(map(float,p)) for p in nodes.values()}
        groups=defaultdict(list);markers={f['id']:f['marker'] for f in data['facets']}
        for f in data['surface']:groups[data['marker_names'][markers[f['facet']]]].append(f['nodes'])
        assert set(groups)=={'cylinder','cap0','cap1'}
        out=[f'{len(nodes)} {len(data["cells"])} {len(groups)}']
        for n,p in nodes.items():out.append(' '.join(format(float(x),'.17g') for x in (*p,cv[n],values[tuple(map(float,p))][0],values[tuple(map(float,p))][2])))
        out.extend(' '.join(str(local[n]) for n in c['nodes']) for c in data['cells'])
        for name,faces in groups.items():out.append(f'{name} {len(faces)}');out.extend(' '.join(str(local[n]) for n in f) for f in faces)
        return out
    for stage in (0,1):
        stem=prefix+'_s'+str(stage);sa=json.loads((source/(stem+'_SA.json')).read_text());p=json.loads((source/(stem+'_projection.json')).read_text())
        scale=sa['donor_scale'] if scale is None else scale;assert scale==sa['donor_scale']
        lines+=mesh(json.loads((source/(stem+'_initial.json')).read_text()),sa['before'])
        lines+=mesh(json.loads((source/(stem+'_accepted.json')).read_text()),sa['after'])
        row={key:[p[key][f] for f in (5,11)] for key in ('target_total','new_total','scale')};row['limited']=int(counts[stage]);stages.append(row)
        lines.append(' '.join(format(x,'.17g') for x in row['target_total']+row['scale']+row['new_total'])+' '+counts[stage])
    lines[0]+=' '+format(scale,'.17g');(folder/'sa_replay.dat').write_text('\n'.join(lines)+'\n');contract['input_sha256']=sha(folder/'sa_replay.dat')
    command=['taskset','-c','5','nice','-n','19',str(binary),'[NativeSavedSAProjection3D]','--reporter','compact'];contract['command']=command
    record.write_text(json.dumps({'status':'RUNNING','pid':os.getpid(),'contract':contract},indent=2)+'\n')
    start=time.monotonic()
    with (folder/'native.log').open('w') as out:
        code=subprocess.run(command,cwd=folder,env=dict(os.environ,OMP_NUM_THREADS='1',OPENBLAS_NUM_THREADS='1'),stdout=out,stderr=subprocess.STDOUT,timeout=120).returncode
    result={'status':'FAIL','contract':contract,'returncode':code,'elapsed_seconds':time.monotonic()-start,'scope':'Serial reference projection and independent SA lower-bound replay of two saved physical transfers. Production and original all-field acceptance gates unchanged; this does not turn the original failed workflow into a pass.'}
    if code==0:
        rows=list(csv.DictReader((folder/'sa_replay.csv').open()));reports=[]
        for stage,wanted in enumerate(stages):
            part=[r for r in rows if int(r['stage'])==stage];assert part
            report={'stage':stage,'nodes':len(part),'fields':[]};limited=0
            for field in ('n','nm1'):
                f=0 if field=='n' else 1;raw=bounded=saved=F(0);nodeError=0
                for r in part:
                    v=F(float(r['volume']));a,b,c=map(float,(r['raw_'+field],r['bounded_'+field],r['saved_'+field]));assert b==max(1e-16,a)
                    raw+=v*F(a);bounded+=v*F(b);saved+=v*F(c);nodeError=max(nodeError,abs(b-c)/scale);limited+=a<1e-16 and abs(a-1e-16)>1e-26
                rawError=abs(float(raw)-wanted['target_total'][f])/wanted['scale'][f];finalError=abs(float(bounded)-wanted['new_total'][f])/wanted['scale'][f]
                assert rawError<1e-12 and finalError<1e-11 and nodeError<2e-10
                report['fields'].append(dict(field=field,raw_integral_residual=rawError,bounded_integral_residual=finalError,maximum_node_residual=nodeError,relative_clipping_change=float(bounded-raw)/wanted['scale'][f]))
            assert limited==wanted['limited'];report['limited']=limited;reports.append(report)
        result['status']='PASS_SA_BOUND_DIAGNOSIS';result['stages']=reports
    assert sha(binary)==contract['test_binary_sha256']
    result['artifacts']={p.name:sha(p) for p in folder.iterdir() if p.is_file()};record.write_text(json.dumps(result,indent=2)+'\n');print(result['status'],flush=True)
    assert code==0
if __name__=='__main__':main()
