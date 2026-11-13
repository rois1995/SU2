"""Reuse latest484physical progress and exact existing residual supports; no native work."""
import json
from pathlib import Path
from assess_surface_transaction470 import sha
R=Path(__file__).resolve().parent
checkpoint=R/'saved_surface484_checkpoint.json';state=json.loads(checkpoint.read_text())
qualification=Path(state['qualification']);a=json.loads(qualification.read_text());assert a['output_cells']==303823 and a['output_nodes']==61550
case=Path(state['source_case']);assert json.loads((case/'terminal_handles_actual.json').read_text())['all_owned_handles_terminal']
for key in ('owned','parameters'):
    item=state[key];assert sha(item['path'])==item['sha256'] and Path(item['path']).stat().st_size==item['bytes']
for item in [state['composed_target'],state['prepared_reference'],*state['all_rank_targets']]:assert sha(item['path'])==item['sha256']
assert sha(state['configuration']['path'])==state['configuration']['sha256']
inventory=R/'surface_residual484_inventory.json';contexts=json.loads(inventory.read_text())
assert contexts['source_owned_sha256']==state['owned']['sha256'] and sha(contexts['contexts'])==contexts['contexts_sha256']
assert [row['complete_endpoint_union_cells'] for row in contexts['inventory']]==[33,10,20]
adapter=R/'surface485_current_state';adapter.mkdir()
for key in ('owned','parameters'):(adapter/f'trajectory_working_{key}.bin').symlink_to(Path(state[key]['path']).resolve())
(adapter/'terminal_handles_actual.json').symlink_to((case/'terminal_handles_actual.json').resolve())
receipt=dict(owned=state['owned'],parameters=state['parameters'],target=state['composed_target'],prepared_reference=state['prepared_reference'],
 contexts=dict(path=str(Path(contexts['contexts']).resolve()),sha256=contexts['contexts_sha256']),all_rank_targets=state['all_rank_targets'],
 capture_adapter=str(adapter.resolve()),stage_height=state['stage_height'],cells=a['output_cells'],nodes=a['output_nodes'],
 source_checkpoint=dict(path=str(checkpoint.resolve()),sha256=sha(checkpoint)),source_qualification=dict(path=str(qualification.resolve()),sha256=sha(qualification)),
 source_inventory=dict(path=str(inventory.resolve()),sha256=sha(inventory)),configuration=state['configuration'],
 purpose='Fixed current residual candidate comparison, no oldP1 or geometry/CFD/metric recomputation.',
 limits=contexts['limitations']+contexts['uncovered_contexts'],final_mesh_accepted=False)
(R/'surface_residual485_inputs.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(dict(cells=receipt['cells'],nodes=receipt['nodes'],context_cells=[33,10,20],stage_height=receipt['stage_height'])))
