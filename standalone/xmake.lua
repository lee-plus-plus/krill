target('calc')
  set_kind('binary')
  add_files('calc.cpp')
  add_deps('krill')

target('kriller')
  set_kind('binary')
  add_files('calc.cpp')
  add_deps('krill')

target('mico')
  set_kind('binary')
  add_files('calc.cpp')
  add_deps('krill')
