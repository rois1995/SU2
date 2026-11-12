#!/usr/bin/env python3
"""Supplementary test of exact extracted source statements, not a C++ semantics proof."""
import hashlib
import json
import re
import subprocess
from pathlib import Path
from check_source import ROOT, check_source

START = '    /*--- Keep only the tangential part of the momentum residuals. ---*/'
END = '    /*--- Jacobian contribution for implicit integration. ---*/'


def extract(text):
    if text.count(START) != 1 or text.count(END) != 1:
        raise ValueError('Ambiguous source boundaries; manual mapping review required')
    return text.split(START, 1)[1].split(END, 1)[0]


def wrapper(name, body):
    return ('void ' + name + '(unsigned nDim, const double* UnitNormal, '
            'double gridNormalVel, Block& LinSysRes) {\n'
            '  const unsigned iPoint = 0, iVel = 1;\n'
            '  const bool compressibleProjection = true;\n' + body + '\n}\n')


def main():
    manifest = check_source()
    new = extract((ROOT / 'source/SU2_CFD/include/solvers/CFVMFlowSolverBase.inl').read_text())
    old = extract((ROOT / 'source/CFVMFlowSolverBase.before.inl').read_text())
    assert 'normalRes -= gridNormalVel * LinSysRes(iPoint, 0);' in new
    assert 'normalRes -= gridNormalVel * LinSysRes(iPoint, 0);' not in old
    marker = '    if (compressibleProjection) {'
    assert new.count(marker) == 1
    accumulation, correction = new.split(marker, 1)
    helper = ('double normalMomentum(unsigned nDim, const double* UnitNormal, Block& LinSysRes) {\n'
              '  const unsigned iPoint = 0, iVel = 1;\n' + accumulation +
              '  return normalRes;\n}\n')
    factored = ('    su2double normalRes = normalMomentum(nDim, UnitNormal, LinSysRes);\n'
                + marker + correction)
    assert accumulation in helper and factored.endswith(marker + correction)
    renamed = re.sub(r'\bnormalRes\b', 'wallResidual', new)
    assert re.sub(r'\bwallResidual\b', 'normalRes', renamed) == new
    # Extraction must fail closed if a boundary is missing or duplicated.
    for malformed in (new, START + START + END):
        try:
            extract(malformed)
        except ValueError:
            pass
        else:
            raise AssertionError('extractor accepted ambiguous source')
    header = r'''
#include <array>
#include <cassert>
#include <cmath>
#include <cstring>
#include <iostream>
#include <random>
using su2double = double;
struct Block {
  std::array<double,5> v{};
  double& operator()(unsigned, unsigned i) { return v.at(i); }
};
'''
    harness = r'''
int main() {
  const double n[3] = {1,0,0};
  Block bad, good;
  bad.v = {0.5,0,0,0,0}; good = bad;
  legacy(2,n,2,bad); repaired(2,n,2,good);
  const double oldDefect = (2+bad.v[1]) - 2*(1+bad.v[0]);
  const double newDefect = (2+good.v[1]) - 2*(1+good.v[0]);
  assert(oldDefect == -1 && newDefect == 0);
  std::mt19937 gen(1729);
  std::uniform_real_distribution<double> sample(-10,10);
  unsigned cases = 0;
  for (unsigned dim : {2u,3u}) {
    for (unsigned trial=0; trial<500; ++trial) {
      double normal[3]{}, wall[3]{}, len2=0, g=0;
      for (unsigned i=0;i<dim;++i) { normal[i]=sample(gen); wall[i]=sample(gen); len2+=normal[i]*normal[i]; }
      for (unsigned i=0;i<dim;++i) { normal[i]/=std::sqrt(len2); g+=wall[i]*normal[i]; }
      Block input;
      for (unsigned i=0;i<dim+2;++i) input.v[i]=sample(gen);
      Block actual=input, renamedActual=input, factoredActual=input;
      repaired(dim,normal,g,actual);
      refactored(dim,normal,g,renamedActual);
      helperFactored(dim,normal,g,factoredActual);
      assert(std::memcmp(actual.v.data(),renamedActual.v.data(),sizeof(actual.v))==0);
      assert(std::memcmp(actual.v.data(),factoredActual.v.data(),sizeof(actual.v))==0);
      double a=0, defect=-actual.v[0]*g;
      for (unsigned i=0;i<dim;++i) a+=(input.v[i+1]-input.v[0]*wall[i])*normal[i];
      assert(actual.v[0]==input.v[0]);
      for (unsigned i=0;i<dim;++i) {
        const double expected=input.v[i+1]-a*normal[i];
        assert(std::abs(actual.v[i+1]-expected)<1e-11);
        defect+=actual.v[i+1]*normal[i];
      }
      assert(std::abs(actual.v[dim+1]-(input.v[dim+1]-g*a))<1e-10);
      assert(std::abs(defect)<1e-11);
      ++cases;
    }
  }
  std::cout << "source residual slice: old defect=-1, repaired defect=0; "
            << cases << " finite 2D/3D cases; renamed and helper-factored slices bitwise equal\n";
}
'''
    out = ROOT / 'artifacts/source-experiment'
    out.mkdir(parents=True, exist_ok=True)
    cpp = out / 'residual_slice.cpp'
    cpp.write_text(header + wrapper('legacy', old) + wrapper('repaired', new) +
                   wrapper('refactored', renamed) + helper + wrapper('helperFactored', factored) + harness)
    compile_cmd = ['g++', '-std=c++17', '-O2', '-fno-fast-math', '-ffp-contract=off',
                   str(cpp), '-o', str(out / 'residual_slice')]
    subprocess.run(compile_cmd, check=True, timeout=120)
    result = subprocess.run([str(out / 'residual_slice')], check=True, timeout=30,
                            text=True, capture_output=True)
    metadata = {'source_commit': manifest['commit'], 'compile_command': compile_cmd,
                'compiler': subprocess.check_output(['g++','--version'], text=True).splitlines()[0],
                'extracted_sha256': hashlib.sha256(new.encode()).hexdigest(),
                'generated_cpp_sha256': hashlib.sha256(cpp.read_bytes()).hexdigest(),
                'result': result.stdout,
                'scope': 'Extracted residual statements only; wrappers, extraction and tests unverified.'}
    (out / 'result.json').write_text(json.dumps(metadata, indent=2)+'\n')
    print(result.stdout, end='')


if __name__ == '__main__':
    main()
