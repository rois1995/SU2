"""Check saved RAE grids/solutions and plot actual mesh/Mach/Cp outputs."""
import argparse, csv, hashlib, json, re, sys
from pathlib import Path
import numpy as np

root=Path(__file__).resolve().parent.parent
sys.path.insert(0,str(root/'TestCases/adaptation/capability'))
import capcheck
from airfoil_reference_audit import reference_audit


def inspect(wd, cycles):
    cfg=(wd/'run.cfg').read_text()
    hausd=float(re.search(r'ADAP_HAUSD\s*=\s*([\deE.+-]+)',cfg)[1])
    requested=re.search(r'ADAP_BL_FIRST_HEIGHT\s*=\s*\(\s*([\deE.+-]+)',cfg)
    h0=float(requested[1]) if requested else None
    history=list(csv.DictReader((wd/'history.csv').open()))
    history=[{k.strip().strip('"'):v for k,v in row.items()} for row in history]
    records=[];plots=[];paths=[wd/'input.su2',wd/'run.cfg',wd/'history.csv',wd/'run_evidence.json']
    for cycle in cycles:
        meshpath=wd/('input.su2' if cycle==0 else f'mesh_adap_{cycle:05d}.su2')
        restart=wd/f'solution_adap_{cycle:05d}.dat';vtu=wd/f'flow_adap_{cycle:05d}.vtu'
        paths.extend((meshpath,restart,vtu))
        mesh=capcheck.read_su2(meshpath);points,fields,_=capcheck.read_restart(restart)
        vp,vfields,_=capcheck.read_vtu(vtu)
        gates=capcheck.check_validity(mesh)
        assert all(g[0]=='PASS' for g in gates.values()),gates
        assert np.array_equal(points,mesh.P)
        assert np.array_equal(vp[:,:2],points.astype('f4').astype(float)) and (vp[:,2]==0).all()
        assert all(np.isfinite(a).all() for a in fields.values())
        pairing={k:np.array_equal(v,fields[k].astype('f4').astype(float)) for k,v in vfields.items()}
        assert all(pairing.values()),pairing
        rho=fields['Density'];internal=fields['Energy']-.5*(fields['Momentum_x']**2+fields['Momentum_y']**2)/rho
        assert (rho>0).all() and (internal>0).all() and (fields['Pressure']>0).all()
        pressure_error=float(np.max(np.abs(.4*internal-fields['Pressure'])/np.maximum(1,np.abs(fields['Pressure']))))
        assert pressure_error<1e-9
        if 'Nu_Tilde' in fields:assert (fields['Nu_Tilde']>=0).all()
        reference=reference_audit(wd/'input.su2',meshpath)
        assert all(r['all_declared_features_exactly_retained'] and
                   abs(r['covered_arc_over_period']-1)<1e-10 and
                   r['max_corresponding_parameter_deviation']<=hausd for r in reference)
        height_error=None
        if h0 is not None and cycle>0:
            incidence={}
            for t in mesh.E:
                for k in range(3):incidence.setdefault(tuple(sorted((int(t[k]),int(t[(k+1)%3])))),[]).append(t)
            errors=[]
            for edge in mesh.M['AIRFOIL']:
                tri=incidence[tuple(sorted(map(int,edge)))];assert len(tri)==1
                a,b,c=mesh.P[tri[0]];area2=abs(np.cross(b-a,c-a))
                altitude=float(area2/np.linalg.norm(mesh.P[edge[1]]-mesh.P[edge[0]]))
                errors.append(abs(altitude/h0-1))
            height_error=max(errors);assert height_error<=1e-8
        last=[r for r in history if int(r['Adap_Cycle'])==cycle][-1]
        rec=dict(cycle=cycle,mesh=str(meshpath),restart=str(restart),paraview=str(vtu),points=len(mesh.P),
                 triangles=len(mesh.E),boundary_edges={k:len(v) for k,v in mesh.M.items()},validity=gates,
                 exact_restart_mesh_pairing=True,all_vtu_scalar_pairing=pairing,
                 minimum_density=float(rho.min()),minimum_internal_energy=float(internal.min()),
                 pressure_consistency_relative_error=pressure_error,reference=reference,
                 maximum_relative_first_height_error=height_error,last_density_residual=float(last['rms[Rho]']),
                 density_criterion_met=float(last['rms[Rho]'])<=-8,CL=float(last['CL']),CD=float(last['CD']))
        records.append(rec);plots.append((mesh,fields))
    sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
    return dict(case=str(wd),runtime=json.loads((wd/'run_evidence.json').read_text()),cycles=records,
                input_files_sha256={str(p):sha(p) for p in set(paths)},checker_sha256=sha(Path(__file__)),
                scope='Saved mesh/solution pairing, admissibility, topology, first height and original-reference checks. No independent metric-quality audit or aerodynamic grid-convergence claim.'),plots


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('case',type=Path);p.add_argument('--cycles',nargs='+',type=int,default=[0,1,2]);args=p.parse_args()
    wd=args.case.resolve();out=wd/'inspection.json'
    if out.exists():raise RuntimeError('Preserve existing inspection evidence; use a fresh case folder')
    record,plots=inspect(wd,args.cycles);out.write_text(json.dumps(record,indent=2)+'\n')
    import matplotlib
    matplotlib.use('Agg');import matplotlib.pyplot as plt
    fig,axes=plt.subplots(2,len(plots),figsize=(6*len(plots),6),squeeze=False,constrained_layout=True)
    for j,((mesh,fields),row) in enumerate(zip(plots,record['cycles'])):
        axes[0,j].triplot(mesh.P[:,0],mesh.P[:,1],mesh.E,lw=.2,color='#345075')
        im=axes[1,j].tripcolor(mesh.P[:,0],mesh.P[:,1],mesh.E,fields['Mach'],shading='gouraud',cmap='turbo',vmin=0,vmax=1.5)
        for ax in axes[:,j]:
            for edge in mesh.M['AIRFOIL']:ax.plot(mesh.P[edge,0],mesh.P[edge,1],'k-',lw=.6)
            ax.set(xlim=(-.05,1.15),ylim=(-.14,.16),xlabel='x/c',ylabel='y/c');ax.set_aspect('equal')
        axes[0,j].set_title(f"Cycle {row['cycle']}: {row['triangles']} triangles")
    fig.colorbar(im,ax=list(axes[1,:]),label='Mach');fig.savefig(wd/'mesh_and_mach.png',dpi=180);plt.close(fig)
    fig,ax=plt.subplots(figsize=(9,5),constrained_layout=True)
    for (mesh,fields),row in zip(plots,record['cycles']):
        ids=np.unique(mesh.M['AIRFOIL']);ax.scatter(mesh.P[ids,0],fields['Pressure_Coefficient'][ids],s=7,label=f"cycle {row['cycle']}")
    ax.invert_yaxis();ax.set(xlabel='x/c',ylabel='Cp');ax.legend();fig.savefig(wd/'surface_cp.png',dpi=180);plt.close(fig)
    for r in record['cycles']:print(r['cycle'],r['points'],r['triangles'],'rho residual',r['last_density_residual'],'CL/CD',r['CL'],r['CD'])

if __name__=='__main__':main()
