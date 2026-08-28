project('sandy', 'cpp',
  version : '0.1.0',
  default_options : ['warning_level=3', 'cpp_std=c++17']
)

inc = include_directories('include')

libcap_dep = dependency('libcap', required : true)
toml_parse = dependency('tomlplusplus', required : true)

sources = files(
  'src/main.cpp',
  'src/which_clone.cpp',
  'src/args_read_from_file.cpp'
)

executable('sandbox_trial',
  sources,
  include_directories : inc, # Passes -Iinclude to g++
  dependencies : [libcap_dep, toml_parse],
  install : true
)

