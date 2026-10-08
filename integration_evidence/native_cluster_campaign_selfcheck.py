"""Exercise cluster orchestration/collection with a fake launcher; no MPI or CFD jobs."""
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
from unittest.mock import patch
import run_native_cluster_campaign as campaign


def main():
    with tempfile.TemporaryDirectory(prefix='native-cluster-selfcheck-') as temporary:
        root=Path(temporary);pack=root/'integration_evidence/native_cluster_campaign_v1'
        fixture=pack/'inputs/actual_bl_to_euler';fixture.mkdir(parents=True)
        (fixture/'run.cfg').write_text('SOLVER= EULER\nMESH_FILENAME= input.su2\nTIME_ITER= 9\nADAP_FREQ= 3\nADAP_NATIVE_RANKS= 1\n')
        (fixture/'input.su2').write_text('fake mesh; not CFD evidence\n')
        (pack/'source_pins.json').write_text('{}')
        manifest={str(p.relative_to(pack)):dict(sha256=hashlib.sha256(p.read_bytes()).hexdigest()) for p in fixture.iterdir()}
        (pack/'input_manifest.json').write_text(json.dumps(dict(files=manifest)))
        binary=root/'fake_solver';binary.write_text('fake binary\n')
        hosts=root/'machinefile';hosts.write_text('localhost\n')
        launchers=root/'launchers';launchers.mkdir()
        fake=launchers/'mpirun'
        fake.write_text('''#!/usr/bin/env python3
from pathlib import Path
import os,re,sys
if '--version' in sys.argv:print('fake MPI for orchestration check');raise SystemExit
cfg=Path('run.cfg').read_text()
assert len(re.findall(r'^ADAP_NATIVE_RANKS=',cfg,re.M))==1
assert 'TIME_ITER= 6' in cfg and 'MESH_FILENAME= input.su2' in cfg
for name in ('mesh_00003.su2','mesh_00003.su2.native_ref','flow_00001.vtu','flow_00002.vtu','solution_00001.dat','solution_00002.dat','flow_00005.vtu','solution_00005.dat'):
 if os.environ.get('FAKE_MISSING') and name=='flow_00005.vtu':continue
 Path(name).write_text('fake output; not mesh validation')
print('Exit Success')
''');fake.chmod(0o755)
        subprocess.run(['git','init','-q',str(root)],check=True)
        # A revision is needed for provenance, not to validate the fake source.
        subprocess.run(['git','-C',str(root),'-c','user.name=Selfcheck','-c','user.email=selfcheck@example.invalid','commit','-q','--allow-empty','-m','fake orchestration check'],check=True)
        script=Path(__file__).resolve().parent/'native_cluster_campaign_v1/RunNativeSGE.sh'
        (pack/'RunNativeSGE.sh').write_text(script.read_text())
        launch_env=dict(os.environ,SGE_O_WORKDIR=str(root),JOB_ID='launcher_missing',NSLOTS='4',
                        PATH_GCC='/unused',LD_LIBRARY_PATH_GCC='/unused',MACHINEFILE_PATH=str(root/'missing_hosts'))
        failed=subprocess.run(['bash',str(script)],env=launch_env,capture_output=True,text=True)
        receipt=root/'ClusterResults/jobs/launcher_missing_launcher'
        assert failed.returncode==2 and (receipt/'exit_code.txt').read_text().strip()=='2'
        assert 'Missing scheduler machinefile' in (receipt/'launcher.err').read_text()
        assert (receipt/'source_revision.txt').read_text().strip()
        assert (receipt/'RunNativeSGE.sh').read_bytes()==script.read_bytes()
        # The launch failed before any solver could run, but its diagnostics are downloadable.
        args=['cluster','--kind','actual_bl_to_euler','--binary',str(binary),'--machinefile',str(hosts),'--ranks','4','--workers','4:2','--repeat','2','--events','1']
        with patch.object(campaign,'ROOT',root),patch.dict(os.environ,dict(PATH=str(launchers)+':'+os.environ['PATH'],JOB_ID='selfcheck')),patch.object(sys,'argv',args):
            assert campaign.main()==0
        cases=list((root/'ClusterResults/cases').iterdir());assert len(cases)==6
        configs=[(case/'run.cfg').read_text() for case in cases]
        assert sum('ADAP_NATIVE_RANKS= 4' in cfg for cfg in configs)==4
        assert sum('ADAP_NATIVE_RANKS= 2' in cfg for cfg in configs)==2
        assert all(json.loads((case/'collection_manifest.json').read_text())['missing']==[] for case in cases)
        with patch.object(campaign,'ROOT',root),patch.dict(os.environ,dict(PATH=str(launchers)+':'+os.environ['PATH'],JOB_ID='selfcheck_missing',FAKE_MISSING='1')),patch.object(sys,'argv',args[:args.index('--repeat')]+['--repeat','1','--events','1']):
            assert campaign.main()==1
        summary=json.loads((root/'ClusterResults/jobs/selfcheck_missing_actual_bl_to_euler/summary.json').read_text())
        assert summary['status']=='FAILED_CASES' and len(summary['failures'])==3
        print('Cluster orchestration selfcheck PASS: early SGE failure diagnostics, repeated worker pairs, config overrides, selected collection and missing-data failure. Fake launcher only.')


if __name__=='__main__':main()
