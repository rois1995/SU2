"""Prepare one isolated BP translation-unit rebuild against frozen build494 inputs."""
import difflib,hashlib,json,os,shlex,shutil,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parent;R=ROOT.parent;REPO=R.parents[3]
BASE=REPO/'integration_evidence/build-integrated-v3-currenthost';BUILD=ROOT/'test_build'
def sha(path):
    with Path(path).open('rb') as f:return hashlib.file_digest(f,'sha256').hexdigest()
def save(path,data):path.write_text(json.dumps(data,indent=2)+'\n')
assert not (ROOT/'build_plan.json').exists(),'Prepare exactly once'
pins=json.loads((R/'build494_source_pins.json').read_text());assert all(sha(p)==h for p,h in pins.items())
program=BASE/'SU2_CFD/src/SU2_CFD';assert sha(program)=='0311b5a2a4f871bd82f48cd7cef3eac49a932cd1d90012bbc7e3c0d887e74ee6'
bp=REPO/'Common/src/adaptation/CNativeBoundaryParameters3D.cpp';original=bp.read_text();parent=subprocess.check_output(['git','show','76f10c8310^:Common/src/adaptation/CNativeBoundaryParameters3D.cpp'],text=True)
start='  auto tighten=[&](const std::vector<Id>& support,double upper) {';end='  };\n  bool direct='
def block(text):
    a=text.index(start);b=text.index(end,a)+len('  };\n');return a,b,text[a:b]
a,b,current=block(original);old=block(parent)[2];assert 'const auto ceiling=2*state.GeometryTolerance();' in old and 'SurfaceBoundPurpose::ErrorMagnitude' not in old
patched=original[:a]+old+original[b:];assert patched[:a]==original[:a] and patched[a+len(old):]==original[b:]
source=BUILD/'source/Common/src/adaptation/CNativeBoundaryParameters3D.cpp';source.parent.mkdir(parents=True);source.write_text(patched)
(BUILD/'source/Common/include').symlink_to(REPO/'Common/include',target_is_directory=True)
(source.parent/'CNativePredicates3D_internal.hpp').symlink_to(bp.parent/'CNativePredicates3D_internal.hpp')
(ROOT/'tighten_diagnostic.patch').write_text(''.join(difflib.unified_diff(original.splitlines(True),patched.splitlines(True),fromfile='build494/Common/src/adaptation/CNativeBoundaryParameters3D.cpp',tofile='diagnostic/Common/src/adaptation/CNativeBoundaryParameters3D.cpp')))
rows=json.loads((BASE/'compile_commands.json').read_text());row=next(r for r in rows if r['file'].endswith('/CNativeBoundaryParameters3D.cpp'));raw_compile=shlex.split(row['command']);orig_object=(BASE/row['output']).resolve();obj=BUILD/row['output'];obj.parent.mkdir(parents=True,exist_ok=True)
compiler=Path(shutil.which(raw_compile[0])).resolve();version=subprocess.check_output([str(compiler),'--version'],text=True).splitlines()[0];comment=subprocess.check_output(['readelf','-p','.comment',str(orig_object)],text=True);assert version.split(' ',1)[1] in comment
compile_cmd=[str(compiler)];index=1
while index<len(raw_compile):
    token=raw_compile[index]
    if token.startswith('-I'):
        path=Path(token[2:]);compile_cmd.append('-I'+str(path if path.is_absolute() else (BASE/path).resolve()))
    elif token in ('-MQ','-MF','-o'):
        compile_cmd.extend([token,str(obj)+('.d' if token=='-MF' else '')]);index+=1
    elif token=='-c':compile_cmd.extend(['-c',str(source)]);index+=1
    else:compile_cmd.append(token)
    index+=1
ninja=REPO/'integration_evidence/build-integrated-v2/tools/ninja';commands=subprocess.check_output([str(ninja),'-C',str(BASE),'-t','commands','SU2_CFD/src/SU2_CFD'],text=True).splitlines();link_raw=shlex.split(commands[-1]);archive=BASE/'Common/src/libSU2AccurateSum.a';new_archive=BUILD/'Common/src/libSU2AccurateSum.a';new_archive.parent.mkdir(parents=True,exist_ok=True)
archive_line=next(c for c in commands if 'gcc-ar csrDT Common/src/libSU2AccurateSum.a ' in c);ar_raw=shlex.split(archive_line.split('&&',1)[1]);assert ar_raw[1]=='csrDT';members=[(BASE/x).resolve() for x in ar_raw[3:]];assert members.count(orig_object)==1
archive_cmd=[shutil.which(ar_raw[0]),ar_raw[1],str(new_archive),*[str(obj if m==orig_object else m) for m in members]]
new_program=BUILD/'SU2_CFD/src/SU2_CFD';new_program.parent.mkdir(parents=True,exist_ok=True);link=[str(compiler)];link_inputs=set();index=1
while index<len(link_raw):
    token=link_raw[index]
    if token=='-o':link.extend(['-o',str(new_program)]);index+=1
    elif token.startswith('-'):link.append(token)
    else:
        path=Path(token);path=path if path.is_absolute() else (BASE/path).resolve();assert path.is_file(),path;link_inputs.add(path);link.append(str(new_archive if path==archive else path))
    index+=1
frozen={program,BASE/'build.ninja',BASE/'compile_commands.json',BASE/'.ninja_log',BASE/'.ninja_deps',*link_inputs}
for lib in link_inputs:
    if lib.open('rb').read(8)==b'!<thin>\n':
        for member in subprocess.check_output(['ar','t',str(lib)],text=True).splitlines():
            m=Path(member);assert m.is_absolute() and m.is_file();frozen.add(m)
inputs=[{'path':str(f),'sha256':sha(f),'bytes':f.stat().st_size,'mtime_ns':f.stat().st_mtime_ns} for f in sorted(frozen) if f.is_file()]
save(ROOT/'baseline_build_inputs.json',{'scope':'All original link inputs and thin archive members, pinned binary, and Ninja/compile metadata; all read only','files':inputs})
save(ROOT/'build_plan.json',{'baseline_build':str(BASE),'test_build':str(BUILD),'baseline_BP_sha256':sha(bp),'diagnostic_BP_sha256':sha(source),'source_difference':'Only tighten lambda from parent76f10c8310; branch BP untouched','compiler':str(compiler),'compiler_version':version,'original_object_compiler_comment':comment,'original_compile_argv':raw_compile,'compile_argv':compile_cmd,'archive_argv':archive_cmd,'original_link_argv':link_raw,'link_argv':link,'source_path':str(source),'new_binary':str(new_program),'working_directory':str(BUILD),'unchanged_source_pin_count':len(pins),'baseline_binary_sha256':sha(program)})
assert sha(bp)==pins[str(bp)]
print('Prepared isolated build; frozen original inputs',len(inputs),'bytes',sum(v['bytes'] for v in inputs),'diagnostic lambda diff only')
