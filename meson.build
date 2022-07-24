project('rcx', 'cpp',
  version : '0.1',
  default_options : ['warning_level=3', 'cpp_std=c++17'])

boost_dep = dependency('boost', modules : [ 'program_options' ])

cmake = import('cmake')

# spdlog is cmake project
spdlog_sproj = cmake.subproject('spdlog')
spdlog_dep = spdlog_sproj.dependency('spdlog')

llvm_dep = dependency(
  'llvm',
  version : '>= 12.0'
  )

rcx_inc_dir = include_directories('src')

executable('rcx',
           'src/rcx.cxx',
           'src/1:Parse/parser.cxx',
           install : true,
           include_directories : rcx_inc_dir,
           dependencies : [ spdlog_dep, llvm_dep, boost_dep ])
