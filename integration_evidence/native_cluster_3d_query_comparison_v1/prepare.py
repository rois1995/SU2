"""Freeze existing baseline/candidate packages and comparison tools; no build, MPI or submission."""
from pathlib import Path
import argparse,json,shutil,sys
from common import sha,verify
ROOT=Path(__file__).resolve().parents[2]
HERE=Path(__file__).resolve().parent
sys.path.insert(0,str(ROOT/'integration_evidence/native_cluster_3d_core_v1'))
from run import verify_package

DONOR='Common/src/adaptation/CNativeDonor3D.cpp'
EXPECTED={'baseline':'a632198f977cedb9ec3a1da38059989a93ac9f1ee143a9ad7dcc692f262753af',
          'candidate':'af9787bd400f22bdd4942992d9a1d3ec8f043ed8c7ae73cfdf1ddd30f01f70f5'}
CHANGED={DONOR,'Common/include/adaptation/CNativeDonor3D.hpp','Common/include/adaptation/CNativeEngine3D.hpp',
         'UnitTests/Common/adaptation/CNativeDistributed3D_tests.cpp'}

def matched_sources(manifests):
    for side in ('baseline','candidate'):
        if manifests[side].get('source_sha256',{}).get(DONOR)!=EXPECTED[side]:
            raise ValueError('Wrong '+side+' donor implementation; use the documented checkpoints')
    old,new=(manifests[side]['source_sha256'] for side in ('baseline','candidate'))
    if set(old)!=set(new) or {n for n in old if old[n]!=new[n]}!=CHANGED:
        raise ValueError('Unrelated source changes; this is not the declared routing comparison')
    options=[]
    for side in ('baseline','candidate'):
        values={r['name']:r['value'] for r in manifests[side].get('build_options',[]) if r['name']!='prefix'}
        if not values:raise ValueError('Missing compiled build options')
        options.append(values)
    if options[0]!=options[1]:raise ValueError('Build options differ; retain matching compiler/MPI/MMG/CGNS options')

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--baseline',type=Path,required=True);p.add_argument('--candidate',type=Path,required=True)
    p.add_argument('--output',type=Path,required=True)
    a=p.parse_args();baseline=a.baseline.resolve();candidate=a.candidate.resolve();out=a.output.resolve()
    out.relative_to(ROOT)
    if out.exists():raise FileExistsError('Keep the old comparison; choose another output')
    manifests={side:verify_package(path) for side,path in [('baseline',baseline),('candidate',candidate)]}
    matched_sources(manifests)
    if manifests['baseline']['files_sha256']['bin/test_driver']==manifests['candidate']['files_sha256']['bin/test_driver']:
        raise ValueError('Baseline/candidate binaries are identical')
    if out in (baseline,candidate) or baseline in out.parents or candidate in out.parents:
        raise ValueError('Comparison must not be nested in an input bundle')
    out.mkdir(parents=True)
    for side,path in [('baseline',baseline),('candidate',candidate)]:
        shutil.copytree(path,out/side)
    (out/'suite').mkdir()
    for name in ('common.py','campaign.py','analyze.py','RunQueryComparisonSGE.sh','README.md'):
        shutil.copyfile(HERE/name,out/'suite'/name)
    for name in ('run.py','mpi_profile.py','profile_summary.py'):
        dest=out/'shared/core'/name;dest.parent.mkdir(parents=True,exist_ok=True)
        shutil.copyfile(ROOT/'integration_evidence/native_cluster_3d_core_v1'/name,dest)
    for name in ('audit_cavities.py','audit_coupled.py','audit_geometry_bridge.py','audit_lifecycle.py','audit_envelope.py','audit_restart.py'):
        dest=out/'shared/audits'/name;dest.parent.mkdir(parents=True,exist_ok=True)
        shutil.copyfile(ROOT/'integration_evidence/native_3d_core_v1'/name,dest)
    for side,path in [('baseline',baseline),('candidate',candidate)]:
        verify_package(path);verify_package(out/side)
    record=dict(status='PREPARED_NOT_VALIDATED',packages={side:dict(manifest_sha256=sha(out/side/'manifest.json'),binary_sha256=manifests[side]['files_sha256']['bin/test_driver']) for side in manifests},
                files_sha256={str(f.relative_to(out)):sha(f) for f in out.rglob('*') if f.is_file()},
                scope='Regular closed copies of two existing immutable native3D packages and comparison tools. Preparation is not correctness, timing, scaling or affordability evidence.')
    (out/'comparison_manifest.json').write_text(json.dumps(record,indent=2)+'\n');verify(out)
    print('Frozen comparison:',out,'No build, MPI or scheduler launch.')
if __name__=='__main__':main()
