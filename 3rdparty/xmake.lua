target('cxxopts')
  set_kind('headeronly')
  add_includedirs('cxxopts/include', {public = true})
  add_headerfiles('cxxopts/include/cxxopts.hpp')
