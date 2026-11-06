#!/usr/bin/env python3
"""Fit a QCR/EPM nodal diagnostic on the same simplex nodes; never claims exact face replay."""
import argparse
import json
from pathlib import Path
import subprocess
import tempfile
import numpy as np
from campaign import digest, read_csv, save
from field_io import write_field


def fit(csv_path,mesh_path,definition_path,probe,output,velocity_ref,length_ref,kind,donor):
    if Path(output).exists():raise ValueError("refusing to overwrite an existing target artifact")
    if not np.isfinite([velocity_ref,length_ref]).all() or min(velocity_ref,length_ref)<=0:
        raise ValueError("positive finite internal reference scales required")
    definition=json.loads(Path(definition_path).read_text());definition["purpose"]="COUPLING_CONTROL"
    with np.load(mesh_path,allow_pickle=False) as mesh:nodes,cells=mesh["nodes"],mesh["cells"]
    dim=nodes.shape[1];data=read_csv(csv_path);order=np.argsort(data["PointID"])
    if not np.array_equal(data["PointID"][order],np.arange(len(nodes))):raise ValueError("target needs each field node id exactly once")
    xyz=np.column_stack([data[key][order] for key in ("x","y","z")[:dim]])*length_ref
    if not np.allclose(xyz,nodes,rtol=1e-12,atol=1e-12):raise ValueError("target and field nodes differ; resampling must be explicit")
    target=np.column_stack([data["Stress_E_"+key][order] for key in ("XX","YY","ZZ","XY","XZ","YZ")])*velocity_ref**2
    count=3 if dim==2 else 5
    points=nodes if dim==3 else np.column_stack((nodes,np.zeros(len(nodes))))
    text="\n".join(" ".join(format(v,".17g") for v in row) for row in points)+"\n"
    operator=np.zeros((len(nodes),6,count))
    with tempfile.TemporaryDirectory() as temp:
        for j in range(count):
            raw=np.zeros((len(nodes),5));raw[:,j]=1
            path=Path(temp)/"basis.suf";write_field(path,nodes,cells,raw,definition)
            result=subprocess.run([str(Path(probe).resolve()),str(path)],input=text,text=True,capture_output=True,check=True)
            operator[:,:,j]=np.array([list(map(float,row.split())) for row in result.stdout.splitlines()])
    raw=np.zeros((len(nodes),5));error=np.zeros(len(nodes))
    for i in range(len(nodes)):
        raw[i,:count]=np.linalg.lstsq(operator[i],target[i],rcond=1e-12)[0]
        error[i]=np.linalg.norm(operator[i]@raw[i,:count]-target[i])
    scale=max(float(np.linalg.norm(target,axis=1).max()),1e-12)
    report={"kind":kind,"donor_id":donor,"purpose":"COUPLING_CONTROL","representation":"SAME_NODE_FIT",
            "source_csv_sha256":digest(csv_path),"mesh_sha256":digest(mesh_path),"definition_sha256":digest(definition_path),
            "internal_velocity_ref":velocity_ref,"internal_length_ref":length_ref,"max_nodal_error":float(error.max()),
            "relative_max_nodal_error":float(error.max()/scale),"worst_node":int(error.argmax()),
            "exact_face_replay":"NOT_PROVIDED_BY_NODAL_FIT","coupled_recovery":"NOT_RUN"}
    save(str(output)+".fit.json",report)
    if error.max()>1e-9*scale:raise ValueError("target is incompatible with declared wall/support map; see fit report")
    definition["donor_fit"]=report
    certificate=write_field(output,nodes,cells,raw,definition)
    report["raw_bounds_satisfied"]=max(certificate["norm_inf"],certificate["norm_H1"])<=definition["epsilon"]*(1+1e-10)
    report["field_sha256"]=certificate["file_sha256"];save(str(output)+".fit.json",report)
    return report


def main():
    p=argparse.ArgumentParser(description=__doc__)
    for name in ("csv","mesh","definition","probe","output","donor-id"):p.add_argument("--"+name,required=True)
    p.add_argument("--kind",choices=("QCR","EPM"),required=True)
    p.add_argument("--velocity-ref",type=float,required=True);p.add_argument("--length-ref",type=float,required=True)
    a=p.parse_args();print(json.dumps(fit(a.csv,a.mesh,a.definition,a.probe,a.output,a.velocity_ref,a.length_ref,a.kind,a.donor_id),indent=2))

if __name__=="__main__":main()
