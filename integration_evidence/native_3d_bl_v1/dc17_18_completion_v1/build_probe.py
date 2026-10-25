import json,shlex,subprocess,sys
from pathlib import Path
root=Path(__file__).resolve().parents[3]
build=root/'integration_evidence/build-integrated-v2'
evidence=Path(__file__).resolve().parent
rows=json.loads((build/'compile_commands.json').read_text())
row=next(r for r in rows if r['file'].endswith('/BoundaryLayerMetric_tests.cpp'))
args=shlex.split(row['command']); out=[];i=0
while i<len(args):
    a=args[i];i+=1
    if a in ('-o','-MF','-MQ','-MT'):
        i+=1;continue
    if a in ('-MD','-MMD','-c'):continue
    if a.endswith('.cpp'):continue
    if a.startswith('-O') or a=='-g':continue
    out.append(a)
destination=evidence/(sys.argv[1] if len(sys.argv)>1 else 'validation7')
destination.mkdir(parents=True,exist_ok=True)
obj=destination/'probe.o';binary=destination/'probe'
probe_source=Path(sys.argv[2]).resolve() if len(sys.argv)>2 else evidence/'probe_transition.cpp'
out+=['-I'+str(root),'-I'+str(root/'Common/include'),'-O1','-g0','-c',str(probe_source),'-o',str(obj)]
link=shlex.split((evidence/'link_command.txt').read_text())
link=[link[0],'-o',str(binary),str(obj)]+[a for a in link[3:] if not a.endswith('.o')]
(destination/'probe_commands.json').write_text(json.dumps([out,link],indent=2)+'\n')
for command in (out,link):subprocess.run(command,cwd=build,check=True)
