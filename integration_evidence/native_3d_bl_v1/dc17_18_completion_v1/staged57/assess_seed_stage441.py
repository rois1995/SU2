"""Verify the new stage0 bundle by reusing exact saved-CFD qualification; never alter the original seed."""
from pathlib import Path
import json,hashlib,sys
R=Path(__file__).resolve().parent
case=Path(sys.argv[1]).resolve();original=R/'m6_bl_inputs/rans_metric_entry364_v1'
sha=lambda p:hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()
status=json.loads((case/'status.json').read_text());assert status['state']=='terminal' and status['exit_code']==0 and status['pins_verified']
assert json.loads((case/'terminal_handles_actual.json').read_text())['wrapper_verified_terminal']
prior=json.loads((original/'provenance.json').read_text());report=json.loads((case/'height_seed_stage.json').read_text())
assert report['N']==5 and report['event']==0 and report['factor']==32
assert report['final_h0']==2e-6 and report['effective_h0']==6.4e-5 and report['growth']==1.2 and report['thickness']==.01
assert report['mesh_unchanged'] and not report['CFD_advanced'] and not report['BL_generated']
files={}
for name in ('m6.su2','m6.su2.native_ref','baseline_sa_seed.dat'):
 assert sha(case/name)==prior['files'][name]['sha256'];files[name]={'sha256':sha(case/name)}
expected=''
for line in (original/'run.cfg').read_text().splitlines():
 assert 'ADAP_BL_H0_RAMP=' not in line
 if 'ADAP_SUBITER=' in line:line='ADAP_SUBITER= ( 7 )'
 expected+=line+'\n'
expected+='ADAP_BL_H0_RAMP= 5\n';assert (case/'run.cfg').read_text()==expected
for name in ('run.cfg','m6.su2.native_rows'):files[name]={'sha256':sha(case/name)}
rows=(case/'m6.su2.native_rows').read_text();assert rows.startswith('SU2_NATIVE_ROWS_CONTINUATION 1\nmesh ')
stage=next(line for line in rows.splitlines() if line.startswith('stage ')).split();assert list(map(int,stage[1:4]))==[5,0,7]
qualification=json.loads((original/'qualification.json').read_text());qualification.update(stage='explicit proposed first-height continuation stage0',N=5,event=0,effective_h0=6.4e-5,final_h0=2e-6,CFD_advanced=False,BL_generated=False,target_normalized=False,source_qualification={'path':str(original/'qualification.json'),'sha256':sha(original/'qualification.json')})
(case/'qualification.json').write_text(json.dumps(qualification,indent=2)+'\n')
provenance={'stage':'verified saved CFD + original no-CAD authority + explicit proposed stage0 metadata','scope':'Only existing ADAP_BL_H0_RAMP=5 and ADAP_SUBITER=(7) change; growth/thickness/final h0/sensor/norm/budgets unchanged. Five intermediate events plus two final-height events are not proof of construction.','files':files,'original_seed_provenance':{'path':str(original/'provenance.json'),'sha256':sha(original/'provenance.json')},'source_seed_stage_report_sha256':sha(case/'height_seed_stage.json'),'assessment_helper':{'path':str(Path(__file__).resolve()),'sha256':sha(__file__)},'target_normalized':False,'mesh_accepted':False,'CFD_advanced':False,'original303_seed_preserved':True}
(case/'provenance.json').write_text(json.dumps(provenance,indent=2)+'\n');(R/'ramp5_seed_stage441.json').write_text(json.dumps({'case':str(case),'provenance_sha256':sha(case/'provenance.json'),'qualification_sha256':sha(case/'qualification.json'),'files':files},indent=2)+'\n');print({'seed_stage':str(case),'inputs_unchanged':True,'effective_h0':6.4e-5,'final_h0':2e-6,'target_normalized':False})
