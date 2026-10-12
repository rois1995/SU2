"""Freeze the current standard driver and link one isolated rotating directory on the login host."""
from pathlib import Path
import argparse
import json
import os
import shlex
import shutil
import subprocess
import sys
from common import sha, verify

ROOT = Path(__file__).resolve().parents[2]
HERE = Path(__file__).resolve().parent
CORE = ROOT/'integration_evidence/native_cluster_3d_core_v1'
sys.path.insert(0, str(CORE))
from run import command, verify_package


def directory_recipe(build, source, output, mapfile):
    entry = next(row for row in json.loads((build/'compile_commands.json').read_text())
                 if row['file'].endswith('/adaptation/CNativeDistributed3D.cpp'))
    args = shlex.split(entry['command'])
    compile_args = []
    i = 0
    while i < len(args):
        if args[i] in ('-o', '-MQ', '-MF', '-c'):
            i += 2
        elif args[i] == '-MD':
            i += 1
        else:
            compile_args.append(args[i])
            i += 1
    obj = source.with_suffix('.o')
    compile_args += ['-I'+str(ROOT/'Common/include'), '-o', str(obj), '-c', str(source)]
    lines = (build/'build.ninja').read_text().splitlines()
    start = next(i for i, line in enumerate(lines) if line.startswith('build UnitTests/test_driver: cpp_LINKER '))
    objects = shlex.split(lines[start].split('cpp_LINKER ', 1)[1].split(' | ', 1)[0])
    if len(objects) != 68:
        raise ValueError('Require the complete configured68-object driver')
    link_args = shlex.split(next(line.split('=', 1)[1] for line in lines[start+1:] if line.startswith(' LINK_ARGS =')))
    link = [args[0], str(obj), *objects, '-o', str(output), '-Wl,-Map,'+str(mapfile), *link_args]
    return obj, compile_args, link


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build-dir', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--ninja', default='ninja')
    parser.add_argument('--mpip-library', type=Path)
    parser.add_argument('--mpip-license', type=Path)
    a = parser.parse_args()
    build, out = a.build_dir.resolve(), a.output.resolve()
    build.relative_to(ROOT)
    out.relative_to(ROOT)
    if out.exists():
        raise FileExistsError('Preserve the old bundle; choose a new output')
    if bool(a.mpip_library) != bool(a.mpip_license):
        raise ValueError('Supply both mpiP library and license')
    qualified_path = ROOT/'integration_evidence/native_3d_core_v1/completion_diagnostic_build_v1/validation.json'
    qualified = json.loads(qualified_path.read_text())
    prototype = ROOT/'integration_evidence/native_3d_core_v1/rotating_reservation_probe_v1/directory_rotating.cpp'
    full = json.loads((ROOT/'integration_evidence/native_3d_core_v1/rotating_reservation_full_build_v1/validation.json').read_text())
    if qualified['status'] != 'PASS' or full['status'] != 'PASS':
        raise ValueError('Missing qualified normal/isolated source checkpoint')
    variant_key = 'integration_evidence/native_3d_core_v1/rotating_reservation_full_build_v1/directory_rotating.cpp'
    if sha(prototype) != full['source_sha256'][variant_key]:
        raise ValueError('Isolated variant differs from qualified protocol/full regression')
    if any(sha(ROOT/name) != digest for name, digest in qualified['source_sha256'].items()
           if name.startswith(('Common/', 'SU2_CFD/', 'UnitTests/'))):
        raise ValueError('Compiled source differs from this qualified scheduler comparison')
    out.mkdir(parents=True)
    print('Working and frozen package folder:', out, flush=True)
    env = dict(os.environ, OMP_NUM_THREADS='1', OPENBLAS_NUM_THREADS='1', MKL_NUM_THREADS='1', PYTHONDONTWRITEBYTECODE='1')
    steps = []
    status = 'FAIL'
    failure = None
    try:
        for args, label in [(['uptime'], 'host_load'), (['free', '-m'], 'host_memory')]:
            steps.append(command(args, out, label, env, 30))
        cmd = [sys.executable, str(CORE/'prepare.py'), '--build-dir', str(build), '--ninja', a.ninja,
               '--output', str(out/'baseline')]
        if a.mpip_library:
            cmd += ['--mpip-library', str(a.mpip_library), '--mpip-license', str(a.mpip_license)]
        stage = command(cmd, out, 'freeze_standard', env, 900)
        steps.append(stage)
        if stage['exit_code'] or stage.get('timeout'):
            raise RuntimeError('Standard package preparation failed; logs retained')
        normal = verify_package(out/'baseline')
        shutil.copytree(out/'baseline', out/'candidate')
        isolated = out/'candidate/isolated'
        isolated.mkdir()
        # Only the include location changes; the qualified reservation implementation remains exact.
        source = isolated/'directory_rotating.cpp'
        text = prototype.read_text()
        first = '#include "../../../Common/include/adaptation/CNativeDistributed3D.hpp"'
        if text.count(first) != 1:
            raise ValueError('Unexpected qualified variant include layout')
        source.write_text(text.replace(first, '#include "adaptation/CNativeDistributed3D.hpp"'))
        shutil.copyfile(HERE/'prepare.py', isolated/'prepare.py')
        obj, compile_args, link_args = directory_recipe(build, source, out/'candidate/bin/test_driver', isolated/'link.map')
        for label, args in [('compile', compile_args), ('link', link_args)]:
            stage = command(['nice', '-n', '19', *args], build, 'reservation_'+label, env, 900)
            for suffix in ('stdout', 'stderr'):
                shutil.move(build/('reservation_'+label+'.'+suffix), isolated/(label+'.'+suffix))
            steps.append(stage)
            if stage['exit_code'] or stage.get('timeout'):
                raise RuntimeError('Isolated '+label+' failed; original evidence retained')
        map_text = (isolated/'link.map').read_text()
        if str(obj) not in map_text or 'adaptation_CNativeDistributed3D.cpp.o' in map_text:
            raise ValueError('Normal directory archive member was unexpectedly linked')
        for name, digest in normal['actual_objects_sha256'].items():
            if sha(build/name) != digest:
                raise ValueError('Normal test object changed during preparation')
        for name, digest in {**normal['library_sha256'], **normal['thin_archive_member_sha256']}.items():
            if sha(Path(name)) != digest:
                raise ValueError('Production archive/member changed during preparation')
        if any(sha(ROOT/name) != digest for name, digest in normal['source_sha256'].items()):
            raise ValueError('Production source changed during preparation')
        if sha(build/'UnitTests/test_driver') != normal['files_sha256']['bin/test_driver']:
            raise ValueError('Normal binary changed during preparation')
        object_pin = dict(bytes=obj.stat().st_size, sha256=sha(obj))
        obj.unlink()
        candidate = dict(normal, isolated_variant=dict(source_sha256=sha(source), qualified_prototype_sha256=sha(prototype),
                         removed_object=object_pin, phases=steps, scope='One isolated rotating-directory object linked before unchanged configured68test objects/archives; normal directory member excluded. Include location only changed. Preparation is not MPI/numerical/performance qualification.'))
        candidate['files_sha256'] = {str(p.relative_to(out/'candidate')):sha(p) for p in (out/'candidate').rglob('*') if p.is_file() and p.name != 'manifest.json'}
        (out/'candidate/manifest.json').write_text(json.dumps(candidate, indent=2)+'\n')
        verify_package(out/'candidate')
        if candidate['files_sha256']['bin/test_driver'] == normal['files_sha256']['bin/test_driver']:
            raise ValueError('Scheduler binaries unexpectedly identical')
        (out/'suite').mkdir()
        for name in ('common.py', 'campaign.py', 'analyze.py', 'RunReservationComparisonSGE.sh', 'README.md'):
            shutil.copyfile(HERE/name, out/'suite'/name)
        for name in ('run.py', 'mpi_profile.py', 'profile_summary.py'):
            p = out/'shared/core'/name
            p.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(CORE/name, p)
        for name in ('audit_cavities.py', 'audit_coupled.py', 'audit_geometry_bridge.py', 'audit_lifecycle.py', 'audit_envelope.py', 'audit_restart.py'):
            p = out/'shared/audits'/name
            p.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(ROOT/'integration_evidence/native_3d_core_v1'/name, p)
        record = dict(status='PREPARED_NOT_VALIDATED', comparison_kind='rotating_reservation',
                      packages={side:dict(manifest_sha256=sha(out/side/'manifest.json'), binary_sha256=verify_package(out/side)['files_sha256']['bin/test_driver']) for side in ('baseline','candidate')},
                      files_sha256={str(p.relative_to(out)):sha(p) for p in out.rglob('*') if p.is_file()},
                      scope='Closed regular runtime inputs for same-build standard versus isolated rotating scheduling. Each trajectory requires a fresh full audit. No production change, correctness, timing, scaling or affordability claim from preparation.')
        (out/'comparison_manifest.json').write_text(json.dumps(record, indent=2)+'\n')
        verify(out)
        status = 'PREPARED_NOT_VALIDATED'
    except Exception as error:
        failure = str(error)
        raise
    finally:
        if status == 'FAIL':
            (out/'preparation_failure.json').write_text(json.dumps(dict(status=status, failure=failure, phases=steps), indent=2)+'\n')
    print('Prepared:', out, 'No MPI run or scheduler submission.', flush=True)


if __name__ == '__main__':
    main()
