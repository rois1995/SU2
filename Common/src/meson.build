common_include = include_directories('./')
common_src =files(['graph_coloring_structure.cpp',
           'CConfig.cpp',
           'basic_types/ad_structure.cpp',
           'wall_model.cpp',
           '../include/parallelization/mpi_structure.cpp',
           '../include/parallelization/omp_structure.cpp',
           'parallelization/CPassiveComm.cpp'])

# The CUDA sources are kept in their own list because nvcc only accepts the defines out of
# su2_cpp_args, not the warning and tuning flags meant for the host compiler. They go into
# the primal and the reverse AD libraries with the same defines as the .cpp sources next to
# them, so su2double means the same thing everywhere; codi_host_only.hpp is what keeps
# CoDiPack out of the device pass. Forward AD gets no kernels, see SU2_ENABLE_CUDA_KERNELS.
common_cuda_src = []
common_cuda_cpp_args = []
foreach arg : su2_cpp_args
  if arg.startswith('-D') or arg.startswith('-U')
    common_cuda_cpp_args += arg
  endif
endforeach

# Note that the OpenMP flags do reach nvcc (through omp_dep), so the .cu objects agree
# with the rest of the library about HAVE_OMP. They must still not instantiate anything
# containing OpenMP directives, because nvcc's host pass rewrites some of them; the
# device path is synchronized in the headers (see SU2_DEVICE_REGION in CSysVector.hpp),
# which only the .cpp sources instantiate.

subdir('linear_algebra')
subdir('toolboxes')
subdir('geometry')
subdir('geometry/elements')
subdir('geometry/dual_grid')
subdir('geometry/primal_grid')
subdir('geometry/meshreader')
subdir('containers')
subdir('interface_interpolation')
subdir('fem')
subdir('grid_movement')
subdir('adt')
subdir('adaptation')

# Strict passive arithmetic: compensated transfer sums and filtered native orientation must survive the compiler.
# Compile without fast math or contraction (options appended after the SU2 flags win).
# Pure doubles: the same library for every build type; retain its existing name.
accurate_sum_lib = static_library('SU2AccurateSum',
                                  files(['adaptation/CAccurateSum.cpp', 'adaptation/CNativePredicates2D.cpp',
                                         'adaptation/CNativePredicates3D.cpp', 'adaptation/CNativeTopology3D.cpp',
                                         'adaptation/CNativeCavity3D.cpp']),
                                  install : false,
                                  cpp_args: [default_warning_flags, su2_cpp_args,
                                             '-fno-fast-math', '-fno-unsafe-math-optimizations',
                                             '-ffp-contract=off'])

if get_option('enable-normal')
  common = static_library('SU2Common',
                          common_src, common_cuda_src,
                          install : false,
                          dependencies : su2_deps,
                          cpp_args: [default_warning_flags, su2_cpp_args],
                          cuda_args: common_cuda_cpp_args)

  common_dep = declare_dependency(link_with: [common, accurate_sum_lib],
                                  include_directories : common_include)
endif

if get_option('enable-autodiff')

  # codi_rev_args is only defined for AD builds, and it is all defines, which nvcc accepts.
  common_cuda_rev_args = common_cuda_cpp_args + codi_rev_args

  commonAD = static_library('SU2CommonAD',
                            common_src, common_cuda_src,
                            install : false,
                            dependencies : [su2_deps, codi_dep],
                            cpp_args: [default_warning_flags, su2_cpp_args, codi_rev_args],
                            cuda_args: common_cuda_rev_args)

  commonAD_dep = declare_dependency(link_with: [commonAD, accurate_sum_lib],
                                    include_directories : common_include)

endif

if get_option('enable-directdiff')

  commonDD = static_library('SU2CommonDD',
                            common_src,
                            install : false,
                            dependencies : [su2_deps, codi_dep],
                            cpp_args: [default_warning_flags, su2_cpp_args, codi_for_args])

  commonDD_dep = declare_dependency(link_with: [commonDD, accurate_sum_lib],
                                    include_directories : common_include)

endif
