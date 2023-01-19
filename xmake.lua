set_project('krill')
set_languages('cxx17')
set_warnings('allextra') -- -Wall
add_rules("mode.debug", "mode.release")

  add_cxflags('-Wno-sign-compare')
includes(
  'krill', 'test'
)

