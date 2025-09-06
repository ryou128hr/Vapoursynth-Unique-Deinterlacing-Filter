project('hybriddeinterlace', 'cpp',
  version: '1.0',
  default_options: ['warning_level=2', 'buildtype=release', 'cpp_std=c++17'])

vs_sdk_dir = 'C:/Users/ryou/AppData/Local/Programs/VapourSynth/sdk'


vs_inc = include_directories(vs_sdk_dir + '/include/vapoursynth')


vapoursynth_dep = declare_dependency(
  include_directories: vs_inc,
  
  link_args: [vs_sdk_dir + '/lib64/vapoursynth.lib'] 
)

shared_module('hybrid_deinterlacer',
  'hybrid_deinterlacer.cpp', 
  dependencies: [vapoursynth_dep],
  install: true,
  name_prefix: '',
  name_suffix: 'dll'

)
