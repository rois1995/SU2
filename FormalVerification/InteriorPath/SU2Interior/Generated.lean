import SU2Interior.SourceGate

set_option maxRecDepth 100000

set_option maxHeartbeats 8000000

namespace SU2Interior.Generated

def time_originalSource : String := "nodes->SetDelta_Time(iPoint, min(Local_Delta_Time, config->GetMax_DeltaTime()));"

def time_original : Bool := true

theorem time_original_parsed : parseTimeStore time_originalSource = some time_original := by decide

def time_uncappedSource : String := "nodes->SetDelta_Time(iPoint, Local_Delta_Time);"

def time_uncapped : Bool := false

theorem time_uncapped_parsed : parseTimeStore time_uncappedSource = some time_uncapped := by decide

def time_refactoredSource : String := "nodes->SetDelta_Time(iPoint, std::min(Local_Delta_Time, config->GetMax_DeltaTime()));"

def time_refactored : Bool := true

theorem time_refactored_parsed : parseTimeStore time_refactoredSource = some time_refactored := by decide

def scatter_originalSource : String := "template <size_t N, class T, class VecTypeSIMD, class F = ScalarType>\n  FORCEINLINE void UpdateBlocks(simd::Array<T, N> iPoint, simd::Array<T, N> jPoint, const VecTypeSIMD& vector,\n                                simd::Array<F, N> mask = 1) {\n    /*--- \"Transpose\" and scale input vector. ---*/\n    constexpr size_t nVar = VecTypeSIMD::StaticSize;\n    assert(nVar == this->nVar);\n    ScalarType vec[N][nVar];\n    UnpackBlock(vector, mask, vec);\n\n    /*--- Update one by one skipping if mask is 0. ---*/\n    for (size_t k = 0; k < N; ++k) {\n      if (mask[k] == 0) continue;\n      SU2_OMP_SIMD\n      for (size_t i = 0; i < nVar; ++i) {\n        vec_val[iPoint[k] * nVar + i] += vec[k][i];\n        vec_val[jPoint[k] * nVar + i] -= vec[k][i];\n      }\n    }\n  }"

def scatter_original : SU2Euler.ScatterCode := ⟨true,.add,.sub⟩

theorem scatter_original_parsed : SU2Euler.parseScatter scatter_originalSource = some scatter_original := by decide

def scatter_wrongSignSource : String := "template <size_t N, class T, class VecTypeSIMD, class F = ScalarType>\n  FORCEINLINE void UpdateBlocks(simd::Array<T, N> iPoint, simd::Array<T, N> jPoint, const VecTypeSIMD& vector,\n                                simd::Array<F, N> mask = 1) {\n    /*--- \"Transpose\" and scale input vector. ---*/\n    constexpr size_t nVar = VecTypeSIMD::StaticSize;\n    assert(nVar == this->nVar);\n    ScalarType vec[N][nVar];\n    UnpackBlock(vector, mask, vec);\n\n    /*--- Update one by one skipping if mask is 0. ---*/\n    for (size_t k = 0; k < N; ++k) {\n      if (mask[k] == 0) continue;\n      SU2_OMP_SIMD\n      for (size_t i = 0; i < nVar; ++i) {\n        vec_val[iPoint[k] * nVar + i] += vec[k][i];\n        vec_val[jPoint[k] * nVar + i] += vec[k][i];\n      }\n    }\n  }"

def scatter_wrongSign : SU2Euler.ScatterCode := ⟨true,.add,.add⟩

theorem scatter_wrongSign_parsed : SU2Euler.parseScatter scatter_wrongSignSource = some scatter_wrongSign := by decide

def scatter_refactoredSource : String := "template <size_t N, class T, class VecTypeSIMD, class F = ScalarType>\n  FORCEINLINE void UpdateBlocks(simd::Array<T, N> iPoint, simd::Array<T, N> jPoint, const VecTypeSIMD& vector,\n                                simd::Array<F, N> mask = 1) {\n    /*--- \"Transpose\" and scale input vector. ---*/\n    constexpr size_t nVar = VecTypeSIMD::StaticSize;\n    assert(nVar == this->nVar);\n    ScalarType unpacked[N][nVar];\n    UnpackBlock(vector, mask, unpacked);\n\n    /*--- Update one by one skipping if mask is 0. ---*/\n    for (size_t k = 0; k < N; ++k) {\n      if (mask[k] == 0) continue;\n      SU2_OMP_SIMD\n      for (size_t i = 0; i < nVar; ++i) {\n        vec_val[iPoint[k] * nVar + i] += unpacked[k][i];\n        vec_val[jPoint[k] * nVar + i] -= unpacked[k][i];\n      }\n    }\n  }"

def scatter_refactored : SU2Euler.ScatterCode := ⟨true,.add,.sub⟩

theorem scatter_refactored_parsed : SU2Euler.parseScatter scatter_refactoredSource = some scatter_refactored := by decide

end SU2Interior.Generated
