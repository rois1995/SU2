#!/usr/bin/env python3
"""Replay the global statistical BL subset through the uncertainty controller, with unknown upper mass."""
import csv,json,sys
from pathlib import Path
from run import HERE,ROOT,OLD,sha,invoke


def run(label,ranks):
    assert ranks in (1,2,4)
    source=HERE/label;prep=json.loads((source/'preparation.json').read_text());packet=source/'queries.bin'
    assert sha(packet)==prep['input_sha256']
    for name,digest in prep['sha256'].items():assert sha(ROOT/name)==digest,name
    dest=HERE/'one_sided_controller_v1'/('%s_np%d'%(label,ranks))
    invoke(dest,{'SU2_NATIVE_BL_WALL_PRISMS':str(packet),'SU2_NATIVE_BL_PRISM_CAP':str(prep['cap']),
        'SU2_NATIVE_BL_PRISM_DELTA':str(prep['delta']),'SU2_NATIVE_BL_PRISM_TARGET':str(prep['requested_complexity'])},OLD/'cell329/replay.dat',ranks)
    result=json.loads((dest/'prism_global_controller.json').read_text());reference=json.loads((source/'np1/statistics.json').read_text())
    assert result['samples']==reference['samples'] and result['status']=='outside_bracket'
    assert result['minimum']>result['target'] and result['upper_mass_unknown'] and not result['full_mass_integrated']
    assert result['new_queries_during_scale_search']==0 and result['scale_trials']<=24
    assert abs(result['mean_lower_contribution']/reference['clipped_lower_mean']-1)<2e-15
    assert abs(result['minimum']/reference['statistical_lower']-1)<2e-15
    rows=[]
    for rank in range(ranks):rows.extend(csv.DictReader((dest/('wall_prism_rank%d.csv'%rank)).open()))
    rows.sort(key=lambda row:int(row['row']));old=list(csv.DictReader((source/'np1/wall_prism_rank0.csv').open()))
    assert rows==old,'Every actual density/containment query must match the preserved original seed.'
    proof=dict(status='PASS',global_lower_reduction_matches_saved_seed=True,all_query_rows_identical_to_saved_seed=True,
        result_sha256=sha(dest/'prism_global_controller.json'),source_preparation_sha256=sha(source/'preparation.json'),
        source_statistics_sha256=sha(source/'np1/statistics.json'),source_rows_sha256=sha(source/'np1/wall_prism_rank0.csv'),
        current_rows_sha256={p.name:sha(p) for p in dest.glob('wall_prism_rank*.csv')},
        assumptions=prep['statistical_assumptions'],production_normalization_changed=False,CFD_steps=0)
    (dest/'verification.json').write_text(json.dumps(proof,indent=2)+'\n');print(json.dumps(result),flush=True)


if __name__=='__main__':run(sys.argv[1],int(sys.argv[2]))
