"""Freeze an up-to-date configured native3D test driver on the login host; no build or scheduler launch."""
from pathlib import Path
import argparse,json,os,shlex,shutil,subprocess
from run import sha,verify_package
ROOT=Path(__file__).resolve().parents[2]
PACK=Path(__file__).resolve().parent
TOOLS=('audit_cavities.py','audit_coupled.py','audit_geometry_bridge.py','audit_lifecycle.py','audit_envelope.py','audit_restart.py','export_envelope_vtu.py')

def closed(path):
    path=Path(path).resolve();path.relative_to(ROOT)
    if not path.is_file():raise FileNotFoundError(path)
    return path

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build-dir',type=Path,default=ROOT/'build-native')
    parser.add_argument('--output',type=Path,default=PACK/'prepared_v1');parser.add_argument('--ninja',default='ninja')
    parser.add_argument('--mpip-library',type=Path,help='Optional MPI-compatible shared mpiP library; copied and pinned inside the bundle')
    parser.add_argument('--mpip-license',type=Path,help='Complete license/notices from the same mpiP installation')
    options=parser.parse_args()
    if bool(options.mpip_library)!=bool(options.mpip_license):raise ValueError('Supply both --mpip-library and --mpip-license')
    build=options.build_dir.resolve();build.relative_to(ROOT)
    destination=options.output.resolve();destination.relative_to(ROOT)
    if destination.exists():raise FileExistsError('Keep the previous bundle; choose a new --output')
    binary=closed(build/'UnitTests/test_driver');ninja=closed(build/'build.ninja')
    env=dict(os.environ,LC_ALL='C')
    dry=subprocess.run([options.ninja,'-C',str(build),'-n','UnitTests/test_driver'],capture_output=True,text=True,env=env)
    if dry.returncode or 'no work to do' not in dry.stdout:
        raise RuntimeError('Build is not current. Rebuild UnitTests/test_driver first.\n'+dry.stdout+dry.stderr)
    build_options=json.loads(closed(build/'meson-info/intro-buildoptions.json').read_text())
    selected={entry['name']:entry['value'] for entry in build_options}
    for name in ('enable-tests','enable-mmg','enable-cgns','enable-normal'):
        if selected.get(name) is not True:raise ValueError('Required build option is not enabled: '+name)
    if '-DHAVE_MPI' not in ninja.read_text():raise ValueError('Configured driver requires MPI')
    line=next(s for s in ninja.read_text().splitlines() if s.startswith('build UnitTests/test_driver:'))
    objects=[closed(build/p) for p in shlex.split(line) if p.endswith('.o')]
    if len(objects)!=68:raise ValueError('Expected the complete configured68-object driver; review scope before using another selection')
    libraries={};members={}
    for name in shlex.split(line):
        if not name.endswith('.a'):continue
        p=Path(name);p=p if p.is_absolute() else build/p
        if not p.is_file():raise FileNotFoundError(p)
        libraries[str(p)]=sha(p)
        with p.open('rb') as f:thin=f.read(8)==b'!<thin>\n'
        if thin:
            for member in subprocess.check_output(['ar','t',str(p)],text=True).splitlines():
                q=Path(member);q=q if q.is_absolute() else ROOT/q
                members[str(q)]=sha(q)
    sources={}
    for directory in ('Common','SU2_CFD','UnitTests'):
        for p in (ROOT/directory).rglob('*'):
            if p.is_file() and p.suffix in ('.cpp','.hpp','.h','.inl','.tpp'):sources[str(p.relative_to(ROOT))]=sha(closed(p))
    names=['integration_evidence/native_cluster_3d_core_v1/'+n for n in ('run.py','RunNative3DSGE.sh','restore.py','analyze.py','profile_summary.py','mpi_profile.py','submit.py','check_package.py','README.md')]
    names+=['integration_evidence/native_3d_core_v1/'+n for n in TOOLS]
    names+=['TestCases/adaptation/capability/capcheck.py']
    # Keep the active native3D implementation and real driver-fixture recipe inspectable with the compact results.
    names += [str(p.relative_to(ROOT)) for directory in ('Common/include/adaptation','Common/src/adaptation') for p in (ROOT/directory).glob('*3D*') if p.is_file() and p.suffix in ('.cpp','.hpp')]
    names += ['UnitTests/SU2_CFD/adaptation/CNativeImport3D_tests.cpp','UnitTests/Common/adaptation/CNativeBoundary3D_tests.cpp',
              'UnitTests/SU2_CFD/adaptation/CNativeRemesher3D_tests.cpp','UnitTests/Common/adaptation/CNativeDistributed3D_tests.cpp']
    if options.mpip_library:
        with options.mpip_library.open('rb') as f:
            if f.read(4)!=b'\x7fELF':raise ValueError('mpiP library must be an ELF shared library')
        mpip_digest=sha(options.mpip_library)
        license_digest=sha(options.mpip_license)
    destination.mkdir(parents=True)
    for name in sorted(set(names)):
        source=closed(ROOT/name);target=destination/name;target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(source,target)
    (destination/'bin').mkdir();shutil.copyfile(binary,destination/'bin/test_driver');(destination/'bin/test_driver').chmod(0o755)
    if options.mpip_library:
        shutil.copyfile(options.mpip_library,destination/'bin/libmpiP.so')
        shutil.copyfile(options.mpip_license,destination/'mpiP-LICENSE.txt')
        if sha(destination/'bin/libmpiP.so')!=mpip_digest or sha(options.mpip_library)!=mpip_digest or sha(destination/'mpiP-LICENSE.txt')!=license_digest or sha(options.mpip_license)!=license_digest:
            raise ValueError('mpiP library changed during preparation')
    record=dict(status='PREPARED_NOT_VALIDATED',source_sha256=sources,actual_objects_sha256={str(p.relative_to(build)):sha(p) for p in objects},library_sha256=libraries,thin_archive_member_sha256=members,build_ninja_sha256=sha(ninja),build_dry_run=dry.stdout,
                build_options=build_options,scope='Current configured build dependencies and source/binary/member pins. Preparation is not any C++/MPI/numerical PASS. Dynamic MPI/CGNS/MMG libraries must be available on cluster nodes.')
    if sha(binary)!=sha(destination/'bin/test_driver') or any(sha(ROOT/name)!=digest for name,digest in sources.items()) or any(sha(p)!=digest for p,digest in {**libraries,**members}.items()) or any(sha(build/name)!=digest for name,digest in record['actual_objects_sha256'].items()):
        raise ValueError('Build/source changed during preparation; retain this incomplete folder and prepare a new bundle')
    record['files_sha256']={str(p.relative_to(destination)):sha(p) for p in destination.rglob('*') if p.is_file()}
    (destination/'manifest.json').write_text(json.dumps(record,indent=2)+'\n');verify_package(destination)
    print('Frozen package:',destination,'\nNo build, MPI test or scheduler job launched.')
if __name__=='__main__':main()
