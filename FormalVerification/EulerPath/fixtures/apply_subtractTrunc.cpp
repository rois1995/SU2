FORCEINLINE su2double apply(unsigned short iVar, const su2double* res, const su2double* resTrunc) {
        return res[iVar] - resTrunc[iVar];
      }
