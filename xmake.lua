set_project('krill')
set_languages('cxx20')
set_warnings('allextra') -- -Wall
add_rules("mode.debug", "mode.release")

includes(
  'krill', 'test', 'standalone'
)

