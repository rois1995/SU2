"""Check SGE comparison ordering and preflight failures using fake qsub only."""
from pathlib import Path
import hashlib, json, os, shutil, subprocess, sys, tempfile

helper = Path(sys.argv[1] if len(sys.argv)>1 else Path(__file__).with_name('SubmitMetricComparison.sh'))
sha = lambda p: hashlib.sha256(p.read_bytes()).hexdigest()
with tempfile.TemporaryDirectory(prefix='native-metric-submit-check-') as tmp:
    root = Path(tmp)
    baseline, candidate = root/'baseline', root/'candidate'
    pins = {}
    for label, repo in [('baseline',baseline),('candidate',candidate)]:
        pack = repo/'integration_evidence/native_cluster_campaign_v1'
        pack.mkdir(parents=True)
        (repo/'source.cpp').write_text(label)
        (pack/'source_pins.json').write_text(json.dumps({'source.cpp':sha(repo/'source.cpp')}))
        (pack/'input_manifest.json').write_text(json.dumps({'files':{}}))
        pins[label+'_source_pins_sha256'] = sha(pack/'source_pins.json')
        for name in ('build-native/SU2_CFD/src/SU2_CFD','build-native/UnitTests/test_driver'):
            p=repo/name;p.parent.mkdir(parents=True,exist_ok=True);p.write_text('fake binary')
    comparison = candidate/'integration_evidence/native_cluster_metric_comparison_v1'
    comparison.mkdir(parents=True)
    script=comparison/'SubmitMetricComparison.sh';shutil.copy2(helper,script)
    pins['input_manifest_sha256']=sha(candidate/'integration_evidence/native_cluster_campaign_v1/input_manifest.json')
    (comparison/'comparison.json').write_text(json.dumps(pins))
    binpath=root/'bin';binpath.mkdir()
    qsub=binpath/'qsub'
    qsub.write_text('#!'+sys.executable+'\n'+'''import json,os,sys
from pathlib import Path
p=Path(os.environ['FAKE_QSUB_LOG'])
rows=json.loads(p.read_text()) if p.exists() else []
rows.append(dict(cwd=os.getcwd(),args=sys.argv[1:],id=str(10001+len(rows))))
p.write_text(json.dumps(rows))
print(rows[-1]['id'])
''')
    qsub.chmod(0o755)
    log=root/'qsub.json'
    env=dict(os.environ,PATH=str(binpath)+':'+os.environ['PATH'],FAKE_QSUB_LOG=str(log),ADAP_WORKERS='4:3:2')
    env.pop('HOLD_JID',None)
    command=['bash',str(script),str(baseline),'4','3','2']
    result=subprocess.run(command,env=env,capture_output=True,text=True)
    assert result.returncode==0,(result.stdout,result.stderr)
    rows=json.loads(log.read_text());assert len(rows)==24
    for index,row in enumerate(rows):
        args=row['args']
        assert '-terse' in args and args[args.index('-pe')+1:args.index('-pe')+3]==['mpi','4']
        if index==0:assert '-hold_jid' not in args
        else:assert args[args.index('-hold_jid')+1]==rows[index-1]['id']
        rep=index//8
        first='baseline' if rep%2==0 else 'candidate'
        expected=first if index%2==0 else ('candidate' if first=='baseline' else 'baseline')
        assert Path(row['cwd']).name==expected
        assert 'REPEATS=1,ADAPT_EVENTS=2,ADAP_WORKERS=4:3:2' in args[args.index('-v')+1]
    receipt=next((candidate/'ClusterResults/submissions').glob('*.tsv'))
    assert len(receipt.read_text().splitlines())==25
    for args in ([str(candidate),'4'],[str(baseline),'0']):
        bad=subprocess.run(['bash',str(script),*args],env=env,capture_output=True,text=True)
        assert bad.returncode!=0 and len(json.loads(log.read_text()))==24
    (baseline/'source.cpp').write_text('changed')
    bad=subprocess.run(command,env=env,capture_output=True,text=True)
    assert bad.returncode!=0 and len(json.loads(log.read_text()))==24
    (baseline/'source.cpp').write_text('baseline')
    source=candidate/'source.cpp';source.unlink()
    outside_source=root/'outside_source';outside_source.write_text('candidate');source.symlink_to(outside_source)
    bad=subprocess.run(command,env=env,capture_output=True,text=True)
    assert bad.returncode!=0 and len(json.loads(log.read_text()))==24
    source.unlink();source.write_text('candidate')
    binary=candidate/'build-native/SU2_CFD/src/SU2_CFD';binary.unlink()
    outside=root/'outside';outside.write_text('outside');binary.symlink_to(outside)
    bad=subprocess.run(command,env=env,capture_output=True,text=True)
    assert bad.returncode!=0 and len(json.loads(log.read_text()))==24
print('PASS: 24 fake submissions; chained holds/order/receipts; invalid ranks, sources, shared root and escaping source/binary reject before submission. No SGE jobs submitted.')
