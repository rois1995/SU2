"""Actual target frames on saved coordinates: frozen-metric placement/tangent controls, never mesh/metric acceptance."""
from pathlib import Path
import hashlib,json,sys
import numpy as np
import assess_saved_row369 as previous
R=Path(__file__).resolve().parent
LD=np.longdouble
sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()
def factor(matrix):
    m=np.asarray(matrix,dtype=LD);m=m/max(abs(m).flat)
    a=np.sqrt(m[0,0]);b=m[1,0]/a;c=m[2,0]/a
    d=np.sqrt(m[1,1]-b*b);e=(m[2,1]-c*b)/d;f=np.sqrt(m[2,2]-c*c-e*e)
    assert np.isfinite([a,d,f]).all() and min(a,d,f)>0
    return np.array([[a,0,0],[b,d,0],[c,e,f]],dtype=LD)
def placement_control(base,matrix,height,tolerance):
    # Exact optimum for constant M, fixed triangle, and only a parallel-plane height interval.
    l=factor(matrix);base=np.asarray(base,dtype=LD);physical=np.cross(base[1]-base[0],base[2]-base[0]);physical/=np.sqrt(physical@physical)
    y=(base-base[0])@l
    z=np.empty(3,dtype=LD);z[0]=physical[0]/l[0,0];z[1]=(physical[1]-l[1,0]*z[0])/l[1,1];z[2]=(physical[2]-l[2,0]*z[0]-l[2,1]*z[1])/l[2,2]
    conversion=1/np.sqrt(z@z);lo=LD(height)*(1-LD(tolerance))*conversion;hi=LD(height)*(1+LD(tolerance))*conversion
    edges=sum(np.dot(y[j]-y[i],y[j]-y[i]) for i,j in ((0,1),(1,2),(2,0)))
    cross=np.cross(y[1]-y[0],y[2]-y[0]);area=np.sqrt(cross@cross)/2
    assert lo>0 and hi>=lo and area>0 and edges>0
    ideal=np.sqrt(2*edges)/3;chosen=min(hi,max(lo,ideal))
    q=12*np.cbrt(area*chosen)**2/(LD(4)/3*edges+3*chosen*chosen)
    return float(q),float(chosen/conversion),physical

def main():
    case=Path(sys.argv[1]).resolve();status=json.loads((case/'status.json').read_text());assert status['state']=='terminal' and status['exit_code']==0 and status['pins_verified']
    assert json.loads((case/'terminal_handles_actual.json').read_text())['all_owned_handles_terminal']
    command=json.loads((case/'command.json').read_text());env=command['environment'];row_path=Path(env['SU2_NATIVE_BL_SAVED_PATCH']);neighbor_path=Path(env['SU2_NATIVE_BL_SAVED_NEIGHBORS'])
    row_cells,row_faces=previous.read_patch(row_path);neighbor_cells,neighbor_faces=previous.read_patch(neighbor_path)
    union=dict(row_cells)
    for identity,value in neighbor_cells.items():
        if identity in union:assert union[identity][0]==value[0] and np.array_equal(union[identity][1],value[1])
        else:union[identity]=value
    assert len(row_cells)==25 and len(union)==84
    triangle=np.array([[0,0,0],[1,0,0],[LD('.5'),np.sqrt(LD(3))/2,0]],dtype=LD)
    exact,_,_=placement_control(triangle,np.eye(3),np.sqrt(LD(2)/3),0)
    assert abs(exact-1)<1e-14
    reports=[];pins={str(p):sha(p) for p in (row_path,neighbor_path,Path(previous.__file__),case/'command.json',case/'status.json')};shared={}
    for label,cells,faces in [('saved_prepared_repair_before',row_cells,row_faces),('saved_prepared_repair_before_plus_neighbor_star',union,list(set(row_faces+neighbor_faces)))]:
        path=case/(label+'_frames.jsonl');pins[str(path)]=sha(path);frames=[json.loads(line) for line in path.read_text().splitlines()]
        assert {x['cell'] for x in frames}==set(cells);results=[];error=0
        for frame in frames:
            assert frame['complete_target_checkpoint'] and not frame['control_target_only'];ids,xyz=cells[frame['cell']]
            xx,xy,xz,yy,yz,zz=frame['metric'];m=np.array([[xx,xy,xz],[xy,yy,yz],[xz,yz,zz]],dtype=LD)
            q,j=previous.quality(xyz,m);error=max(error,abs(q-frame['q']),abs(j-frame['J']));assert abs(q-frame['q'])<2e-7 and abs(j-frame['J'])<2e-7
            wall=[f for f in faces if set(f[:3]).issubset(ids)];assert bool(wall)==frame['wall_child']
            normal=np.asarray(frame['wall0_normal'],dtype=LD);normal/=np.sqrt(normal@normal);projection=np.eye(3,dtype=LD)-np.outer(normal,normal)
            controls={}
            for f in (1,2,4,8,16):
                trial=m+(f-1)*(projection@m@projection);assert abs((normal@trial@normal)/(normal@m@normal)-1)<1e-12
                controls[str(f)]=dict(zip(('q','J'),previous.quality(xyz,trial)))
            item=dict(cell=frame['cell'],wall_child=bool(wall),baseline_q=q,baseline_J=j,tangent_controls=controls)
            if wall:
                assert len(wall)==1;base=np.asarray([xyz[ids.index(i)] for i in wall[0][:3]],dtype=LD)
                bound,height,plane_normal=placement_control(base,m,2e-6,.1)
                apex=next(xyz[k] for k,i in enumerate(ids) if i not in wall[0][:3]);actual_height=abs(np.dot(apex-base[0],plane_normal))
                item.update(frozen_centroid_plane_height_control_q_max=bound,control_height=height,physical_plane_height=float(actual_height),normal_alignment_abs=float(abs(normal@plane_normal)))
                if label=='saved_prepared_repair_before':assert abs(actual_height/LD('2e-6')-1)<=LD('.1') and q<=bound+1e-10
                rotation=previous.rotate(normal,plane_normal);aligned=rotation@m@rotation.T
                item['face_normal_aligned_control']=dict(zip(('q','J'),previous.quality(xyz,aligned)))
            if frame['cell'] in shared:
                old=shared[frame['cell']];assert old['metric']==frame['metric'] and old['centroid']==frame['centroid'] and old['q']==frame['q'] and old['J']==frame['J']
            else:shared[frame['cell']]=frame
            results.append(item)
        summary={}
        for f in ('1','2','4','8','16'):
            summary[f]={group:dict(q_bad=sum(x['tangent_controls'][f]['q']<.2 for x in results if x['wall_child']==wall),J_bad=sum(x['tangent_controls'][f]['J']<.05 for x in results if x['wall_child']==wall)) for group,wall in [('wall',True),('other',False)]}
        reports.append(dict(context=label,cells=len(cells),baseline_maximum_absolute_error=error,wall_cells_with_frozen_plane_height_qmax_below_gate=sum(x.get('frozen_centroid_plane_height_control_q_max',1)<.2 for x in results if x['wall_child']),tangent_control_counts=summary,results=results))
    result=dict(scope=__doc__,case=str(case),reports=reports,input_sha256=pins,helper_sha256=sha(__file__),limits=['Frozen-centroid constant-tensor plane-height optimum ignores position-dependent target, curved distance/footprint, neighbor validity and body/global clearance; NOT a full-field impossibility proof.','Orientation and tangent controls change matrices at fixed points; they are not production targets or admissible movement/mesh candidates.','Saved coordinates are old358; evolving current nodal metric transport is not reconstructed.','No new geometry queries, target selection, construction policy, normal law or acceptance gate change.'])
    output=R/'saved_row399_sensitivity.json';assert not output.exists();output.write_text(json.dumps(result,indent=2,allow_nan=False)+'\n')
    print(json.dumps([{k:x[k] for k in ('context','cells','baseline_maximum_absolute_error','wall_cells_with_frozen_plane_height_qmax_below_gate','tangent_control_counts')} for x in reports]))
if __name__=='__main__':main()
