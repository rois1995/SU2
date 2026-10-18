#!/usr/bin/env python3
"""Check the rejected private working skin separately from tentative collapse geometry."""
import collections,hashlib,importlib.util,json
from fractions import Fraction as F
from pathlib import Path
HERE=Path(__file__).resolve().parent;BASE=HERE.with_name('planar_symmetry_reference_v1')
def main():
    source=BASE/'annulus_v8_n1/symmetry_annulus_bl_budget_20_m1_rejected_rank0.json'
    d=json.loads(source.read_text());points={n[0]:tuple(F(float(x)) for x in n[1:]) for n in d['nodes']}
    spec=importlib.util.spec_from_file_location('annular_audit',BASE/'audit_annulus.py');a=importlib.util.module_from_spec(spec);spec.loader.exec_module(a)
    facets={f['id']:f for f in d['facets']};membership={};references={};areas=collections.defaultdict(F)
    for group in d['facet_groups']:
        first=facets[group[0]];triangle=[tuple(F(float(x)) for x in n[1:]) for n in first['vertices']]
        normal=a.base.cross(a.base.sub(triangle[1],triangle[0]),a.base.sub(triangle[2],triangle[0]));axis=next(i for i in range(3) if normal[i])
        refs=[];signed=F(0)
        for id in group:
            membership[id]=min(group);t=[tuple(F(float(x)) for x in n[1:]) for n in facets[id]['vertices']]
            refs.append([tuple(p[k] for k in range(3) if k!=axis) for p in t]);signed+=a.twice_area(refs[-1])
        references[min(group)]=(triangle[0],normal,axis,refs,signed)
    for face in d['surface']:
        root=membership[face['facet']];origin,normal,axis,refs,signed=references[root];t=[points[n] for n in face['nodes']]
        assert all(a.base.dot(normal,a.base.sub(p,origin))==0 for p in t)
        flat=[tuple(p[k] for k in range(3) if k!=axis) for p in t];area=a.twice_area(flat)
        assert area*signed>0 and sum(a.intersection_area(flat,r) for r in refs)==abs(area)
        areas[root]+=area
    assert all(areas[root]==row[-1] for root,row in references.items())
    report=dict(status='REJECTED_PRIVATE_WORKING_SKIN_AUTHORITY_PASS',accepted_event=False,cells=len(d['cells']),faces=len(d['surface']),
        finite_groups=len(references),exact_planarity_orientation_finite_coverage_and_areas=True,
        limits='Physical working skin only; no volume/metric/transfer acceptance. Tentative invalid collapse geometry is a separate local refusal.',
        sha256={str(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in (source,BASE/'audit_annulus.py',Path(__file__))})
    (HERE/'rejected_skin.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))
if __name__=='__main__':main()
