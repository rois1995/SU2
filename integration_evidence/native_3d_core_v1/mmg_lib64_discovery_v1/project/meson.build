project('mmg_directory_check','cpp', default_options:['cpp_std=c++17'])
su2_deps=[]
su2_cpp_args=[]
# MMG (in-memory mesh adaptation), static or shared libmmg with its optional SCOTCH dependency
if get_option('enable-mmg')
  cpp = meson.get_compiler('cpp')
  mmg_root = get_option('mmg_root')
  mmg_deps = [cpp.find_library('mmg', dirs: [mmg_root / 'lib', mmg_root / 'lib64'], required: true)]

  scotch_root = get_option('mmg_scotch_root')
  scotch_dirs = scotch_root != '' ? [scotch_root / 'lib', scotch_root / 'lib64'] : []
  scotch_required = scotch_root != ''
  scotch_lib = cpp.find_library('scotch', dirs: scotch_dirs, required: scotch_required)
  if scotch_lib.found()
    mmg_deps += [scotch_lib, cpp.find_library('scotcherrexit', dirs: scotch_dirs, required: scotch_required),
                 dependency('zlib', required: false)]
  endif
  mmg_deps += [dependency('threads'), cpp.find_library('m', required: false), cpp.find_library('rt', required: false)]

  mmg_dep = declare_dependency(include_directories: include_directories(mmg_root / 'include', is_system: true),
                               dependencies: mmg_deps)

  mmg_test = '''#include "mmg/libmmg.h"
  int main() {
    MMG5_pMesh mesh = nullptr;
    MMG5_pSol met = nullptr;
    MMG2D_Init_mesh(MMG5_ARG_start, MMG5_ARG_ppMesh, &mesh, MMG5_ARG_ppMet, &met, MMG5_ARG_end);
    MMG2D_mmg2dlib(mesh, met);
    MMG2D_Free_all(MMG5_ARG_start, MMG5_ARG_ppMesh, &mesh, MMG5_ARG_ppMet, &met, MMG5_ARG_end);
    MMG3D_Init_mesh(MMG5_ARG_start, MMG5_ARG_ppMesh, &mesh, MMG5_ARG_ppMet, &met, MMG5_ARG_end);
    MMG3D_mmg3dlib(mesh, met);
    MMG3D_Free_all(MMG5_ARG_start, MMG5_ARG_ppMesh, &mesh, MMG5_ARG_ppMet, &met, MMG5_ARG_end);
    return 0;
  }'''
  assert(cpp.links(mmg_test, dependencies: mmg_dep, name: 'MMG library'),
         'Cannot link MMG from ' + mmg_root + '. Set -Dmmg_root (and -Dmmg_scotch_root if MMG was built with SCOTCH).')

  su2_deps     += mmg_dep
  su2_cpp_args += '-DHAVE_MMG'
endif
