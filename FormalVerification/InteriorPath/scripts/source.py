#!/usr/bin/env python3
"""Unverified extraction with exact byte spans; only declared leaf grammars are Lean parsed."""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import subprocess
ROOT = Path(__file__).resolve().parents[1]
REPO = ROOT.parents[1]
COMMIT = 'd162016382f0c122433000c114e88d808b48f7a7'
spec = importlib.util.spec_from_file_location('prior_selection',ROOT.parent/'EulerPath/scripts/source.py')
prior = importlib.util.module_from_spec(spec)
spec.loader.exec_module(prior)
def digest(raw): return hashlib.sha256(raw).hexdigest()
def generate(out):
    out = Path(out)
    (out/'fixtures').mkdir(parents=True,exist_ok=True)
    (out/'SU2Interior').mkdir(exist_ok=True)
    selections = {}
    files = {}
    def text(path):
        raw = prior.data(path)
        files[path] = dict(sha256=digest(raw),git_blob=subprocess.check_output(['git','-C',str(REPO),'rev-parse',COMMIT+':'+path],text=True).strip())
        return raw.decode()
    def add(key,path,selected,status='manual exact-real implementation model and source trace; not verified C++ translation'):
        start,end,raw = selected
        full = text(path)
        selections[key] = dict(path=path,span_bytes=[len(full[:start].encode()),len(full[:end].encode())],sha256=digest(raw.encode()),formal_status=status)
        (out/'fixtures'/(key+'.cpp')).write_text(raw+'\n')
        return raw
    def method(key,path,anchor): return add(key,path,prior.method(text(path),anchor))
    solver = 'SU2_CFD/src/solvers/CEulerSolver.cpp'
    base = 'SU2_CFD/include/solvers/CFVMFlowSolverBase.hpp'
    inl = 'SU2_CFD/include/solvers/CFVMFlowSolverBase.inl'
    vector = 'Common/include/linear_algebra/CSysVector.hpp'
    variable = 'SU2_CFD/include/variables/CVariable.hpp'
    method('timestepDispatch',solver,'void CEulerSolver::SetTime_Step(')
    method('timestep',base,'FORCEINLINE void SetTime_Step_impl(')
    method('upwindDispatch',solver,'void CEulerSolver::Upwind_Residual(')
    method('edgeLoop',inl,'void CFVMFlowSolverBase<V, R>::EdgeFluxResidual(')
    method('finalizeResidual',base,'void FinalizeResidualComputation(')
    edgeText = text(inl)
    loopStart,_,loopRaw = prior.method(edgeText,'for(auto k = 0ul; k < color.size; k += Double::Size)')
    add('packetTraversal',inl,(loopStart,loopStart+len(loopRaw),loopRaw))
    method('sourceResidual',solver,'void CEulerSolver::Source_Residual(')
    method('fluidInterface',inl,'void CFVMFlowSolverBase<V, FlowRegime>::BC_Fluid_Interface(')
    method('spaceIntegration','SU2_CFD/src/integration/CIntegration.cpp','void CIntegration::Space_Integration(')
    for key,anchor in [('setCFL','inline void SetLocalCFL('),('getCFL','inline su2double GetLocalCFL('),('setDt','inline void SetDelta_Time('),('getDt','inline su2double GetDelta_Time('),('setLambda','inline void SetMax_Lambda_Inv('),('addLambda','inline void AddMax_Lambda_Inv('),('getLambda','inline su2double GetMax_Lambda_Inv(')]: method(key,variable,anchor)
    method('getProjected','SU2_CFD/include/variables/CEulerVariable.hpp','inline su2double GetProjVel(')
    method('getSound','SU2_CFD/include/variables/CEulerVariable.hpp','inline su2double GetSoundSpeed(')
    for key,anchor in [('emptyViscous','inline virtual void Viscous_Residual(CGeometry'),('emptyFixed','virtual void Impose_Fixed_Values(')]: method(key,'SU2_CFD/include/solvers/CSolver.hpp',anchor)
    method('getVolume','Common/include/geometry/dual_grid/CPoint.hpp','inline const su2double& GetVolume(')
    method('setEdges','Common/src/geometry/CGeometry.cpp','void CGeometry::SetEdges()')
    method('edgeColoring','Common/src/geometry/CGeometry.cpp','const CCompressedSparsePatternUL& CGeometry::GetEdgeColoring(')
    method('naturalColoring','Common/include/toolboxes/graph_toolbox.hpp','T createNaturalColoring(')
    method('controlVolumes','Common/src/geometry/CPhysicalGeometry.cpp','void CPhysicalGeometry::SetControlVolume(CConfig* config, unsigned short action)')
    edge = 'Common/src/geometry/dual_grid/CEdge.cpp'
    method('dualVolume',edge,'su2double CEdge::GetVolume(const su2double* coord_Edge_CG, const su2double* coord_Elem_CG,')
    method('dualNormal',edge,'void CEdge::SetNodes_Coord(unsigned long iEdge, const su2double* coord_Edge_CG, const su2double* coord_Elem_CG)')
    common = 'SU2_CFD/include/numerics_simd/flow/convection/common.hpp'
    method('reconstruct',common,'FORCEINLINE CPair<ReconVarType> reconstructPrimitives(')
    method('pMatrix',common,'FORCEINLINE MatrixDbl<nDim+2> pMatrix(')
    method('inverseP',common,'FORCEINLINE MatrixDbl<nDim+2> pMatrixInv(')
    for key,anchor in [('conservatives','FORCEINLINE CCompressibleConservatives<nDim> compressibleConservatives('),('averages','FORCEINLINE CRoeVariables<nDim> roeAveragedVariables(')]: method(key,'SU2_CFD/include/numerics_simd/flow/variables.hpp',anchor)
    upwind = 'SU2_CFD/include/numerics_simd/flow/convection/upwind.hpp'
    method('computeFlux',upwind,'  void ComputeFlux(const Int iEdge,')
    raw = text(upwind)
    first = raw.index('class CRoeScheme :')
    a,b,body = prior.method(raw[first:raw.index('class CMSWScheme :',first)],'FORCEINLINE void finalizeFlux(')
    add('roeFinal',upwind,(first+a,first+b,body))
    for key,anchor in [('factory','CNumericsSIMD* CNumericsSIMD::CreateNumerics('),('idealFactory','CNumericsSIMD* createUpwindIdealNumerics('),('dimensionFactory','CNumericsSIMD* createNumerics(')]: method(key,'SU2_CFD/include/numerics_simd/CNumericsSIMD.cpp',anchor)
    scatter = method('scatter',vector,'template <size_t N, class T, class VecTypeSIMD, class F = ScalarType>\n  FORCEINLINE void UpdateBlocks')
    method('unpack',vector,'template <size_t N, size_t nVar, class VecTypeSIMD, class F>\n  FORCEINLINE static void UnpackBlock')
    # These additional complete files establish trace context even where no translator is provided.
    for path in ['SU2_CFD/include/numerics_simd/util.hpp','SU2_CFD/include/numerics_simd/flow/diffusion/viscous_fluxes.hpp','Common/include/CConfig.hpp','Common/src/CConfig.cpp','Common/include/geometry/dual_grid/CEdge.hpp','Common/include/containers/C2DContainer.hpp','SU2_CFD/include/solvers/CFVMFlowSolverBase.inl']:
        text(path)
    full = text(inl)
    # Constructor CFL stores have a different local name from dual-time resets.
    first = full.index('const auto CFL = config->GetCFL(MGLevel);')
    end = full.index('Avg_CFL_Local = CFL;',first)+len('Avg_CFL_Local = CFL;')
    add('initialCFL',inl,(first,end,full[first:end]))
    statement = 'nodes->SetDelta_Time(iPoint, min(Local_Delta_Time, config->GetMax_DeltaTime()));'
    for key,raw in [('localTime','su2double Local_Delta_Time = nodes->GetLocalCFL(iPoint)*Vol / nodes->GetMax_Lambda_Inv(iPoint);'),('resetLambda','nodes->SetMax_Lambda_Inv(iPoint,0.0);')]:
        add(key,base,prior.span(text(base),raw,raw))
    capped = add('timeStore',base,prior.span(text(base),statement,statement),'closed Lean leaf grammar with exact-real store semantics')
    lines = ['import SU2Interior.SourceGate','set_option maxRecDepth 100000','set_option maxHeartbeats 8000000','namespace SU2Interior.Generated']
    variants = {'original':capped,'uncapped':capped.replace('min(Local_Delta_Time, config->GetMax_DeltaTime())','Local_Delta_Time'),'refactored':capped.replace('min(Local_Delta_Time, config->GetMax_DeltaTime())','std::min(Local_Delta_Time, config->GetMax_DeltaTime())')}
    for name,body in variants.items():
        (out/'fixtures'/('timeStore_'+name+'.cpp')).write_text(body+'\n')
        lines += ['def time_'+name+'Source : String := '+json.dumps(body),'def time_'+name+' : Bool := '+str(name!='uncapped').lower(),'theorem time_'+name+'_parsed : parseTimeStore time_'+name+'Source = some time_'+name+' := by decide']
    for name,body in {'original':scatter,'wrongSign':scatter.replace('vec_val[jPoint[k] * nVar + i] -=','vec_val[jPoint[k] * nVar + i] +='),'refactored':scatter.replace('ScalarType vec[','ScalarType unpacked[').replace(', vec);',', unpacked);').replace('vec[k][i]','unpacked[k][i]')}.items():
        (out/'fixtures'/('scatter_'+name+'.cpp')).write_text(body+'\n')
        code = '⟨true,.add,.'+('add' if name=='wrongSign' else 'sub')+'⟩'
        lines += ['def scatter_'+name+'Source : String := '+json.dumps(body),'def scatter_'+name+' : SU2Euler.ScatterCode := '+code,'theorem scatter_'+name+'_parsed : SU2Euler.parseScatter scatter_'+name+'Source = some scatter_'+name+' := by decide']
    lines.append('end SU2Interior.Generated')
    (out/'SU2Interior/Generated.lean').write_text('\n\n'.join(lines)+'\n')
    manifest = dict(source_commit=COMMIT,production_patches=[],full_source_files=files,selections=selections,
        restriction='Only full SIMD scatter method and complete timestep-store statement are Lean parsed. Other C++ methods are manually mapped exact-real implementation models. Unsupported syntax in the declared grammars fails; all other syntax is explicitly outside the translator.',
        limitations='Unverified extraction/dispatch/accessor/geometry mapping, C++ heap/ABI/pointers/scratch allocation, floating-point/libm/compiler, MPI/OpenMP and complete numerical Roe specification are not proved.')
    (out/'source-manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
    return manifest
if __name__=='__main__':
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('output',type=Path)
    generate(p.parse_args().output)
