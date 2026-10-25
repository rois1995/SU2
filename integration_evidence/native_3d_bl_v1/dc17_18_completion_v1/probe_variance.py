"""Instrument the house estimator and reuse the M6 probe loader for coarsest-field profile diagnostics."""
import hashlib
import json
import shlex
import subprocess
import sys
from pathlib import Path

evidence = Path(__file__).resolve().parent
root = evidence.parents[2]
build = root / 'integration_evidence/build-integrated-v2'
destination = evidence / sys.argv[1]
destination.mkdir(parents=True, exist_ok=True)
source = root / 'SU2_CFD/src/adaptation/CNativeComplexity3D.cpp'
original = source.read_text()
needle = 'batchLocal[q.batch]+=y;local[3]+=y;local[4]+=y*y;local[5]+=y!=0;++counts[0];++statistics.correctionEvaluations;'
assert original.count(needle) == 1
instrument = '''
      double ratio=INFINITY,normalSize=INFINITY;
      for(const auto& sample:q.geometry) {
        const double r=SU2_TYPE::GetValue(sample.sample.distance/layers.GetLayer(sample.wall).thickness);
        if(r<ratio){ratio=r;normalSize=SU2_TYPE::GetValue(sample.sample.hn);}
      }
      auto baseMetric=sensorMetric,fullMetric=sensorMetric;
      if(target)for(const auto& sample:q.baseGeometry)target->base->ApplySample(sample.wall,sample.sample,baseMetric,core,nullptr,false);
      for(const auto& sample:q.geometry)layers.ApplySample(sample.wall,sample.sample,fullMetric,core,nullptr,false);
      std::fprintf(stderr,"VARIANCE %.17g %llu %u %.17g %.17g %.17g %.17g %.17g %.17g %.17g %.17g %.17g %.17g %.17Lg %.17g\\n",
          layers.GetOuterNormal().parameter,(unsigned long long)statistics.samples,q.batch,q.point.x,q.point.y,q.point.z,
          ratio,q.inversePDF,double(y),a,LogDensity(baseMetric),LogDensity(fullMetric),b,q.weight,normalSize);
'''
copy = destination / 'complexity_diagnostic.cpp'
copy.write_text('#include <cstdio>\n' + original.replace(needle, needle + instrument))
rows = json.loads((build / 'compile_commands.json').read_text())
row = next(r for r in rows if r['file'].endswith('/CNativeComplexity3D.cpp'))
args = shlex.split(row['command'])
command = []
i = 0
while i < len(args):
    arg = args[i]
    i += 1
    if arg in ('-o', '-MF', '-MQ', '-MT'):
        i += 1
        continue
    if arg in ('-MD', '-MMD', '-c', '-g') or arg.endswith('.cpp'):
        continue
    command.append(arg)
obj = destination / 'complexity_diagnostic.o'
command += ['-I' + str(source.parent), '-g0', '-c', str(copy), '-o', str(obj)]
loader = (evidence / 'probe_transition.cpp').read_text()
split = '    const size_t count=donors.size();FrozenField frozen'
assert loader.count(split) == 1
probe = destination / 'probe_budget.cpp'
probe.write_text(loader.split(split)[0].replace('#undef CHECK', '#undef CHECK\n#include "SU2_CFD/include/adaptation/CNativeBLTarget3D.hpp"') + r'''
    core=1/std::pow(double(config.GetAdap_Hmax()),2);
    for(auto& m:original){m=BL::Tensor{};for(unsigned j=0;j<3;++j)m.m[j][j]=core;}
    std::vector<DonorCell>().swap(donors);
    auto layers=BL::FromNativeReference(state,config);
    layers->ConfigureOuterNormal(config.GetAdap_BL_ProtectDistance(),config.GetAdap_BL_OuterGrowthMax());
    auto target=SU2NativeBLPipeline3D::MakeTargetPolicy(config,state,core,-1,layers);
    CNativeComplexity3D estimate(*layers,std::move(source),original.size(),size_t(2)<<30,8192,20261017,true);
    std::ofstream out(argv[4]);out<<std::setprecision(17);
    for(unsigned stage=0;;++stage) {
      for(unsigned index=0;index<=8;index+=(stage?4:1)) {
        const double parameter=index/8.;layers->SelectOuterNormal(parameter);target->Update(core,config.GetAdap_Hgrad());
        const auto result=estimate.Assess(original,core,true,target.get());
        const auto& stats=estimate.GetStatistics();
        out<<"{\"stage\":"<<stage<<",\"profile\":"<<parameter<<",\"samples\":"<<result.samples
           <<",\"inside\":"<<result.inside<<",\"value\":"<<result.global.value<<",\"uncertainty\":"<<result.global.uncertainty
           <<",\"sensor\":"<<result.sensorQuadrature<<",\"correction\":"<<result.correction
           <<",\"mandatory\":"<<result.mandatory.value<<",\"direct_checks\":"<<result.directChecks
           <<",\"retained_bytes\":"<<stats.retainedBytes<<",\"support_candidates\":"<<stats.supportCandidates<<"}\n"<<std::flush;
      }
      std::string reason;if(!estimate.Refine(reason)){std::cerr<<"REFINEMENT_STOP "<<reason<<'\n';break;}
    }
  }catch(const std::exception& e){std::cerr<<e.what()<<'\n';exitCode=1;}
  SU2_MPI::Finalize();return exitCode;
}
''')
subprocess.run([sys.executable, str(evidence / 'build_probe.py'), sys.argv[1], str(probe)], check=True)
subprocess.run(command, cwd=build, check=True)
link = json.loads((destination / 'probe_commands.json').read_text())[1]
link.insert(4, str(obj))
subprocess.run(link, cwd=build, check=True)
(destination / 'diagnostic_build.json').write_text(json.dumps({
    'original': str(source), 'source_sha256': hashlib.sha256(original.encode()).hexdigest(),
    'diagnostic_sha256': hashlib.sha256(copy.read_bytes()).hexdigest(),
    'loader_sha256': hashlib.sha256(loader.encode()).hexdigest(),
    'probe_sha256': hashlib.sha256(probe.read_bytes()).hexdigest(),
    'compile': command, 'link': link,
}, indent=2) + '\n')
