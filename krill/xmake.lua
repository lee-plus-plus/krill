add_requires('spdlog', {configs={fmt_external = true}})
add_requires('magic_enum')
add_requires('cxxopts')

target('krill')
  set_kind('static')
  add_includedirs('include', {public = true})
  add_files('src/**.cpp')
  add_packages('spdlog', 'magic_enum', 'cxxopts', {public = true})
