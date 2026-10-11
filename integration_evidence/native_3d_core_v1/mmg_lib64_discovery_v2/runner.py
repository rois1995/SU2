from pathlib import Path
import hashlib,json,os,shutil,subprocess,sys,time
root=Path(__file__).resolve().parents[3];folder=Path(__file__).resolve().parent
sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()
text=(root/'meson.build').read_text();start=text.index('# MMG (in-memory');end=text.index('\nmel_dep =',start);block=text[start:end]
assert "dirs: [mmg_root / 'lib', mmg_root / 'lib64']" in block
assert "[scotch_root / 'lib', scotch_root / 'lib64']" in block
project=folder/'project';project.mkdir();(project/'meson.build').write_text("project('mmg_directory_check','cpp', default_options:['cpp_std=c++17'])\nsu2_deps=[]\nsu2_cpp_args=[]\n"+block)
options=(root/'meson_options.txt').read_text().splitlines();(project/'meson_options.txt').write_text('\n'.join(line for line in options if line.startswith(tuple("option('"+n+"'" for n in ('enable-mmg','mmg_root','mmg_scotch_root'))))+'\n')
inputs={str(p):sha(p) for p in (Path('/usr/local/lib/libmmg.a'),Path('/home/rausa/Software/scotch/lib/libscotch.a'),Path('/home/rausa/Software/scotch/lib/libscotcherrexit.a'))}
sources={n:sha(root/n) for n in ('meson.build','meson_options.txt','externals/meson/meson.py','externals/meson/mesonbuild/coredata.py')};cases=[];status='FAIL';reason=''
env=dict(os.environ,NINJA='/tmp/native-build-tools/ninja',TMPDIR=str(folder),OMP_NUM_THREADS='1',OPENBLAS_NUM_THREADS='1',MKL_NUM_THREADS='1',PYTHONDONTWRITEBYTECODE='1')
try:
 for name,libdir,broken in (('lib','lib',False),('lib64','lib64',False),('broken_lib64','lib64',True)):
  case=folder/name;case.mkdir();prefix=case/'prefix';(prefix/libdir).mkdir(parents=True);(prefix/'include').mkdir()
  shutil.copytree('/usr/local/include/mmg',prefix/'include/mmg')
  mmg=prefix/libdir/'libmmg.a'
  if broken:mmg.write_bytes(b'!<arch>\n')
  else:shutil.copyfile('/usr/local/lib/libmmg.a',mmg)
  scotch=case/'scotch';(scotch/libdir).mkdir(parents=True)
  for archive in ('libscotch.a','libscotcherrexit.a'):shutil.copyfile(Path('/home/rausa/Software/scotch/lib')/archive,scotch/libdir/archive)
  args=[sys.executable,str(root/'externals/meson/meson.py'),'setup',str(case/'build'),str(project),'-Denable-mmg=true','-Dmmg_root='+str(prefix),'-Dmmg_scotch_root='+str(scotch)]
  start=time.monotonic()
  with (case/'configure.stdout').open('w') as out,(case/'configure.stderr').open('w') as err:p=subprocess.run(args,env=env,stdout=out,stderr=err,timeout=60)
  stdout=(case/'configure.stdout').read_text();log=case/'build/meson-logs/meson-log.txt';raw=log.read_text();shutil.copyfile(log,case/'meson-log.txt')
  link_commands=[line for line in raw.splitlines() if 'Command line:' in line and str(mmg) in line]
  assert link_commands, 'Probe did not use the intended installed library'
  if broken:assert p.returncode!=0 and 'Cannot link MMG from' in stdout
  else:
   assert p.returncode==0 and any('Checking if "MMG library"' in line and 'links: YES' in line for line in stdout.splitlines())
   assert any(str(scotch/libdir/'libscotch.a') in line for line in link_commands)
  cases.append(dict(name=name,directory=libdir,broken=broken,exit_code=p.returncode,seconds=time.monotonic()-start,command=args,link_commands=link_commands,mmg_sha256=sha(mmg)))
 assert all(sha(root/n)==h for n,h in sources.items()) and all(sha(Path(n))==h for n,h in inputs.items())
 status='PASS'
except Exception as e:reason=str(e);print(reason,flush=True)
finally:
 v=dict(status=status,reason=reason,cases=cases,source_sha256=sources,input_sha256=inputs,scope='Actual Meson configure/compiler-link checks of the exact production MMG dependency block. Both lib and lib64 select the copied real combined static MMG and explicit SCOTCH archives; empty lib64 archive must fail the MMG2D/3D link guard. No SU2 rebuild, MPI, remeshing, cluster execution or performance claim.')
 v['artifact_sha256']={str(p.relative_to(folder)):sha(p) for p in [Path(__file__).resolve(),*project.iterdir(),*(p for case in ('lib','lib64','broken_lib64') for p in (folder/case).glob('*.txt')),*folder.glob('*/configure.std*')] if p.is_file()}
 (folder/'validation.json').write_text(json.dumps(v,indent=2)+'\n');print(status,folder,flush=True)
raise SystemExit(status!='PASS')
