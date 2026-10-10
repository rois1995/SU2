"""Prepare a strict same-executable matrix using the already-tested diagnostic binary."""
import json
from pathlib import Path
import prepare

EXPECTED = '5827ffbe8b7ac9dd0336d000a33a8d649975579f1cd57f623a440a00fb87019e'


def cpp(name):
    return (name.startswith(('Common/','SU2_CFD/','UnitTests/')) and
            Path(name).suffix in ('.cpp','.hpp','.h','.inl','.tpp')) or name in (
                'meson.build','UnitTests/meson.build','Common/src/adaptation/meson.build')


def check_build(old):
    if old['binaries']['profile']['sha256']!=EXPECTED or prepare.sha(prepare.closed(prepare.CANDIDATE))!=EXPECTED:
        raise ValueError('Recover the tested job582344 executable; this matrix requires the same binary, not a rebuild')
    original = {k:v for k,v in old['files_sha256'].items() if cpp(k)}
    current = {k:v for k,v in prepare.pins().items() if cpp(k)}
    if not original or original!=current:
        raise ValueError('C++/build source changed since the diagnostic; validate the new build before using this matrix')
    metadata = prepare.closed(prepare.ROOT / 'build-native/meson-info/intro-buildoptions.json')
    if json.loads(metadata.read_text())!=old['build_options']:
        raise ValueError('Build options changed since the diagnostic')


if __name__=='__main__':
    old = json.loads(prepare.closed(prepare.CHECKPOINT).read_text())
    check_build(old)
    prepare.CHECKPOINT = prepare.ROOT / 'build-native/reuse_matrix_checkpoint.json'
    prepare.prepare()
