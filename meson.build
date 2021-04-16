project(
  'RNMC',
  'c',
  version: '1.0',
  license: 'BSD3-Clause-LBNL',
)


cc = meson.get_compiler('c')

add_project_arguments('-Wno-unused-variable', language: 'c')

if cc.get_id() == 'gcc'
  add_project_arguments('-Wno-unused-but-set-variable', language: 'c')
endif

gsl_dep = cc.find_library('gsl')
gslcblas_dep = cc.find_library('gslcblas')
sqlite = cc.find_library('sqlite3')
thread_dep = dependency('threads')
# linux has a standalone math header so we need to have it as an explicit dependency
math_dep = cc.find_library('m', required : false)

common_files = [
  'src/reaction_network.c',
  'src/sampler.c',
  'src/simulation.c',
  'src/solvers.c',
  'src/dispatcher.c',
  'src/sql_programs.c'
]

RNMC_include = include_directories('src')

executable(
  'RNMC',
  files(['src/RNMC.c'] + common_files),
  dependencies: [gsl_dep, gslcblas_dep, math_dep, thread_dep, sqlite],
  include_directories: [RNMC_include],
  install: true
)

tests = executable(
  'RNMC_test',
  files(['src/test.c'] + common_files),
  dependencies: [gsl_dep, gslcblas_dep, math_dep, thread_dep, sqlite],
  include_directories: [RNMC_include],
  install: false
)

test('tests', tests, args: ['../test_materials'])
