"""Link the signed DEF1 CConfig cases using the existing reverse-AD Meson objects."""

import argparse
from pathlib import Path
import shlex
import subprocess

from build_fp3 import AD_BUILD, HERE, TOOLS, cut, terminal_build, verify_build, warnings

OBJECTS = ('UnitTests/test_driver_AD.p/Common_adaptation_DEF1Options_tests.cpp.o',
           'UnitTests/test_driver_AD.p/test_driver.cpp.o')
ARCHIVES = ('Common/src/libSU2CommonAD.a', 'Common/src/libSU2AccurateSum.a',
            'externals/cgns/libcgns.a', 'externals/cgns/hdf5/libsu2hdf5.a',
            'externals/metis/libmetis.a', 'externals/parmetis/libparmetis.a')


def link_arguments(binary):
    commands = subprocess.check_output([str(TOOLS / 'ninja'), '-C', str(AD_BUILD),
                                        '-t', 'commands', 'UnitTests/test_driver_AD'], text=True)
    link = shlex.split(commands.splitlines()[-1])
    assert link[1:3] == ['-o', 'UnitTests/test_driver_AD']
    assert all(f in link for f in (*OBJECTS, *ARCHIVES))
    scope = set(OBJECTS)
    link[2] = str(binary)
    return [arg for arg in link if (not arg.startswith('UnitTests/test_driver_AD.p/') or arg in scope)
            and not arg.startswith('SU2_CFD/src/')]


def command_wait(out, args):
    argv = ['nice', '-n', '19', *args]
    cut.save(out / 'build_ad_link_plan.json', {'argv': argv, 'cwd': str(AD_BUILD),
                                              'source_pins_sha256': cut.sha(out / 'source_pins.json')})
    # cut.run_gate runs in the checkout, while Meson's link arguments are relative to AD_BUILD.
    command = ['/usr/bin/env', '-C', str(AD_BUILD), *argv]
    try:
        return cut.run_gate(out, 'build_ad_link', command)
    except RuntimeError:
        return cut.read(out / 'build_ad_link_wait.json')


def build(label):
    direct = verify_build(label)
    out = HERE / f'build{label}'
    full = out / 'ad_full_setup/build_ad_status.json'
    stopped = cut.read(full)
    assert not stopped['passed'] and stopped['wait']['terminal'] and cut.terminal(stopped['wait']['controller'])
    assert 'CBoundaryLayerMetric.cpp' in (full.parent / 'build_ad_outer.log').read_text(errors='replace')
    options = {row['name']: row['value'] for row in cut.read(AD_BUILD / 'meson-info/intro-buildoptions.json')}
    assert options['enable-autodiff'] and options['enable-tests'] and options['enable-mmg']
    compile_wait = terminal_build(out, AD_BUILD, [*OBJECTS, *ARCHIVES], 'build_ad_focus')
    binary = out / 'test_driver_AD'
    wait = compile_wait if compile_wait['returncode'] else command_wait(out, link_arguments(binary))
    verify_build(label)
    all_warnings = warnings(out / 'build_ad_focus_outer.log')
    if (out / 'build_ad_link_outer.log').is_file():
        all_warnings += warnings(out / 'build_ad_link_outer.log')
    receipt = {'passed': wait['returncode'] == 0 and binary.is_file(), 'build': label,
               'source_commit': direct['source_commit'], 'source_pins_sha256': direct['source_pins_sha256'],
               'scope': 'DEF1Options reverse-AD Catch fixture with the existing CommonAD build and main',
               'build_helper_sha256': cut.sha(Path(__file__).resolve()),
               'failed_full_ad_build': {'path': str(full), 'sha256': cut.sha(full)},
               'compile_wait': compile_wait, 'wait': wait, 'binary': str(binary),
               'binary_sha256': cut.sha(binary) if binary.is_file() else None,
               'graph_after': {name: cut.sha(AD_BUILD / name) for name in ('build.ninja', 'compile_commands.json')},
               'warnings': all_warnings}
    cut.save(out / 'build_ad_status.json', receipt)
    return receipt['passed']


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('label', type=int)
    raise SystemExit(not build(parser.parse_args().label))
