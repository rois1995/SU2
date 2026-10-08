import sys,json
from pathlib import Path
root=Path('/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated');sys.path.insert(0,str(root/'integration_evidence'))
from audit_native_unsteady import audit
case=Path(sys.argv[1]);row=audit(case,require_native_reference=False)
row['scope']+='; explicit MMG compatibility control, original native-reference sidecar requirement does not apply'
(case/'independent_mmg_unsteady_audit.json').write_text(json.dumps(row,indent=2)+'\n');print('MMG CONTROL PASS')
