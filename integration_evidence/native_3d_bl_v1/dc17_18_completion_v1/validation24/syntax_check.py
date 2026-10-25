import json,shlex,subprocess
from pathlib import Path
root=Path.cwd();build=root/'integration_evidence/build-integrated-v2'
rows=json.loads((build/'compile_commands.json').read_text())
files=['CNativeBoundaryParameters3D.cpp','CNativeAffineReference.cpp','CNativeBoundary3D.cpp','CNativeImport3D.cpp','CBoundaryLayerMetric.cpp','CNativeAffineReference_tests.cpp','BoundaryLayerMetric_tests.cpp']
for name in files:
 row=next(r for r in rows if r['file'].endswith('/'+name));args=shlex.split(row['command']);command=[];i=0
 while i<len(args):
  a=args[i];i+=1
  if a in ('-o','-MF','-MT','-MQ'):i+=1;continue
  if a in ('-c','-MD','-MMD','-g'):continue
  command.append(a)
 print('SYNTAX',name,flush=True);subprocess.run(command+['-fsyntax-only'],cwd=build,check=True)
