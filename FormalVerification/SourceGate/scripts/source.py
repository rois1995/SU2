#!/usr/bin/env python3
"""Select the complete pinned scalar method and emit raw source literals, never a hand-written AST."""
import hashlib
import json
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[1]
REPO = ROOT.parents[1]
COMMIT = 'e0d3ee8d72598767227e62a9be5834a4554532b6'
HEADER = 'SU2_CFD/include/variables/CVariable.hpp'
SIGNATURE = re.compile(r'inline\s+void\s+AddSolution\s*\(\s*unsigned\s+long\s+([A-Za-z_]\w*)\s*,\s*unsigned\s+long\s+([A-Za-z_]\w*)\s*,\s*su2double\s+([A-Za-z_]\w*)\s*\)\s*\{', re.ASCII)


def digest(data):
    return hashlib.sha256(data).hexdigest()


def git_bytes(revision, path):
    return subprocess.check_output(['git', '-C', str(REPO), 'show', revision + ':' + path])


def extract(header):
    # This selector is unverified tooling; exact span and full-header digests are recorded.
    # Comments are masked solely to locate the definition. The selected source is not normalized.
    masked = re.sub(r'/\*.*?\*/|//[^\n]*', lambda m: ' ' * len(m.group()), header, flags=re.S)
    matches = list(SIGNATURE.finditer(masked))
    if len(matches) != 1:
        raise ValueError('Expected exactly one scalar AddSolution definition')
    match = matches[0]
    depth = 1
    end = match.end()
    while end < len(masked) and depth:
        depth += (masked[end] == '{') - (masked[end] == '}')
        end += 1
    if depth:
        raise ValueError('Unclosed scalar method')
    selected = header[match.start():end]
    if '#' in selected:
        raise ValueError('Preprocessor directives are outside the selected subset')
    return match.start(), end, selected


def variants(source):
    if source.count('Solution_Old(iPoint, iVar) + solution') != 1:
        raise ValueError('Review mutation anchors against the selected algorithm')
    return {
        'original': source,
        'subtract': source.replace(' + solution', ' - solution'),
        'currentRead': source.replace('Solution_Old(iPoint, iVar)', 'Solution(iPoint, iVar)'),
        'wrongTarget': source.replace('Solution(iPoint, iVar) =', 'Solution_Old(iPoint, iVar) ='),
        'refactored': '''inline void AddSolution(unsigned long point, unsigned long component, su2double increment) {
    const su2double previous = Solution_Old(point, component);
    Solution(point, component) = previous + increment;
  }''',
    }


def generate(destination):
    header = git_bytes(COMMIT, HEADER)
    decoded = header.decode('utf-8')
    start, end, source = extract(decoded)
    before, after = len(decoded[:start].encode()), len(decoded[:end].encode())
    implementations = variants(source)
    fixture_dir = destination / 'fixtures'
    (destination / 'SU2Source').mkdir(parents=True, exist_ok=True)
    fixture_dir.mkdir(exist_ok=True)
    statements = ['import SU2Source.Parser', '', 'set_option maxRecDepth 100000',
                  'set_option maxHeartbeats 800000', '', 'namespace SU2Source.Generated', '']
    entries = {}
    for name, text in implementations.items():
        (fixture_dir / (name + '.cpp')).write_text(text + '\n')
        statements += [f'def {name}Source : String := ' + json.dumps(text),
                       f'def {name}Program : Program := parsedProgram {name}Source (by decide)',
                       f'theorem {name}_parsed : parse {name}Source = some {name}Program := by decide', '']
        variant_header = header[:before] + text.encode() + header[after:]
        entries[name] = {'method_sha256': digest(text.encode()), 'header_sha256': digest(variant_header)}
    statements.append('end SU2Source.Generated')
    (destination / 'SU2Source/Generated.lean').write_text('\n'.join(statements) + '\n')
    maps = {}
    for path in [HEADER, 'Common/include/containers/C2DContainer.hpp', 'Common/include/code_config.hpp',
                 'SU2_CFD/include/solvers/CFVMFlowSolverBase.inl']:
        data = git_bytes(COMMIT, path)
        maps[path] = {'sha256': digest(data), 'blob': hashlib.sha1(b'blob ' + str(len(data)).encode() + b'\0' + data).hexdigest()}
    manifest = {'source_commit': COMMIT, 'source_files': maps, 'source_span_bytes': [before, after],
                'variants': entries,
                'configuration': 'C++17, LP64, serial passive double; no AD, single precision, volatile or concurrent access',
                'evidence': 'Kernel parses the complete selected source text into the restricted language. Extraction and mapping of C++ member/accessor semantics remain explicit trust boundaries.'}
    (destination / 'source-manifest.json').write_text(json.dumps(manifest, indent=2) + '\n')
    # The selector must never silently pick a duplicate definition or a truncated body.
    for bad in [source + '\n' + source, source[:-1]]:
        try:
            extract(bad)
        except ValueError:
            pass
        else:
            raise AssertionError('Invalid source selection accepted')
    assert extract('/* ' + source + ' */\n' + source)[2] == source
    assert len(set(e['method_sha256'] for e in entries.values())) == len(entries)
    print('Generated source literals for original, three semantic mutants, and local-variable refactor.')


if __name__ == '__main__':
    generate(ROOT)
