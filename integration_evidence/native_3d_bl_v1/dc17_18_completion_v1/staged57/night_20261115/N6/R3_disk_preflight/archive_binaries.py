"""Preserve terminal C and D binary backups in verified compressed archives."""
import hashlib
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

SPACE = Path(__file__).resolve().parent
sys.path.insert(0, str(SPACE.parents[0]))
from run_cut import N6, cut


def pack(source, digest, temporary):
    assert source.is_file() and not source.is_symlink() and cut.sha(source) == digest
    archive = source.with_name(source.name + '.zst')
    assert not archive.exists()
    subprocess.run(['zstd', '-q', '-3', '-T1', '-o', str(archive), str(source)], check=True)
    with subprocess.Popen(['zstd', '-dc', str(archive)], stdout=subprocess.PIPE) as decoder:
        restored = hashlib.file_digest(decoder.stdout, 'sha256').hexdigest()
        assert decoder.wait() == 0 and restored == digest
    archive.chmod(0o444)
    duplicate = temporary / source.parent.name / source.name
    duplicate.parent.mkdir(exist_ok=True)
    shutil.copy2(source, duplicate)
    assert cut.sha(duplicate) == digest
    pending = source.with_name(source.name + '.archive-link-pending')
    pending.symlink_to(duplicate)
    assert cut.sha(pending) == digest
    os.replace(pending, source)
    assert source.is_symlink() and cut.sha(source) == digest
    result = {'original_path': str(source), 'raw_sha256': digest, 'bytes': duplicate.stat().st_size,
              'mode': oct(duplicate.stat().st_mode & 0o777), 'compressed_path': str(archive),
              'compressed_sha256': cut.sha(archive), 'compressed_bytes': archive.stat().st_size,
              'active_recovery_copy': str(duplicate), 'archived_roundtrip_sha256': restored}
    cut.save(SPACE / f'{source.parent.name}_{source.name}.json', result)
    return result


def main():
    assert Path('/proc/1/comm').read_text().strip() == 'systemd'
    for handle in (N6 / 'continue_campaign_launch.json', N6 / 'Optimizer_campaign_launch.json',
                   N6.parent / 'N18/entry_boundary_launch.json'):
        assert cut.terminal(cut.read(handle))
    failure = cut.PHASE / 'N6Optimizer/validation'
    wait = cut.read(failure / 'GQ_wait.json')
    assert wait['terminal'] and cut.terminal(wait['controller'])
    assert 'less than 2 GB free on evidence filesystem' in (failure / 'GQ_outer.log').read_text()
    assert not (failure / 'cases/C10_GQ_577').exists()
    subprocess.run([cut.PYTHON, str(cut.PREFLIGHT), str(SPACE / 'archive_preflight.json')], check=True)
    temporary = Path(tempfile.mkdtemp(prefix='su2_verified_binary_recovery_', dir='/tmp'))
    before = shutil.disk_usage(cut.REPO).free
    records = []
    for phase, build in (('C', 570), ('D', 572)):
        folder = cut.PHASE / f'phase_{phase}/qualified_build{build}'
        manifest = cut.read(folder / 'manifest.json')
        assert manifest['build'] == build
        for binary in manifest['binaries'].values():
            path = Path(binary['local_copy'])
            assert path.parent == folder
            records.append(pack(path, binary['sha256'], temporary))
    after = shutil.disk_usage(cut.REPO).free
    assert after > 2_000_000_000
    cut.save(SPACE / 'space_recovery.json', {'before_free_bytes': before, 'after_free_bytes': after,
             'source_or_gate_changed': False, 'archives': records})
    print(f'Evidence disk free before {before:,} B and after {after:,} B.', flush=True)


if __name__ == '__main__':
    main()
