"""Apply connected stage-review changes only AFTER the pinned first build terminates."""
from pathlib import Path

def edit(path,old,new):
 p=Path(path);s=p.read_text();assert s.count(old)==1,(path,s.count(old));p.write_text(s.replace(old,new,1))

edit('SU2_CFD/src/adaptation/CBoundaryLayerMetric.cpp','''  if(outerNormal.frozen || outerNormal.version || !(protectedDistance>0)''','''  if(nDim!=3 || outerNormal.frozen || outerNormal.version || !(protectedDistance>0)''')
edit('SU2_CFD/include/adaptation/CNativeNormalizationStage3D.hpp','''      family->normalization.status!=SU2Complexity::Status::EstimatedTolerance && selected.rows()''','''      (config.GetAdap_BL_Budget() || family->normalization.status!=SU2Complexity::Status::EstimatedTolerance) && selected.rows()''')
edit('SU2_CFD/include/adaptation/CNativeNormalizationStage3D.hpp','''      result.reason="outside_reassessed_scale_bounds";result.status=SU2Complexity::Status::OutsideBracket;return result;''','''      result.reason="outside_reassessed_scale_bounds";result.status=SU2Complexity::Status::OutsideBracket;
      if(config.GetAdap_BL_Budget() && current.value-current.uncertainty>target) {
        result.valid=false;result.reason="estimated_budget_incompatible_at_original_family_lower_bound";
      }
      return result;''')
edit('SU2_CFD/include/adaptation/CNativeNormalizationStage3D.hpp','''    if(!result.valid)return result;
    int changed=0;''','''    if(!result.valid)return result;
    if(config.GetAdap_BL_Budget() && result.status==SU2Complexity::Status::OutsideBracket &&
        result.estimate.value-result.estimate.uncertainty>target) {
      result.valid=false;result.reason="estimated_budget_incompatible_on_reassessed_original_family";return result;
    }
    int changed=0;''')
edit('Common/src/adaptation/CNativeRemesher3D.cpp','''        stats.normalization.reason="correction_stage_limit_budget_residual";
    }
    if(!ready || stage==1''','''        stats.normalization.reason="correction_stage_limit_budget_residual";
      if(config.GetAdap_BL_Budget() && stats.complexity.value-stats.complexity.uncertainty>target) {
        ready=false;reason="estimated_budget_incompatible_after_bounded_normalization_stage";
        stats.normalization.reason=reason;
      }
    }
    if(ready && config.GetAdap_BL_Budget() && !normalizer.propose && stats.complexity.estimated &&
        stats.complexity.value-stats.complexity.uncertainty>SU2_TYPE::GetValue(config.GetAdap_Complexity())) {
      ready=false;reason="estimated_budget_incompatible_without_original_family_correction";
      stats.normalization.reason=reason;
    }
    if(!ready || stage==1''')
p=Path('UnitTests/SU2_CFD/adaptation/fixtures/native_normalization_stages.inc');s=p.read_text();a=s.index('TEST_CASE("Opt-in BL budget selection');body=s[a:]
needle='ADAP_BL_OUTER_GROWTH_MAX= 2\\n';assert body.count(needle)==1;body=body.replace(needle,'ADAP_BL_OUTER_GROWTH_MAX= 10\\n',1)
needle='''  if(budget==20)CHECK(selected.parameter==0);''';assert body.count(needle)==1;body=body.replace(needle,needle+'\n  if(budget==3)CHECK(selected.parameter>0);',1)
needle='''  CConservativeTransfer transfer;driver->ReplaceMesh(result,transfer);''';assert body.count(needle)==1
body=body.replace(needle,'''  const auto authorityBefore=EncodeReference(*driver->Reference());
  std::ifstream savedConfig(name+".cfg");std::stringstream configText;configText<<savedConfig.rdbuf();auto text=configText.str();
  const std::string work="ADAP_NATIVE_MAX_WORK= 8192";const auto at=text.find(work);REQUIRE(at!=std::string::npos);
  text.replace(at,work.size(),"ADAP_NATIVE_MAX_WORK= 1");std::stringstream limitedText(text);std::unique_ptr<CConfig> limited;
  {Mute mute;limited=std::make_unique<CConfig>(limitedText,SU2_COMPONENT::SU2_CFD,false);}
  const auto refused=backend->Remesh(*limited,g,solver.GetNodes()->GetMetric());
  CHECK(refused.status!=CRemeshResult::Status::COMPLETE);CHECK_FALSE(refused.onAccepted);
  CHECK(State(g,solver)==before);CHECK(State(g,*driver->Turbulence())==beforeTurb);
  CHECK(EncodeReference(*driver->Reference())==authorityBefore);CHECK(family->layers->GetOuterNormal().version==selected.version);
  CConservativeTransfer transfer;driver->ReplaceMesh(result,transfer);''',1)
p.write_text(s[:a]+body)
