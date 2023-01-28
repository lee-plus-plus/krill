add_requires('spdlog', {configs={fmt_external = true}})
add_requires('magic_enum')

target('krill')
  set_kind('static')
  add_includedirs('include', {public = true})
  add_files('src/**.cpp')
  add_deps('cxxopts', {public = true})
  add_packages('spdlog', 'magic_enum', {public = true})
