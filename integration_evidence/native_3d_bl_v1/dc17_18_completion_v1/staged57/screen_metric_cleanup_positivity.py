"""Exact coordinate obstruction screen only; never certifies a replacement."""
from pathlib import Path
import hashlib,json,time
import meshio
import numpy as np
from prepare_boundary202 import exact_positive
R=Path(__file__).resolve().parent
A=R/'m6_runs/euler_build262_compact_capture_mach_only_real_cfd50k'
sha=lambda p:hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()
def main():
    started=time.monotonic();selection=A/'post_refinement_cleanup_selection_v1.json'
    output=A/'post_refinement_cleanup_positivity_v1.json';assert not output.exists()
    chosen=json.loads(selection.read_text());vtu=A/'inspection/rejected_mesh.vtu'
    assert sha(vtu)==chosen['input_hashes_after'][str(vtu.resolve())]
    before={str(p):sha(p) for p in (selection,vtu,Path(__file__).resolve(),R/'prepare_boundary202.py')}
    mesh=meshio.read(vtu);P=mesh.points;T=mesh.cells[0].data;ids=mesh.point_data['global_node_id'].ravel();cids=mesh.cell_data['global_cell_id'][0].ravel();order=np.argsort(cids)
    assert np.all(ids[1:]>ids[:-1]);rows=[]
    for case in chosen['bounded_selected_contexts']:
        wanted=np.asarray(case['context_cell_ids'],dtype=cids.dtype);indices=order[np.searchsorted(cids[order],wanted)];assert np.array_equal(cids[indices],wanted)
        context={int(cids[i]):[dict(id=int(ids[n]),xyz=P[n].tolist()) for n in T[i]] for i in indices}
        assert len(context)==case['four_star_union_size']<=64
        for nodes in context.values():assert exact_positive(nodes)
        a,b=case['selected_edge']
        for removed,kept in ((a,b),(b,a)):
            active={cid:nodes for cid,nodes in context.items() if any(n['id']==removed for n in nodes)}
            assert len(active)==case['vertex_star_sizes'][case['nodes'].index(removed)]
            assert len(active)==np.count_nonzero(np.any(ids[T]==removed,axis=1))
            vertex=dict(id=kept,xyz=P[np.searchsorted(ids,kept)].tolist());assert int(ids[np.searchsorted(ids,kept)])==kept
            fresh={cid:[vertex if n['id']==removed else n for n in nodes] for cid,nodes in active.items() if not any(n['id']==kept for n in nodes)}
            nonpositive=[cid for cid,nodes in fresh.items() if not exact_positive(nodes)]
            joined={cid:nodes for cid,nodes in context.items() if cid not in active};joined.update(fresh)
            keys=[tuple(sorted(n['id'] for n in nodes)) for nodes in joined.values()]
            rows.append(dict(role=case['role'],core=case['id'],removed=removed,retained=kept,old_active_cells=len(active),remaining_active_cells=len(fresh),removed_cells=len(active)-len(fresh),nonpositive_candidate_cell_ids=nonpositive,duplicate_whole_context_cells=len(keys)-len(set(keys)),exact_endpoint_coordinate_screen_passed=bool(fresh) and not nonpositive and len(keys)==len(set(keys)),geometry_association_metric_height_interface_contact_admission='NOT TESTED',replacement_admissible=False))
    after={p:sha(p) for p in before};assert before==after
    result=dict(time=time.time(),seconds=time.monotonic()-started,rows=rows,input_hashes_before=before,input_hashes_after=after,scope='Independent exact binary64-coordinate determinant screen using existing helper. A nonpositive retained tetrahedron proves direct endpoint-collapse obstruction. Passing this screen does not establish topology/interface, geometry, metric, shape, height or contact admissibility; no production operator or candidate publication executed.')
    output.write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(dict(seconds=result['seconds'],directions=len(rows),passes=sum(r['exact_endpoint_coordinate_screen_passed'] for r in rows),nonpositive_blocked=sum(bool(r['nonpositive_candidate_cell_ids']) for r in rows),duplicates=sum(bool(r['duplicate_whole_context_cells']) for r in rows))))
if __name__=='__main__':main()
