"""Freeze existing baseline/candidate packages and comparison tools; no build, MPI or submission."""
from pathlib import Path
import argparse,json,shutil,sys
from common import sha,verify
ROOT=Path(__file__).resolve().parents[2]
HERE=Path(__file__).resolve().parent
sys.path.insert(0,str(ROOT/'integration_evidence/native_cluster_3d_core_v1'))
from run import verify_package

DONOR='Common/src/adaptation/CNativeDonor3D.cpp'
SOURCE_PINS={
  "original": {
    "Common/src/adaptation/CNativeDonor3D.cpp": "a632198f977cedb9ec3a1da38059989a93ac9f1ee143a9ad7dcc692f262753af",
    "Common/include/adaptation/CNativeDonor3D.hpp": "e5f5c63617460388a0d13c189e879b2690f21184adf9888e2b528459b8b6374a",
    "Common/include/adaptation/CNativeEngine3D.hpp": "ae054867710f57dc5d1569b4b94995d37073f00b737423e97a0379f8e792c13c",
    "UnitTests/Common/adaptation/CNativeDistributed3D_tests.cpp": "d28d5e9d045326ece57af1e8392963c59ee614fbbcbe0a4b2e35a8e563aed73c",
    "Common/src/adaptation/CNativeEngine3D.cpp": "c4e9c8a49a2ba7c74529f57bf6cdc43382113db5d25628cbc8c88b1d4e8cdecc",
    "Common/src/adaptation/CNativeRemesher3D.cpp": "125b51b38421c67a38373494fd93a80c30959c2ab014f635065704a5c629135b"
  },
  "routing": {
    "Common/src/adaptation/CNativeDonor3D.cpp": "af9787bd400f22bdd4942992d9a1d3ec8f043ed8c7ae73cfdf1ddd30f01f70f5",
    "Common/include/adaptation/CNativeDonor3D.hpp": "5550f94a78cc5005ab7b7baa2bb9645bdc906625463984082430dc2da7685943",
    "Common/include/adaptation/CNativeEngine3D.hpp": "524cae568ab3602d735a540702373ba5e1dc42e7ac6fff4a3b61d9d4bc4f4327",
    "UnitTests/Common/adaptation/CNativeDistributed3D_tests.cpp": "9799ae5adf7a80347b5bc0d8769dd7dd113c385c4cb36d78da956dba39b51b24",
    "Common/src/adaptation/CNativeEngine3D.cpp": "c4e9c8a49a2ba7c74529f57bf6cdc43382113db5d25628cbc8c88b1d4e8cdecc",
    "Common/src/adaptation/CNativeRemesher3D.cpp": "125b51b38421c67a38373494fd93a80c30959c2ab014f635065704a5c629135b"
  },
  "packed": {
    "Common/src/adaptation/CNativeDonor3D.cpp": "e475835ebbbff7656621b2c7405638e11930c5f2faafb2819f765356d362cf8a",
    "Common/include/adaptation/CNativeDonor3D.hpp": "5c978c7a20f5057ad6554aaab9f0447894228df72f940ce9490292bd0c6ced5c",
    "Common/include/adaptation/CNativeEngine3D.hpp": "524cae568ab3602d735a540702373ba5e1dc42e7ac6fff4a3b61d9d4bc4f4327",
    "UnitTests/Common/adaptation/CNativeDistributed3D_tests.cpp": "fcc3d7af4d8b55a6d5df5ec209116627d723824475261d1669984a53bf5f500b",
    "Common/src/adaptation/CNativeEngine3D.cpp": "c4e9c8a49a2ba7c74529f57bf6cdc43382113db5d25628cbc8c88b1d4e8cdecc",
    "Common/src/adaptation/CNativeRemesher3D.cpp": "125b51b38421c67a38373494fd93a80c30959c2ab014f635065704a5c629135b"
  },
  "box": {
    "Common/src/adaptation/CNativeDonor3D.cpp": "cddffde7fd65bf142665efd27d09c83f5d3354fc1e761844d2349f40e07a7813",
    "Common/include/adaptation/CNativeDonor3D.hpp": "eb758657b4f3c8df93d5dff55ffb3bb7de8a4245438294d7a9904bb12f8ac3aa",
    "Common/include/adaptation/CNativeEngine3D.hpp": "6ccdad9dcd1d89c26b6a4babe927132b5db07b41b4be9d69b898d9de2fd58eb2",
    "UnitTests/Common/adaptation/CNativeDistributed3D_tests.cpp": "1efc2932a389489f5383349f598163f1c10146450b9b73fb265fe4c8641062b1",
    "Common/src/adaptation/CNativeEngine3D.cpp": "61be0d25a092d060d36d6d869986bd60e6ae11d288a9ecb844ac529bf8ee4477",
    "Common/src/adaptation/CNativeRemesher3D.cpp": "be4cea094d700684856d9520441c33e9c7c84924be81fe0745985c52a507533f"
  }
}
EXPECTED={'baseline':SOURCE_PINS['original'][DONOR],'candidate':SOURCE_PINS['routing'][DONOR]}
CHANGED=set(SOURCE_PINS['original'])

def matched_sources(manifests):
    versions={}
    for side in ('baseline','candidate'):
        actual=manifests[side].get('source_sha256',{})
        version=next((name for name,pins in SOURCE_PINS.items() if actual.get(DONOR)==pins[DONOR]),None)
        if version is None or any(actual.get(name)!=digest for name,digest in SOURCE_PINS[version].items()):
            raise ValueError('Wrong '+side+' source pins; use the documented checkpoints')
        versions[side]=version
    if (versions['baseline'],versions['candidate']) not in (('original','routing'),('original','packed'),('routing','packed'),('original','box'),('routing','box'),('packed','box')):
        raise ValueError('Unsupported or reversed source comparison')
    old,new=(manifests[side]['source_sha256'] for side in ('baseline','candidate'))
    expected={name for name in CHANGED if SOURCE_PINS[versions['baseline']][name]!=SOURCE_PINS[versions['candidate']][name]}
    if set(old)!=set(new) or {n for n in old if old[n]!=new[n]}!=expected:
        raise ValueError('Unrelated source changes; this is not the declared query comparison')
    options=[]
    for side in ('baseline','candidate'):
        values={r['name']:r['value'] for r in manifests[side].get('build_options',[]) if r['name']!='prefix'}
        if not values:raise ValueError('Missing compiled build options')
        options.append(values)
    if options[0]!=options[1]:raise ValueError('Build options differ; retain matching compiler/MPI/MMG/CGNS options')
    return versions

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--baseline',type=Path,required=True);p.add_argument('--candidate',type=Path,required=True)
    p.add_argument('--output',type=Path,required=True)
    a=p.parse_args();baseline=a.baseline.resolve();candidate=a.candidate.resolve();out=a.output.resolve()
    out.relative_to(ROOT)
    if out.exists():raise FileExistsError('Keep the old comparison; choose another output')
    manifests={side:verify_package(path) for side,path in [('baseline',baseline),('candidate',candidate)]}
    variants=matched_sources(manifests)
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
    record=dict(status='PREPARED_NOT_VALIDATED',source_variants=variants,packages={side:dict(manifest_sha256=sha(out/side/'manifest.json'),binary_sha256=manifests[side]['files_sha256']['bin/test_driver']) for side in manifests},
                files_sha256={str(f.relative_to(out)):sha(f) for f in out.rglob('*') if f.is_file()},
                scope='Regular closed copies of two existing immutable native3D packages and comparison tools. Preparation is not correctness, timing, scaling or affordability evidence.')
    (out/'comparison_manifest.json').write_text(json.dumps(record,indent=2)+'\n');verify(out)
    print('Frozen comparison:',out,'No build, MPI or scheduler launch.')
if __name__=='__main__':main()
