import SU2Euler.Parser

set_option maxRecDepth 100000
set_option maxHeartbeats 2000000

namespace SU2Euler.Generated

def update_originalSource : String := "inline void AddSolution(unsigned long iPoint, unsigned long iVar, su2double solution) {\n    Solution(iPoint, iVar) = Solution_Old(iPoint, iVar) + solution;\n  }"
def update_originalCode : SU2Source.Program := (SU2Source.parse update_originalSource).get (by decide)
theorem update_original_parsed : SU2Source.parse update_originalSource = some update_originalCode := by decide

def update_subtractSource : String := "inline void AddSolution(unsigned long iPoint, unsigned long iVar, su2double solution) {\n    Solution(iPoint, iVar) = Solution_Old(iPoint, iVar) - solution;\n  }"
def update_subtractCode : SU2Source.Program := (SU2Source.parse update_subtractSource).get (by decide)
theorem update_subtract_parsed : SU2Source.parse update_subtractSource = some update_subtractCode := by decide

def update_currentReadSource : String := "inline void AddSolution(unsigned long iPoint, unsigned long iVar, su2double solution) {\n    Solution(iPoint, iVar) = Solution(iPoint, iVar) + solution;\n  }"
def update_currentReadCode : SU2Source.Program := (SU2Source.parse update_currentReadSource).get (by decide)
theorem update_currentRead_parsed : SU2Source.parse update_currentReadSource = some update_currentReadCode := by decide

def update_wrongTargetSource : String := "inline void AddSolution(unsigned long iPoint, unsigned long iVar, su2double solution) {\n    Solution_Old(iPoint, iVar) = Solution_Old(iPoint, iVar) + solution;\n  }"
def update_wrongTargetCode : SU2Source.Program := (SU2Source.parse update_wrongTargetSource).get (by decide)
theorem update_wrongTarget_parsed : SU2Source.parse update_wrongTargetSource = some update_wrongTargetCode := by decide

def update_refactoredSource : String := "inline void AddSolution(unsigned long point, unsigned long component, su2double increment) {\n    const su2double previous = Solution_Old(point, component);\n    Solution(point, component) = previous + increment;\n  }"
def update_refactoredCode : SU2Source.Program := (SU2Source.parse update_refactoredSource).get (by decide)
theorem update_refactored_parsed : SU2Source.parse update_refactoredSource = some update_refactoredCode := by decide

def scatter_originalSource : String := "template <size_t N, class T, class VecTypeSIMD, class F = ScalarType>\n  FORCEINLINE void UpdateBlocks(simd::Array<T, N> iPoint, simd::Array<T, N> jPoint, const VecTypeSIMD& vector,\n                                simd::Array<F, N> mask = 1) {\n    /*--- \"Transpose\" and scale input vector. ---*/\n    constexpr size_t nVar = VecTypeSIMD::StaticSize;\n    assert(nVar == this->nVar);\n    ScalarType vec[N][nVar];\n    UnpackBlock(vector, mask, vec);\n\n    /*--- Update one by one skipping if mask is 0. ---*/\n    for (size_t k = 0; k < N; ++k) {\n      if (mask[k] == 0) continue;\n      SU2_OMP_SIMD\n      for (size_t i = 0; i < nVar; ++i) {\n        vec_val[iPoint[k] * nVar + i] += vec[k][i];\n        vec_val[jPoint[k] * nVar + i] -= vec[k][i];\n      }\n    }\n  }"
def scatter_originalCode : ScatterCode := (parseScatter scatter_originalSource).get (by decide)
theorem scatter_original_parsed : parseScatter scatter_originalSource = some scatter_originalCode := by decide

def scatter_refactoredSource : String := "template <size_t N, class T, class VecTypeSIMD, class F = ScalarType>\n  FORCEINLINE void UpdateBlocks(simd::Array<T, N> iPoint, simd::Array<T, N> jPoint, const VecTypeSIMD& vector,\n                                simd::Array<F, N> mask = 1) {\n    /*--- \"Transpose\" and scale input vector. ---*/\n    constexpr size_t nVar = VecTypeSIMD::StaticSize;\n    assert(nVar == this->nVar);\n    ScalarType unpacked[N][nVar];\n    UnpackBlock(vector, mask, unpacked);\n\n    /*--- Update one by one skipping if mask is 0. ---*/\n    for (size_t k = 0; k < N; ++k) {\n      if (mask[k] == 0) continue;\n      SU2_OMP_SIMD\n      for (size_t i = 0; i < nVar; ++i) {\n        vec_val[iPoint[k] * nVar + i] += unpacked[k][i];\n        vec_val[jPoint[k] * nVar + i] -= unpacked[k][i];\n      }\n    }\n  }"
def scatter_refactoredCode : ScatterCode := (parseScatter scatter_refactoredSource).get (by decide)
theorem scatter_refactored_parsed : parseScatter scatter_refactoredSource = some scatter_refactoredCode := by decide

def scatter_wrongSignSource : String := "template <size_t N, class T, class VecTypeSIMD, class F = ScalarType>\n  FORCEINLINE void UpdateBlocks(simd::Array<T, N> iPoint, simd::Array<T, N> jPoint, const VecTypeSIMD& vector,\n                                simd::Array<F, N> mask = 1) {\n    /*--- \"Transpose\" and scale input vector. ---*/\n    constexpr size_t nVar = VecTypeSIMD::StaticSize;\n    assert(nVar == this->nVar);\n    ScalarType vec[N][nVar];\n    UnpackBlock(vector, mask, vec);\n\n    /*--- Update one by one skipping if mask is 0. ---*/\n    for (size_t k = 0; k < N; ++k) {\n      if (mask[k] == 0) continue;\n      SU2_OMP_SIMD\n      for (size_t i = 0; i < nVar; ++i) {\n        vec_val[iPoint[k] * nVar + i] += vec[k][i];\n        vec_val[jPoint[k] * nVar + i] += vec[k][i];\n      }\n    }\n  }"
def scatter_wrongSignCode : ScatterCode := (parseScatter scatter_wrongSignSource).get (by decide)
theorem scatter_wrongSign_parsed : parseScatter scatter_wrongSignSource = some scatter_wrongSignCode := by decide

def scatter_wrongMaskSource : String := "template <size_t N, class T, class VecTypeSIMD, class F = ScalarType>\n  FORCEINLINE void UpdateBlocks(simd::Array<T, N> iPoint, simd::Array<T, N> jPoint, const VecTypeSIMD& vector,\n                                simd::Array<F, N> mask = 1) {\n    /*--- \"Transpose\" and scale input vector. ---*/\n    constexpr size_t nVar = VecTypeSIMD::StaticSize;\n    assert(nVar == this->nVar);\n    ScalarType vec[N][nVar];\n    UnpackBlock(vector, mask, vec);\n\n    /*--- Update one by one skipping if mask is 0. ---*/\n    for (size_t k = 0; k < N; ++k) {\n      if (mask[k] != 0) continue;\n      SU2_OMP_SIMD\n      for (size_t i = 0; i < nVar; ++i) {\n        vec_val[iPoint[k] * nVar + i] += vec[k][i];\n        vec_val[jPoint[k] * nVar + i] -= vec[k][i];\n      }\n    }\n  }"
def scatter_wrongMaskCode : ScatterCode := (parseScatter scatter_wrongMaskSource).get (by decide)
theorem scatter_wrongMask_parsed : parseScatter scatter_wrongMaskSource = some scatter_wrongMaskCode := by decide

def apply_originalSource : String := "FORCEINLINE su2double apply(unsigned short iVar, const su2double* res, const su2double* resTrunc) {\n        return res[iVar] + resTrunc[iVar];\n      }"
def apply_originalCode : Op := (parseApply apply_originalSource).get (by decide)
theorem apply_original_parsed : parseApply apply_originalSource = some apply_originalCode := by decide

def apply_subtractTruncSource : String := "FORCEINLINE su2double apply(unsigned short iVar, const su2double* res, const su2double* resTrunc) {\n        return res[iVar] - resTrunc[iVar];\n      }"
def apply_subtractTruncCode : Op := (parseApply apply_subtractTruncSource).get (by decide)
theorem apply_subtractTrunc_parsed : parseApply apply_subtractTruncSource = some apply_subtractTruncCode := by decide

def scale_originalSource : String := "su2double Vol = geometry->nodes->GetVolume(iPoint) + geometry->nodes->GetPeriodicVolume(iPoint);\n        su2double Delta = nodes->GetDelta_Time(iPoint) / Vol;"
def scale_originalCode : Unit := (parseScale scale_originalSource).get (by decide)
theorem scale_original_parsed : parseScale scale_originalSource = some scale_originalCode := by decide

def call_originalSource : String := "case EULER_EXPLICIT:\n              nodes->AddSolution(iPoint,iVar, -Res*Delta);\n              break;"
def call_originalCode : Bool := (parseCall call_originalSource).get (by decide)
theorem call_original_parsed : parseCall call_originalSource = some call_originalCode := by decide

def call_wrongSignSource : String := "case EULER_EXPLICIT:\n              nodes->AddSolution(iPoint,iVar, +Res*Delta);\n              break;"
def call_wrongSignCode : Bool := (parseCall call_wrongSignSource).get (by decide)
theorem call_wrongSign_parsed : parseCall call_wrongSignSource = some call_wrongSignCode := by decide

end SU2Euler.Generated
