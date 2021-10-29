project('fgen', ['cpp'], version:'0.0.1', default_options:['cpp_std=c++17', 'cpp_eh=none'])

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
executable('fgen', src, cpp_pch:'src/stdafx.h', dependencies:deps, include_directories:incs)
