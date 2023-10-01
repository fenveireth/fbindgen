project('fbindgen', ['cpp'], version:'0.0.1', default_options:['cpp_std=c++20', 'cpp_eh=none'])

cxx = meson.get_compiler('cpp')

runllvmlib = run_command('llvm-config', '--libdir', check:true)
runllvminc = run_command('llvm-config', '--includedir', check:true)

libdirs = [runllvmlib.stdout().strip()]

deps = [cxx.find_library('clang-cpp', dirs:libdirs), cxx.find_library('LLVM', dirs:libdirs)]
incs = [runllvminc.stdout().strip()]
src = [
  'src/clang.cpp',
  'src/detect.cpp',
  'src/function.cpp',
  'src/macro.cpp',
  'src/main.cpp',
  'src/stdafx.cpp',
  'src/types.cpp',
]
executable('fbindgen', src, cpp_pch:'src/stdafx.h', dependencies:deps, include_directories:incs)

test_detect = executable('test_detect', ['tests/detect.cpp', 'src/detect.cpp'], cpp_pch:'src/stdafx.h', dependencies:deps, include_directories:incs, build_by_default: false)
test('clang detection', test_detect, workdir: meson.source_root() / 'tests/')
test('all tests', find_program('tests/test.sh'), workdir: meson.source_root() / 'tests/')
