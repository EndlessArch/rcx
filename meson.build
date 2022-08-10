project('rcx', 'cpp',
  version : '0.1',
  default_options : [
    'warning_level=3',
    'cpp_std=c++17',
    'cpp_eh=none',
    'cpp_rtti=false'
    ])

add_global_arguments(
  [ '-Wno-unused-parameter'
  # , '-Wno-return-type'
  ], language : [ 'c', 'cpp' ])

boost_dep = dependency('boost', modules : [ 'program_options' ])

cmake = import('cmake')

# spdlog is cmake project
spdlog_sproj = cmake.subproject('spdlog')
spdlog_dep = spdlog_sproj.dependency('spdlog')

llvm_dep = dependency(
  'llvm',
  version : '>= 13.0'
  # modules : [ 'core', 'native', 'bitwriter' ]
  )

clang_dep = dependency(
  'clang',
  static : true,
  modules : [ 'clangBasic', 'clangFrontend', 'clangCodeGen' ])

rcx_inc_dir = include_directories('src')

sources = [
  'src/rcx.cxx',
  'src/parse/parser.cxx'
  ]

executable('rcx',
           sources,
           install : true,
           include_directories : rcx_inc_dir,
           dependencies : [ spdlog_dep, llvm_dep, clang_dep, boost_dep ])