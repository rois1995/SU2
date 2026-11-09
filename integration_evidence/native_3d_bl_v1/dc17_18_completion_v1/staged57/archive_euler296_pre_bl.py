"""Preserve qualified Euler296 source/binaries before the connected BL source change."""
from pathlib import Path
import hashlib,json,os,shutil,tempfile
R=Path(__file__).resolve().parent;repo=R.parents[3]
a=R/'m6_runs/euler_build296_boundary_reuse_mach_only_real_cfd50k'
assert json.loads((a/'terminal_handles.json').read_text())['all_owned_handles_terminal']
assert json.loads((a/'workflow296_independent_validation.json').read_text())['all_pins_verified']
sha=lambda p:hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()
sources=json.loads((R/'build296_source_pins.json').read_text())
pins=json.loads((a/'pins.json').read_text());assert all(sha(p)==h for p,h in pins.items())
parent=R/'baselines';parent.mkdir(exist_ok=True)
destination=parent/'qualified_euler296_pre_bl_v1';assert not destination.exists()
tmp=Path(tempfile.mkdtemp(prefix='.euler296-',dir=parent));files={}
for name,digest in sources.items():
    relative=Path('source_tree')/Path(name).relative_to(repo)
    target=tmp/relative;target.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(name,target)
    assert sha(target)==digest;files[str(relative)]=digest
for relative in ('SU2_CFD/src/SU2_CFD','UnitTests/test_driver'):
    source=repo/'integration_evidence/build-integrated-v3-currenthost'/relative
    target=tmp/'binaries'/Path(relative).name;target.parent.mkdir(exist_ok=True);shutil.copy2(source,target)
    assert sha(target)==pins[str(source)];files[str(target.relative_to(tmp))]=pins[str(source)]
manifest=dict(scope='Supplementary qualified first296 Euler source/binary archive. Completed293 TWO-event/isolated-restart evidence and both checkpoints remain the primary regression baseline; this archive does not replace them.',original_source_pins=sources,archived_files=files,source_files=len(sources),all_bytes_verified=True)
(tmp/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
assert all(sha(p)==h for p,h in pins.items())
os.rename(tmp,destination)
print(json.dumps({'archive':str(destination),'source_files':len(sources),'binaries':2,'all_bytes_verified':True}))
