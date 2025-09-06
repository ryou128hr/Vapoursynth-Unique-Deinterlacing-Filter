project('hybriddeinterlace', 'cpp',
  version: '1.0',
  default_options: ['warning_level=2', 'buildtype=release', 'cpp_std=c++17'])

vs_sdk_dir = 'C:/Users/ryou/AppData/Local/Programs/VapourSynth/sdk'

# あなたの環境で VapourSynth.h が見つかる正しいパス
vs_inc = include_directories(vs_sdk_dir + '/include/vapoursynth')

# VapourSynthライブラリへの依存関係 (より堅牢な書き方)
vapoursynth_dep = declare_dependency(
  include_directories: vs_inc,
  # 64bit環境を想定。もし32bitなら lib/vapoursynth.lib に変更してください。
  link_args: [vs_sdk_dir + '/lib64/vapoursynth.lib'] 
)

shared_module('hybrid_deinterlacer',
  'hybrid_deinterlacer.cpp', # C++ファイル名をこちらに合わせる
  dependencies: [vapoursynth_dep],
  install: true,
  name_prefix: '',
  name_suffix: 'dll'
)