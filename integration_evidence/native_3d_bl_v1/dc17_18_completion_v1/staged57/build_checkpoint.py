"""One-worker checkpoint build with frozen inputs and process-tree resource receipts."""
import json, os, subprocess, sys, time
from pathlib import Path
ROOT=Path(__file__).resolve().parent
sys.path.insert(0,str(ROOT.parent/'diagnostic56'))
from run import sha, save
name='build'+str(int(sys.argv[1]))
assert not (ROOT/(name+'_status.json')).exists(), 'Inspect the previous owned handle before retrying'
paths=set(json.loads((ROOT/'build4_source_pins.json').read_text()))
paths.add(str(Path(__file__).resolve()))
paths.update(str(ROOT.parents[3]/p) for p in (
    'Common/include/adaptation/CNativeAtlas3D.hpp',
    'Common/include/adaptation/CNativeCurvedAtlas3D.hpp',
    'Common/include/adaptation/CNativeCanonicalTrace3D.hpp',
    'Common/include/adaptation/CNativeTraceCoverage3D.hpp',
    'Common/include/adaptation/CNativeReferenceTriangulation3D.hpp',
    'Common/include/adaptation/CNativeGeometryBackend3D.hpp',
    'Common/src/adaptation/CNativeAtlas3D.cpp',
    'Common/src/adaptation/CNativeCurvedAtlas3D.cpp',
    'Common/src/adaptation/CNativeCanonicalTrace3D.cpp',
    'Common/src/adaptation/CNativeTraceCoverage3D.cpp',
    'Common/src/adaptation/CNativeReferenceTriangulation3D.cpp',
    'Common/src/adaptation/CNativeGeometryBackend3D.cpp',
    'UnitTests/Common/adaptation/CNativeAtlas3D_tests.cpp',
    'UnitTests/Common/adaptation/CNativeCurvedAtlas3D_tests.cpp',
    'UnitTests/Common/adaptation/CNativeCanonicalTrace3D_tests.cpp',
    'UnitTests/Common/adaptation/CNativeTraceCoverage3D_tests.cpp',
    'UnitTests/Common/adaptation/CNativeReferenceTriangulation3D_tests.cpp',
    'UnitTests/Common/adaptation/CNativeGeometryBackend3D_tests.cpp',
    'meson.build', 'Common/src/meson.build', 'Common/src/adaptation/meson.build',
    'SU2_CFD/src/meson.build', 'UnitTests/meson.build'))
pins={p:sha(p) for p in sorted(paths)}
save(ROOT/(name+'_source_pins.json'),pins)
command=json.loads((ROOT/'build4_command.json').read_text())
command['argv'].extend(['-k','0','SU2_CFD/src/SU2_CFD'])
command['environment']['NINJA']=command['argv'][3]
env=os.environ.copy();env.update(command['environment'])
save(ROOT/(name+'_command.json'),command)
start=time.time()
with (ROOT/(name+'.log')).open('w') as log, (ROOT/(name+'_resources.jsonl')).open('w') as receipt:
    p=subprocess.Popen(command['argv'],env=env,stdout=log,stderr=subprocess.STDOUT)
    save(ROOT/(name+'_status.json'),dict(state='running',pid=p.pid,start=start,pinned_files=len(pins)))
    print(name,'RUNNING',p.pid,'pinned_files',len(pins),flush=True)
    while p.poll() is None:
        pending=[p.pid];seen=set();rows=[]
        while pending:
            pid=pending.pop()
            if pid in seen:continue
            seen.add(pid)
            try:
                proc=Path('/proc')/str(pid)
                pending.extend(map(int,(proc/'task'/str(pid)/'children').read_text().split()))
                rows.append(dict(pid=pid,rss=int((proc/'statm').read_text().split()[1])*os.sysconf('SC_PAGE_SIZE'),name=(proc/'comm').read_text().strip()))
            except (FileNotFoundError,ProcessLookupError):pass
        receipt.write(json.dumps(dict(elapsed=time.time()-start,processes=rows,summed_rss=sum(r['rss'] for r in rows)))+'\n');receipt.flush()
        time.sleep(3)
    code=p.wait()
changed=[p for p,h in pins.items() if sha(p)!=h]
result=dict(state='terminal',exit_code=code,seconds=time.time()-start,pins_verified=not changed,changed=changed)
save(ROOT/(name+'_status.json'),result);print(json.dumps(result),flush=True)
raise SystemExit(code or bool(changed))
