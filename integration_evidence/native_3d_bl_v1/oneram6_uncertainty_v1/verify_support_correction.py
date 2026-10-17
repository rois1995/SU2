#!/usr/bin/env python3
"""Independent original-P1 density audit and MPI comparison; not a quadrature bound."""
import csv,json,struct,sys
from fractions import Fraction
from pathlib import Path
import numpy as np
from run import HERE,ROOT,sha
from support_correction import DOMAIN


def determinant(m):
    return sum(m[0][k]*(m[1][(k+1)%3]*m[2][(k+2)%3]-m[1][(k+2)%3]*m[2][(k+1)%3]) for k in range(3))


def exact_weights(vertices,point):
    x=[[Fraction(float(a)) for a in row] for row in vertices];p=[Fraction(float(a)) for a in point]
    matrix=[[x[k+1][j]-x[0][j] for k in range(3)] for j in range(3)];denominator=determinant(matrix)
    result=[]
    for k in range(3):
        replaced=[row.copy() for row in matrix]
        for j in range(3):replaced[j][k]=p[j]-x[0][j]
        result.append(determinant(replaced)/denominator)
    result=[1-sum(result)]+result
    assert all(w>=0 for w in result),'Claimed original donor does not contain the exact binary64 point.'
    return [float(w) for w in result]


def density(matrix):
    matrix=np.tril(matrix)+np.swapaxes(np.tril(matrix,-1),-1,-2)
    sign,log=np.linalg.slogdet(matrix);assert np.all(sign>0)
    return np.exp(.5*log)


def audit(label,ranks=4):
    folder=HERE/label;prep=json.loads((folder/'preparation.json').read_text());source=ROOT/prep['domain']
    assert sha(source)==prep['domain_sha256'] and sha(folder/'proposal_samples.npz')==prep['proposal_sha256']
    with source.open('rb') as f:magic,count,core=struct.unpack('<QQd',f.read(24))
    assert magic==0x4e41545352433344 and source.stat().st_size==24+count*DOMAIN.itemsize
    domain=np.memmap(source,dtype=DOMAIN,offset=24,mode='r',shape=(count,));rows=[];dest=folder/('np%d'%ranks)
    report=json.loads((dest/'report.json').read_text())
    for name,digest in report['rows_sha256'].items():
        assert sha(dest/name)==digest;rows.extend(csv.DictReader((dest/name).open()))
    rows.sort(key=lambda row:int(row['row']));assert [int(row['row']) for row in rows]==list(range(prep['samples']))
    present=np.array([i for i,row in enumerate(rows) if int(row['donor'])!=2**64-1]);assert len(present)>0
    ids=np.array([int(rows[i]['donor']) for i in present]);offset=np.zeros(len(ids),dtype=int) if count==1 else ids
    records=domain[offset];assert np.array_equal(records['id'],ids)
    points=np.load(folder/'proposal_samples.npz',allow_pickle=False)['points'][present]
    x=records['coordinates'];edge=x[:,1:]-x[:,0,None];matrix=np.swapaxes(edge,1,2)
    partial=np.linalg.solve(matrix,(points-x[:,0])[...,None])[...,0];weights=np.column_stack([1-partial.sum(axis=1),partial])
    native=np.array([float(rows[i]['sensor']) for i in present]);estimated=density(np.einsum('nk,nkab->nab',weights,records['sensor']))
    condition=np.linalg.cond(matrix);selection=np.unique(np.r_[np.arange(min(32,len(ids))),np.argsort(condition)[-32:]])
    exact=np.array([exact_weights(x[i],points[i]) for i in selection])
    precise=density(np.einsum('nk,nkab->nab',exact,records['sensor'][selection]))
    error=float(np.max(abs(estimated/native-1)));exact_error=float(np.max(abs(precise/native[selection]-1)))
    assert max(error,exact_error)<1e-9,'Independent original-field density differs materially.'
    result=dict(status='PASS_ORIGINAL_SENSOR_DENSITY_AUDIT',inside_points=len(ids),all_point_maximum_relative_density_difference=error,
        exact_binary64_containment_and_weights_samples=len(selection),exact_weight_maximum_relative_density_difference=exact_error,
        maximum_cell_condition=float(condition.max()),proven_integration_error_bound=False,
        scope='Independent NumPy determinant/every inside source; exact rational containment and barycentrics on first32 and worst32 conditions. Outside-domain coverage and composed density are not independently checked here.',
        domain_sha256=sha(source),preparation_sha256=sha(folder/'preparation.json'),consumer_report_sha256=sha(dest/'report.json'),helper_sha256=sha(Path(__file__)))
    other=folder/'np2/report.json'
    if ranks==4 and other.exists():
        old=[];previous=json.loads(other.read_text())
        for name,digest in previous['rows_sha256'].items():
            assert sha(other.parent/name)==digest;old.extend(csv.DictReader((other.parent/name).open()))
        old.sort(key=lambda row:int(row['row']));assert old==rows
        result.update(all_MPI2_4_rows_identical=True,MPI2_report_sha256=sha(other))
    (dest/'independent_source_audit.json').write_text(json.dumps(result,indent=2)+'\n');print(label,json.dumps(result),flush=True)


if __name__=='__main__':
    for label in sys.argv[1:]:audit(label)
