#!/usr/bin/env python3
"""Summarize existing lifecycle clocks without rerunning CFD or inferring a CFD-only cost."""
import hashlib,json,sys
from pathlib import Path
HERE=Path(__file__).resolve().parent

def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def fields(line):
    result={}
    for token in line.split()[1:]:
        key,value=token.split('=',1)
        try:result[key]=float(value)
        except ValueError:result[key]=value
    return result

def closes(row,keys):
    residual=row['total']-sum(row[k] for k in keys)
    assert abs(residual)<=1e-7*max(1,abs(row['total'])),('Clock closure',row.get('stage'),residual)
    return residual

def main():
    case=HERE/sys.argv[1];receipt_path=case.with_suffix('.json');receipt=json.loads(receipt_path.read_text())
    assert receipt['status']=='PASS' and all(sha(case/p)==h for p,h in receipt['artifacts'].items())
    lines=(case/'native.log').read_text().splitlines();native=[fields(l) for l in lines if l.startswith('NATIVE3D rank=')]
    detail=[fields(l) for l in lines if l.startswith('NATIVE3D_DETAIL ')];stages=[fields(l) for l in lines if l.startswith('NATIVE3D_STAGE ')]
    ranks=receipt['contract']['MPI_ranks'];workers=receipt['contract']['environment']['SU2_NATIVE_POLICY_WORKERS']
    assert len(native)==2*ranks and all(r['complete']==1 for r in native)
    native_keys=['support','reference','import','source','weights','partition','donor_migration','communicator','refine','coarsen','wait','return','policy','other']
    refine_keys=['initialization','seed','first_layer','selection','dependency','reservation','donor','reconstruction','metric','routing','publication','update','completion','other']
    for row in native:row['exclusive_clock_closure']=closes(row,native_keys)
    for row in detail:
        if row['stage'] in ('refine','coarsen'):row['exclusive_clock_closure']=closes(row,refine_keys)
    for rank in range(ranks):assert sum(row['rank']==rank for row in native)==2
    epochs=[json.loads((case/('curved_rans_m'+workers+'_s'+str(i)+'_summary.json')).read_text()) for i in (0,1)]
    report={'status':'REPORTED_SECTION_CLOCKS_DIAGNOSTIC_ONLY','native_receipt_sha256':sha(receipt_path),'log_sha256':sha(case/'native.log'),
            'helper_sha256':sha(Path(__file__).resolve()),'MPI_ranks':ranks,'adaptation_workers':int(workers),
            'fixture_epoch_clocks':epochs,'native_exclusive_phases_by_rank':native,'nested_details_by_rank':detail,'driver_metric_rebuild_clocks':stages,
            'scope':'Existing accepted correctness-run clocks. Native exclusive phase sums are checked, including partition/migration/communicator/return; nested fields must not be added again. Parallel rank times must not be summed as elapsed time.',
            'missing_or_unproven':['solve_seconds includes automatic metric calls; CFD-only time and adaptation/CFD ratio unavailable.',
                'Regional reconstruction still combines surface/band/core/embedding validation; its dominant internal operation is not isolated.',
                'Metric complexity/gradation/audit shares a combined clock; costs are not yet independently separated.',
                'These small local cases and machine conditions do not establish scaling, wall-time speedup, viscous accuracy or unsteady affordability.']}
    (case/'reported_sections.json').write_text(json.dumps(report,indent=2)+'\n');print(report['status'],flush=True)
if __name__=='__main__':main()
