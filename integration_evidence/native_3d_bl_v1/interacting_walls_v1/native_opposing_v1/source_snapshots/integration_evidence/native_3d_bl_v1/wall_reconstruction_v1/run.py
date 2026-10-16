#!/usr/bin/env python3
"""Reuse the production provenance collector for bounded serial wall-reconstruction stages."""
import hashlib,json,os,re,subprocess,sys,time
from pathlib import Path
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2];BUILD=ROOT/'integration_evidence/build-integrated-v2';BINARY=BUILD/'UnitTests/test_driver'
def digest(p):
 h=hashlib.sha256()
 with p.open('rb') as f:
  for block in iter(lambda:f.read(1024*1024),b''):h.update(block)
 return h.hexdigest()
def main():
 case=sys.argv[1]
 runtime=re.fullmatch(r'native_curved_rans_n([124])_m([124])_v[0-9]+',case)
 layered=re.fullmatch(r'native_front_(constant|varying|rotated)(?:_mpi([24]))?_v[0-9]+',case)
 core=layered or re.fullmatch(r'native_core_(constant|varying|rotated)(?:_mpi([24]))?_v[0-9]+',case)
 opposing=re.fullmatch(r'native_opposing(?:_mpi([24]))?_v[0-9]+',case)
 edge_controls=re.fullmatch(r'native_global_edge_controls_v[0-9]+',case)
 if runtime:assert int(runtime[2])<=int(runtime[1])
 recipes={'native_core_constant_v1':'geometry','native_core_varying_v1':'geometry','native_core_rotated_v1':'geometry','native_complete_domain_controls_v1':'geometry','native_curved_volume_controls_v7':'geometry','native_curved_volume_controls_v6':'geometry','native_curved_volume_controls_v5':'geometry','native_curved_volume_controls_v4':'geometry','native_curved_volume_controls_v3':'geometry','native_curved_volume_controls_v2':'geometry','native_curved_volume_controls_v1':'geometry','native_curved_rans_v1':'geometry','native_curved_rans_mpi2_v1':'geometry','native_curved_rans_mpi4_v1':'geometry','native_region_field_controls_mpi2_v2':'geometry','native_region_field_controls_mpi4_v2':'geometry','native_region_field_controls_v2':'geometry','native_refreshed_source_v2':'curved_cold','native_refreshed_source_mpi2_v2':'curved_cold','native_refreshed_source_mpi4_v2':'curved_cold','native_field_compat_v2':'curved_feature_complete','native_region_field_controls_v1':'geometry','native_refreshed_source_v1':'curved_cold','native_refreshed_source_mpi2_v1':'curved_cold','native_refreshed_source_mpi4_v1':'curved_cold','native_field_compat_v1':'curved_feature_complete','native_cold_v3':'curved_cold','native_cold_mpi2_v3':'curved_cold','native_cold_mpi4_v3':'curved_cold','native_complete_v3':'curved_feature_complete','native_complete_field_v3':'curved_grid_audit','native_cold_bl_field_v3':'curved_grid_audit','native_cold_v2':'curved_cold','native_complete_v2':'curved_feature_complete','native_primitive_controls_v1':'geometry','native_cold_field_v1':'curved_grid_audit','native_cold_v1':'curved_cold','native_complete_field_v1':'curved_grid_audit','native_complete_mpi2':'curved_joint_protocol_feature_mpi2','native_complete_mpi4':'curved_joint_protocol_mpi4','native_complete_v1':'curved_feature_complete','metric_derivative_controls':'geometry','curved_metric_patch_probe':'curved_current_wall_refit_probe','curved_metric_complete':'curved_feature_complete','curved_metric_residual':'curved_feature_residual','curved_joint_protocol_metric_mpi2':'curved_joint_protocol_feature_mpi2'}
 recipes.update({'native_complete_v11':'curved_joint_protocol','native_complete_mpi2_v11':'curved_joint_protocol_mpi2','native_complete_mpi4_v11':'curved_joint_protocol_mpi4','native_curved_volume_controls_v11':'geometry'})
 field_sources={'native_complete_field_v1':('native_complete_v1','curved_grid_adapted.su2'),'native_complete_field_v3':('native_complete_v3','curved_grid_adapted.su2'),'native_cold_bl_field_v3':('native_cold_v3','curved_cylinder_BL.su2')}
 stage='geometry' if runtime or core or edge_controls or opposing else recipes.get(case,case);assert stage in ("geometry","planar_wall_replay","sensor","sensor_mpi2","sensor_mpi4","curved_field","curved_volume","curved_cold","curved_replay","curved_replay_remaining","curved_replay_complete","curved_grid_audit","curved_cold_residual","curved_grid_refine","curved_transition_complete","curved_joint_engine","curved_edge_replay","curved_joint_search","curved_interior_search","curved_interior_engine","curved_interior_residual","curved_interior_continue","curved_reconnection_probe","curved_cover_probe","curved_cap_cover_probe","curved_cap_point_probe","curved_costed_complete","curved_wall_refit_probe","curved_rim_refit_probe","curved_current_wall_refit_probe","curved_wall_refit_complete","curved_joint_connectivity_complete","curved_feature_complete","curved_wall_refit_residual","curved_joint_connectivity_residual","curved_feature_residual","curved_refit_interior_point","curved_refit_interior_cover","curved_refit_interior_dependencies","curved_joint_protocol","curved_joint_protocol_mpi2","curved_joint_protocol_cap_mpi2","curved_joint_protocol_refit_mpi2","curved_joint_protocol_connectivity_mpi2","curved_joint_protocol_feature_mpi2","curved_joint_protocol_mpi4","interior_geometry","topology_hint_controls","curved_repair_controls","curved_feature_controls","interacting_volume")
 ranks=2 if stage in ("sensor_mpi2","curved_joint_protocol_mpi2","curved_joint_protocol_cap_mpi2","curved_joint_protocol_refit_mpi2","curved_joint_protocol_connectivity_mpi2","curved_joint_protocol_feature_mpi2") else 4 if stage in ("sensor_mpi4","curved_joint_protocol_mpi4") else 1;selection="[NativeWallReconstruction3D],[NativeJointGeometry3D]" if stage=="geometry" else "[NativeFinerSensorBL3D]"
 if case in ('native_cold_mpi2_v3','native_refreshed_source_mpi2_v1','native_refreshed_source_mpi2_v2','native_region_field_controls_mpi2_v2'):ranks=2
 if case in ('native_cold_mpi4_v3','native_refreshed_source_mpi4_v1','native_refreshed_source_mpi4_v2','native_region_field_controls_mpi4_v2'):ranks=4
 expected=4 if stage=="geometry" else 1
 if stage=="planar_wall_replay":selection="[NativePlanarWallReplay3D]"
 if stage=="topology_hint_controls":selection="[NativeClosureHint3D]"
 if stage=="interior_geometry":selection="[NativeInteriorPlacement3D]"
 if stage.startswith("curved_") or stage=="interacting_volume":
  selection={"curved_field":"[NativeCurvedBL3D]","curved_volume":"[NativeCurvedBLVolume3D]","curved_cold":"[NativeCurvedBLConstruction3D]","curved_replay":"[NativeCurvedBLConstruction3D]","curved_replay_remaining":"[NativeCurvedBLConstruction3D]","curved_replay_complete":"[NativeCurvedBLConstruction3D]","curved_grid_audit":"[NativeCurvedBLConstruction3D]","curved_cold_residual":"[NativeCurvedBLConstruction3D]","curved_grid_refine":"[NativeCurvedBLConstruction3D]","curved_transition_complete":"[NativeCurvedBLConstruction3D]","curved_joint_engine":"[NativeCurvedBLConstruction3D]","curved_edge_replay":"[NativeCurvedBLConstruction3D]","curved_joint_search":"[NativeCurvedBLConstruction3D]","curved_interior_search":"[NativeCurvedBLConstruction3D]","curved_interior_engine":"[NativeCurvedBLConstruction3D]","curved_interior_residual":"[NativeCurvedBLConstruction3D]","curved_interior_continue":"[NativeCurvedBLConstruction3D]","curved_reconnection_probe":"[NativeCurvedBLConstruction3D]","curved_cover_probe":"[NativeCurvedBLConstruction3D]","curved_cap_cover_probe":"[NativeCurvedBLConstruction3D]","curved_cap_point_probe":"[NativeCurvedBLConstruction3D]","curved_costed_complete":"[NativeCurvedBLConstruction3D]","curved_wall_refit_probe":"[NativeCurvedBLConstruction3D]","curved_rim_refit_probe":"[NativeCurvedBLConstruction3D]","curved_current_wall_refit_probe":"[NativeCurvedBLConstruction3D]","curved_wall_refit_complete":"[NativeCurvedBLConstruction3D]","curved_joint_connectivity_complete":"[NativeCurvedBLConstruction3D]","curved_feature_complete":"[NativeCurvedBLConstruction3D]","curved_wall_refit_residual":"[NativeCurvedBLConstruction3D]","curved_joint_connectivity_residual":"[NativeCurvedBLConstruction3D]","curved_feature_residual":"[NativeCurvedBLConstruction3D]","curved_refit_interior_point":"[NativeCurvedBLConstruction3D]","curved_refit_interior_cover":"[NativeCurvedBLConstruction3D]","curved_refit_interior_dependencies":"[NativeCurvedBLConstruction3D]","curved_joint_protocol":"[NativeCurvedBLConstruction3D]","curved_joint_protocol_mpi2":"[NativeCurvedBLConstruction3D]","curved_joint_protocol_cap_mpi2":"[NativeCurvedBLConstruction3D]","curved_joint_protocol_refit_mpi2":"[NativeCurvedBLConstruction3D]","curved_joint_protocol_connectivity_mpi2":"[NativeCurvedBLConstruction3D]","curved_joint_protocol_feature_mpi2":"[NativeCurvedBLConstruction3D]","curved_joint_protocol_mpi4":"[NativeCurvedBLConstruction3D]","curved_repair_controls":"[NativeConnectivity3D],[NativeConnectivityMove3D],[NativeConnectivityHeight3D],[NativeConnectivityInterface3D]","curved_feature_controls":"[NativeCurvedGeometry3D],[NativeCurvedChart3D],[NativeCurvedLayered3D],[NativePlanarStrip3D]","interacting_volume":"[NativeInteractingBLVolume3D]"}[stage]
  expected=11 if stage=="curved_feature_controls" else 5 if stage=="curved_repair_controls" else 4 if stage=="curved_field" else 1
 if case.startswith('native_curved_rans'):selection='[NativeCurvedRANS3D]';expected=1;ranks=4 if 'mpi4' in case else 2 if 'mpi2' in case else 1
 if runtime:ranks=int(runtime[1])
 if case in ('native_curved_volume_controls_v1','native_curved_volume_controls_v2','native_curved_volume_controls_v3','native_curved_volume_controls_v4','native_curved_volume_controls_v5','native_curved_volume_controls_v6','native_curved_volume_controls_v7'):selection='[NativeCylinderBLVolume3D],[NativeCurvedBLVolume3D]';expected=2
 if case in ('native_curved_volume_controls_v6','native_curved_volume_controls_v7'):selection+=',[NativeParallelBL3D]';expected=3
 if case=='native_curved_volume_controls_v11':selection='[NativeCylinderBLVolume3D],[NativeCurvedBLVolume3D],[NativeParallelBL3D]';expected=3
 if core:selection='[NativeRegionCore3D]';expected=1;ranks=int(core[2] or 1)
 if opposing:selection='[NativeOpposingBL3D]';expected=1;ranks=int(opposing[1] or 1)
 if edge_controls:selection='[NativeComposite3D],[NativeBalancedSplit3D],[NativeLayeredField3D],[NativeSmoothClosest3D]';expected=12
 if case=='native_complete_domain_controls_v1':selection='[NativePolicyRuntime3D]';expected=1
 if case=='metric_derivative_controls':selection='[NativeMetricJacobian3D]';expected=1
 if case=='native_primitive_controls_v1':selection='[NativePrimitiveRuntime3D]';expected=1
 if case.startswith('native_region_field_controls'):selection='[NativeRegionField3D]';expected=1
 check=subprocess.run(['/tmp/native-build-tools/ninja','-C',str(BUILD),'-n','UnitTests/test_driver','SU2_CFD/src/SU2_CFD'],stdout=subprocess.PIPE,stderr=subprocess.STDOUT,universal_newlines=True,timeout=30)
 assert check.returncode==0 and 'ninja: no work to do.' in check.stdout,check.stdout
 sources=['UnitTests/SU2_CFD/adaptation/BoundaryLayerMetric_tests.cpp','Common/include/adaptation/CNativeMetric3D.hpp','UnitTests/Common/adaptation/CNativeDistributed3D_tests.cpp','Common/src/adaptation/CNativeDistributed3D.cpp','Common/include/adaptation/CNativeField3D.hpp','Common/src/adaptation/CNativeField3D.cpp','Common/src/adaptation/CNativeMetric3D.cpp','Common/include/adaptation/CNativeBoundary3D.hpp','Common/src/adaptation/CNativeBoundary3D.cpp','UnitTests/Common/adaptation/CNativeBoundary3D_tests.cpp','Common/include/adaptation/CNativeRemesher3D.hpp','Common/src/adaptation/CNativeRemesher3D.cpp','Common/include/adaptation/CNativeEngine3D.hpp','Common/src/adaptation/CNativeEngine3D.cpp','Common/src/adaptation/CNativeImport3D.cpp','Common/src/adaptation/CNativeReferenceIO3D.cpp','SU2_CFD/include/adaptation/CBoundaryLayerMetric.hpp','SU2_CFD/src/adaptation/CBoundaryLayerMetric.cpp','SU2_CFD/src/drivers/CSinglezoneDriver.cpp','SU2_CFD/src/solvers/CSolver.cpp','UnitTests/SU2_CFD/adaptation/CNativeImport3D_tests.cpp',str(Path(__file__).resolve().relative_to(ROOT))]
 sources.extend(['Common/include/adaptation/CNativeDistributed3D.hpp','Common/include/adaptation/CNativeTopology3D.hpp','Common/include/adaptation/CNativeCavity3D.hpp','Common/src/adaptation/CNativeCavity3D.cpp','UnitTests/Common/adaptation/CNativeCavity3D_tests.cpp'])
 sources.extend(['Common/include/adaptation/CNativeRegionField3D.hpp','Common/src/adaptation/CNativeRegionField3D.cpp','Common/include/adaptation/CNativeDonor3D.hpp','Common/src/adaptation/CNativeDonor3D.cpp'])
 sources.extend(['Common/include/adaptation/CNativeRegion3D.hpp','Common/src/adaptation/CNativeRegion3D.cpp','Common/src/meson.build'])
 if case=='native_cold_field_v1':sources.append('integration_evidence/native_3d_bl_v1/region_construction_v1/construct.py')
 sources.append('UnitTests/SU2_CFD/adaptation/CNativeRemesher3D_tests.cpp')
 sources.append('UnitTests/SU2_CFD/adaptation/fixtures/native_curved_rans_runtime.inc')
 sources.append('UnitTests/SU2_CFD/adaptation/fixtures/native_region_core_fields.inc')
 sources.extend(['UnitTests/SU2_CFD/adaptation/CConservativeTransfer_tests.cpp','UnitTests/SU2_CFD/adaptation/fixtures/native_saved_sa_projection.inc','SU2_CFD/include/adaptation/CConservativeTransfer.hpp','SU2_CFD/src/adaptation/CConservativeTransfer.cpp'])
 sources.extend(['UnitTests/Common/adaptation/CNativeMetric3D_tests.cpp','UnitTests/Common/adaptation/CNativeField3D_tests.cpp','UnitTests/Common/adaptation/fixtures/native_joint_obstruction.inc'])
 sources.append('UnitTests/Common/adaptation/fixtures/native_bl_planar_wall.inc')
 sources.extend(['UnitTests/SU2_CFD/adaptation/fixtures/native_curved_wall_blockage.inc','UnitTests/SU2_CFD/adaptation/fixtures/native_curved_remaining.inc'])
 pins={p:digest(ROOT/p) for p in sources}
 if stage in ('curved_grid_audit','curved_cold_residual','curved_grid_refine','curved_transition_complete','curved_joint_engine','curved_edge_replay','curved_joint_search','curved_interior_search','curved_interior_engine','curved_interior_residual','curved_interior_continue','curved_reconnection_probe','curved_cover_probe','curved_cap_cover_probe','curved_cap_point_probe','curved_costed_complete','curved_wall_refit_probe','curved_rim_refit_probe','curved_current_wall_refit_probe','curved_wall_refit_complete','curved_joint_connectivity_complete','curved_feature_complete','curved_wall_refit_residual','curved_joint_connectivity_residual','curved_feature_residual','curved_refit_interior_point','curved_refit_interior_cover','curved_refit_interior_dependencies','curved_joint_protocol','curved_joint_protocol_mpi2','curved_joint_protocol_cap_mpi2','curved_joint_protocol_refit_mpi2','curved_joint_protocol_connectivity_mpi2','curved_joint_protocol_feature_mpi2','curved_joint_protocol_mpi4'):
  grid_stage='curved_cold' if (HERE/'curved_cold/curved_cylinder_BL_rejected.su2').is_file() else 'curved_cold_complete_rejected_v1'
  if stage=='curved_grid_refine':grid_stage='curved_cold_complete_rejected_v1'
  grid_name='curved_cylinder_BL_rejected.su2'
  if stage in ('curved_interior_search','curved_interior_engine') or stage.startswith('curved_joint_protocol'):grid_stage='curved_joint_engine';grid_name='curved_grid_partial_rejected.su2'
  if stage in ('curved_interior_residual','curved_interior_continue'):grid_stage='curved_interior_engine';grid_name='curved_grid_partial_rejected.su2'
  if stage in ('curved_reconnection_probe','curved_cover_probe','curved_cap_cover_probe','curved_cap_point_probe','curved_costed_complete','curved_wall_refit_probe','curved_rim_refit_probe','curved_current_wall_refit_probe','curved_wall_refit_complete','curved_joint_connectivity_complete','curved_feature_complete'):grid_stage='curved_interior_continue';grid_name='curved_grid_partial_rejected.su2'
  if stage in ('curved_joint_protocol_refit_mpi2','curved_joint_protocol_connectivity_mpi2','curved_joint_protocol_feature_mpi2'):grid_stage='curved_interior_continue';grid_name='curved_grid_partial_rejected.su2'
  if stage in ('curved_wall_refit_residual','curved_joint_connectivity_residual','curved_feature_residual','curved_refit_interior_point','curved_refit_interior_cover','curved_refit_interior_dependencies'):grid_stage='curved_wall_refit_complete';grid_name='curved_grid_partial_rejected.su2'
  if stage=='curved_joint_connectivity_residual':grid_stage='curved_joint_connectivity_complete';grid_name='curved_grid_partial_rejected.su2'
  if stage=='curved_feature_residual':grid_stage='curved_feature_complete';grid_name='curved_grid_partial_rejected.su2'
  if stage in ('curved_rim_refit_probe','curved_current_wall_refit_probe'):grid_stage='curved_joint_connectivity_complete';grid_name='curved_grid_partial_rejected.su2'
  if case.startswith('native_complete') or case.startswith('native_field_compat'):grid_stage='curved_interior_continue';grid_name='curved_grid_partial_rejected.su2'
  if case=='curved_metric_patch_probe':grid_stage='curved_feature_complete'
  if case=='curved_metric_residual':grid_stage='curved_metric_complete'
  if case in field_sources:
   source_case,grid_name=field_sources[case];grid_stage=HERE.parent/'region_construction_v1'/source_case
  if case=='native_cold_field_v1':
   grid_stage=HERE.parent/'region_construction_v1/native_cold_v1';grid_name='region_proposal_rejected.su2'
   cold=json.loads(Path(str(grid_stage)+'.json').read_text());assert cold['status']=='FAIL'
   proposal=grid_stage/'curved_rejected_patch_rank0_proposal.json'
   assert digest(proposal)==cold['artifacts'][proposal.name]
   import importlib.util,shutil
   helper=ROOT/'integration_evidence/native_3d_bl_v1/region_construction_v1/construct.py'
   spec=importlib.util.spec_from_file_location('native_inspection',helper);module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
   data=json.loads(proposal.read_text())
   module.write_su2(grid_stage/grid_name,{n[0]:n[1:] for n in data['nodes']},[c['nodes'] for c in data['cells']],data['surface'],{f['id']:f['marker'] for f in data['facets']},data['marker_names'])
   shutil.copyfile(grid_stage/'curved_cylinder_initial_euler.su2.native_ref',grid_stage/(grid_name+'.native_ref'))
   pins['derived_proposal_json']=digest(proposal)
  grid=HERE/grid_stage/grid_name;grid_receipt=json.loads((HERE/(str(grid_stage)+'.json')).read_text())
  assert grid_receipt['status']==('PASS' if case in field_sources else 'FAIL')
  if case!='native_cold_field_v1':assert digest(grid)==grid_receipt['artifacts'][grid.name]
  pins['saved_grid']=digest(grid);pins['saved_grid_receipt']=digest(HERE/(str(grid_stage)+'.json'))
 pins['build.ninja']=digest(BUILD/'build.ninja');pins['compile_commands.json']=digest(BUILD/'compile_commands.json');pins['test_binary']=digest(BINARY);pins['solver_binary']=digest(BUILD/'SU2_CFD/src/SU2_CFD')
 for library in ['Common/src/libSU2Common.a','Common/src/libSU2AccurateSum.a','SU2_CFD/src/libSU2core.a']:
  path=BUILD/library;pins[library]=digest(path)
  with path.open('rb') as f:thin=f.read(8)==b'!<thin>\n'
  if thin:
   for member in subprocess.check_output(['ar','t',str(path)],cwd=ROOT,universal_newlines=True).splitlines():
    f=Path(member);f=f if f.is_absolute() else ROOT/f;pins['object:'+str(f)]=digest(f)
 command=['taskset','-c','5' if ranks==1 else '0,1,2,3','nice','-n','19']
 if ranks>1:command+=['mpirun','--oversubscribe','--bind-to','none','-np',str(ranks)]
 command+=[str(BINARY),selection,'--reporter','compact']
 env=dict({k:v for k,v in os.environ.items() if not k.startswith('SU2_NATIVE_CURVED_')},OMP_NUM_THREADS='1',OPENBLAS_NUM_THREADS='1',MKL_NUM_THREADS='1',OMPI_MCA_mpi_yield_when_idle='1',PYTHONDONTWRITEBYTECODE='1',SU2_NATIVE_CURVED_EVIDENCE='1')
 if stage.startswith('curved_replay'):env['SU2_NATIVE_CURVED_REPLAY']={'curved_replay':'1','curved_replay_remaining':'remaining','curved_replay_complete':'complete'}[stage]
 if stage in ('curved_grid_audit','curved_cold_residual','curved_grid_refine','curved_transition_complete','curved_joint_engine','curved_edge_replay','curved_joint_search','curved_interior_search','curved_interior_engine','curved_interior_residual','curved_interior_continue','curved_reconnection_probe','curved_cover_probe','curved_cap_cover_probe','curved_cap_point_probe','curved_costed_complete','curved_wall_refit_probe','curved_rim_refit_probe','curved_current_wall_refit_probe','curved_wall_refit_complete','curved_joint_connectivity_complete','curved_feature_complete','curved_wall_refit_residual','curved_joint_connectivity_residual','curved_feature_residual','curved_refit_interior_point','curved_refit_interior_cover','curved_refit_interior_dependencies','curved_joint_protocol','curved_joint_protocol_mpi2','curved_joint_protocol_cap_mpi2','curved_joint_protocol_refit_mpi2','curved_joint_protocol_connectivity_mpi2','curved_joint_protocol_feature_mpi2','curved_joint_protocol_mpi4'):env['SU2_NATIVE_CURVED_GRID_AUDIT']='saved_grid.su2'
 if stage in ('curved_grid_refine','curved_transition_complete','curved_joint_engine','curved_interior_engine','curved_interior_continue','curved_costed_complete','curved_wall_refit_complete','curved_joint_connectivity_complete','curved_feature_complete'):env['SU2_NATIVE_CURVED_GRID_REFINE']='1'
 if stage in ('curved_transition_complete','curved_joint_engine','curved_interior_engine','curved_interior_continue','curved_costed_complete','curved_wall_refit_complete','curved_joint_connectivity_complete','curved_feature_complete'):env['SU2_NATIVE_CURVED_GRID_COMPLETE']='1'
 if stage in ('curved_edge_replay','curved_joint_search','curved_interior_search','curved_reconnection_probe','curved_cover_probe','curved_cap_cover_probe','curved_cap_point_probe','curved_wall_refit_probe','curved_rim_refit_probe','curved_current_wall_refit_probe','curved_refit_interior_point','curved_refit_interior_cover','curved_refit_interior_dependencies'):env['SU2_NATIVE_CURVED_EDGE_REPLAY']='1'
 if stage in ('curved_joint_search','curved_interior_search','curved_reconnection_probe','curved_cover_probe','curved_cap_cover_probe','curved_cap_point_probe','curved_refit_interior_point','curved_refit_interior_cover','curved_refit_interior_dependencies'):env['SU2_NATIVE_CURVED_JOINT_SEARCH']='1'
 if stage in ('curved_interior_search','curved_reconnection_probe','curved_cover_probe','curved_refit_interior_point','curved_refit_interior_cover','curved_refit_interior_dependencies'):env['SU2_NATIVE_CURVED_INTERIOR_SEARCH']='1'
 if stage=='curved_refit_interior_dependencies':env['SU2_NATIVE_CURVED_ADMIT_HINTS']='1'
 if stage in ('curved_wall_refit_probe','curved_rim_refit_probe','curved_current_wall_refit_probe'):env['SU2_NATIVE_CURVED_WALL_REFIT']='1'
 if stage=='curved_rim_refit_probe':env['SU2_NATIVE_CURVED_RIM_REFIT']='1'
 if stage=='curved_cap_point_probe':env['SU2_NATIVE_CURVED_POINT_ONLY']='1'
 if stage in ('curved_cover_probe','curved_cap_cover_probe','curved_refit_interior_cover','curved_refit_interior_dependencies'):env['SU2_NATIVE_CURVED_COUPLED_COVER']='1'
 if stage.startswith('curved_joint_protocol'):env['SU2_NATIVE_CURVED_PROTOCOL']='1'
 if case=='curved_metric_patch_probe':env['SU2_NATIVE_CURVED_METRIC_SOLVE']='1'
 if case.startswith('native_curved_rans'):env['SU2_NATIVE_POLICY_WORKERS']=runtime[2] if runtime else str(ranks)
 if case.startswith('native_refreshed_source'):env['SU2_NATIVE_CURVED_REFRESH_SOURCE']='1'
 env.pop('SU2_NATIVE_FRONT_HEIGHT',None);env.pop('SU2_NATIVE_FRONT_LENGTH',None)
 if core:env['SU2_NATIVE_CORE_FIELD']=str({'constant':0,'varying':1,'rotated':2}[core[1]])
 if layered:env.update(SU2_NATIVE_FRONT_HEIGHT='.01',SU2_NATIVE_FRONT_LENGTH='.2')
 contract={'recipe':stage,'pins':pins,'command':command,'MPI_ranks':ranks,'environment':{k:v for k,v in env.items() if k.startswith('SU2_NATIVE_CURVED_') or k in ('SU2_NATIVE_CORE_FIELD','SU2_NATIVE_FRONT_HEIGHT','SU2_NATIVE_FRONT_LENGTH','SU2_NATIVE_POLICY_WORKERS','OMP_NUM_THREADS','OPENBLAS_NUM_THREADS','MKL_NUM_THREADS','OMPI_MCA_mpi_yield_when_idle')}}
 destination=HERE.parent/'interacting_walls_v1' if opposing else HERE.parent/'region_construction_v1' if case.startswith('native_') else HERE
 destination.mkdir(exist_ok=True)
 record=destination/(case+'.json');folder=destination/case
 if case in field_sources:
  source_case=field_sources[case][0];record=destination/source_case/'native_field.json';folder=destination/source_case/'native_field'
  assert pins['test_binary']==grid_receipt['contract']['pins']['test_binary'],'Accepted engine and field audit binaries differ'
  contract['accepted_engine_receipt_sha256']=pins['saved_grid_receipt']
 print('Case folder:',folder,flush=True)
 archive=str(time.time_ns())
 if record.exists():
  old=json.loads(record.read_text())
  if old.get('status') in ('PASS','PASS_NATIVE_FIELD_ONLY') and old.get('contract')==contract and all(digest(folder/f)==sha for f,sha in old['artifacts'].items()):print('Reused PASS',flush=True);return 0
  record.rename(record.with_name(record.stem+'_previous_'+archive+record.suffix))
 if folder.exists():folder.rename(folder.with_name(folder.name+'_previous_'+archive))
 folder.mkdir()
 if stage in ('curved_grid_audit','curved_cold_residual','curved_grid_refine','curved_transition_complete','curved_joint_engine','curved_edge_replay','curved_joint_search','curved_interior_search','curved_interior_engine','curved_interior_residual','curved_interior_continue','curved_reconnection_probe','curved_cover_probe','curved_cap_cover_probe','curved_cap_point_probe','curved_costed_complete','curved_wall_refit_probe','curved_rim_refit_probe','curved_current_wall_refit_probe','curved_wall_refit_complete','curved_joint_connectivity_complete','curved_feature_complete','curved_wall_refit_residual','curved_joint_connectivity_residual','curved_feature_residual','curved_refit_interior_point','curved_refit_interior_cover','curved_refit_interior_dependencies','curved_joint_protocol','curved_joint_protocol_mpi2','curved_joint_protocol_cap_mpi2','curved_joint_protocol_refit_mpi2','curved_joint_protocol_connectivity_mpi2','curved_joint_protocol_feature_mpi2','curved_joint_protocol_mpi4'):(folder/'saved_grid.su2').write_bytes(grid.read_bytes())
 for source in sources:
  out=folder/'source_snapshots'/source;out.parent.mkdir(parents=True,exist_ok=True);out.write_bytes((ROOT/source).read_bytes())
 log=folder/'native.log';start=time.monotonic()
 record.write_text(json.dumps({'status':'RUNNING','contract':contract,'collector_pid':os.getpid(),'scope':'Execution intent only; verify actual live process before restart; not a passing receipt.'},indent=2)+'\n')
 with log.open('w') as out:
  try:code=subprocess.run(command,cwd=folder,env=env,stdout=out,stderr=subprocess.STDOUT,timeout=420).returncode
  except subprocess.TimeoutExpired:code=124
 text=log.read_text();counts=re.findall(r'Passed (?:(?:all|both) )?(\d+) test cases? with ',text);passed=code==0 and counts==[str(expected)]*ranks
 assert all(digest(ROOT/p)==pins[p] for p in sources),'Sources changed during stage'
 assert digest(BINARY)==pins['test_binary'],'Binary changed during stage'
 result={'status':'PASS' if passed else 'FAIL','returncode':code,'contract':contract,'elapsed_seconds':time.monotonic()-start,'artifacts':{str(f.relative_to(folder)):digest(f) for f in folder.rglob('*') if f.is_file()},'stage':case,'scope':('Continuation of the source-pinned1110-cell result, with the identical original frozen field and another bounded4096-work invocation. Report cumulative work from the473-cell chain; not a single-event/cold-start completion or a changed production limit. ' if stage=='curved_interior_continue' else '')+('Bounded actual-engine protocol/work-limit/source-preservation check; passing requires joint-placement execution. Not final mesh acceptance, CFD, restart or performance qualification. ' if stage.startswith('curved_joint_protocol') else '')+('Complete original-field refinement of the saved rejected grid is required; no cold-start, CFD transfer, restart or performance qualification. ' if stage in ('curved_transition_complete','curved_joint_engine','curved_interior_engine','curved_interior_continue','curved_costed_complete','curved_wall_refit_complete','curved_joint_connectivity_complete','curved_feature_complete') else '')+('Frozen-field saved-grid diagnosis or bounded real-engine repair; PASS establishes the declared partial-progress check, not final mesh acceptance. ' if stage in ('curved_grid_audit','curved_cold_residual','curved_grid_refine','curved_transition_complete','curved_joint_engine','curved_edge_replay','curved_joint_search','curved_interior_search','curved_interior_engine','curved_interior_residual','curved_interior_continue','curved_reconnection_probe','curved_cover_probe','curved_cap_cover_probe','curved_cap_point_probe','curved_costed_complete','curved_wall_refit_probe','curved_rim_refit_probe','curved_current_wall_refit_probe','curved_wall_refit_complete','curved_joint_connectivity_complete','curved_feature_complete','curved_wall_refit_residual','curved_joint_connectivity_residual','curved_feature_residual','curved_refit_interior_point','curved_refit_interior_cover','curved_refit_interior_dependencies','curved_joint_protocol','curved_joint_protocol_mpi2','curved_joint_protocol_cap_mpi2','curved_joint_protocol_refit_mpi2','curved_joint_protocol_connectivity_mpi2','curved_joint_protocol_feature_mpi2','curved_joint_protocol_mpi4') else '')+('Exact curved cavity shape/reconnection diagnosis only, not full construction/removal or accepted replacement. ' if stage.startswith('curved_replay') else '')+'Selected focused geometry/field/integration control or complete declared cold construction/removal probe. Curved stages use actual smooth reference and original sensors; cold case imports initialized RANS CFD geometry but does not run replacement/solution transfer or CFD iterations. Independent audit and full-goal qualification remain required; no performance claim.'}
 if case in field_sources and passed:
  result['metrics']=json.loads((folder/'grid_metric_summary.json').read_text())
  passed=result['metrics']['shape_failures']==0 and result['metrics']['oversized_edges']==0
  result['status']='PASS_NATIVE_FIELD_ONLY' if passed else 'REJECTED_NATIVE_METRIC'
 if opposing:result['scope']='Complete opposing planar BL construction/removal target with overlapping thickness .6 in a unit gap, first height .02, growth1.2, immutable original P1 sensor4*(1+3*x) I. Actual geometric composition/full edges, original q/J/work/time gates. Native acceptance and independent geometry/field proof are separate; no CFD or general interacting geometry qualification.'
 if edge_controls:result['scope']='Focused actual geometric BL edge-band/rotation controls, original-P1 physical/collar priority and exact sensor edges, metric-balanced placements, global composed-edge error with tiny noise and high-demand thin bands. No complete mesh, MPI/CFD/restart/accuracy or performance qualification.'
 if core:result['scope']='Complete initialized-RANS geometry native construction with a stronger frozen original-P1 sensor, actual geometric BL/fade, unchanged geometry/h/q/J/full-edge/work/time gates, source-preserving publication and exported field/mesh records. Independent audit remains separate. No CFD evolution, solution transfer/restart, general geometry or performance qualification.'
 if case.startswith('native_curved_rans'):result['scope']='Actual short curved RANS/SA evolution, native BL construction, current/BDF conservative transfer, SU2/CGNS output and real restart, subsequent sensor-only remeshing from refreshed pressure-derived metric and further CFD steps. Strict geometry/height/quality/full-edge gates unchanged. N/M coverage follows configured selection. Not converged viscous accuracy, general geometry or performance/scaling qualification.'
 if case.startswith('native_complete'):result['scope']='Complete original1402-cell curved target native generation/publication with unchanged original-P1 sensors/geometric BL/fade/geometry/h/q/J/full lengths,4096 charged construction work and300s. Atomic accepted-input immutability checked. Independent geometry audit, CFD/transfer/restart/wider geometries/performance qualification remain separate.'
 if case in field_sources:result['scope']='Read-only actual-centroid and full-edge remeasurement of a separately accepted native engine candidate; accepted-engine receipt hash is in contract. No additional CFD/MPI/restart/physical-accuracy or performance qualification.'
 record.write_text(json.dumps(result,indent=2)+'\n');print(result['status'],flush=True);return 0 if passed else 1
if __name__=='__main__':sys.exit(main())
