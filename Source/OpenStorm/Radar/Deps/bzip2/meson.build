project('bzip2', 'c', version : '1.0.6', meson_version : '>=0.49')

bzip2_sources = [
  'blocksort.c',
  'huffman.c',
  'crctable.c',
  'randtable.c',
  'compress.c',
  'decompress.c',
  'bzlib.c'
]

bz2_c_args = []

cc = meson.get_compiler('c')
if cc.get_id() == 'msvc'
  # Ignore C4996 about "insecure functions"
  bz2_c_args += ['/wd4996']
else
  # bzip2.c:557: warning: variable nread set but not used
  bz2_c_args += ['-Wno-unused-but-set-variable']
endif

bz2_incs = include_directories('.')

install_headers(['bzlib.h'])

# libbz2 is unversioned on Windows
libkwargs = {}
if host_machine.system() != 'windows'
  libkwargs = {'version': meson.project_version()}
endif

libbz2 = library('bz2', bzip2_sources,
  c_args : bz2_c_args,
  include_directories : bz2_incs,
  install : true,
  vs_module_defs : 'libbz2.def',
  darwin_versions : ['1', meson.project_version()],
  kwargs : libkwargs,
)

bz2_dep = declare_dependency(link_with : libbz2,
                             include_directories : bz2_incs)
pkg = import('pkgconfig')
pkg.generate(name: 'bzip2',
             description: 'bzip2 data compression library',
             version: meson.project_version(),
             libraries: libbz2)

executable('bzip2', 'bzip2.c',
  install : true,
  dependencies : bz2_dep,
  c_args: bz2_c_args)

executable('bunzip2', 'bzip2.c',
  install : true,
  dependencies : bz2_dep,
  c_args: bz2_c_args)

executable('bzcat', 'bzip2.c',
  install : true,
  dependencies : bz2_dep,
  c_args: bz2_c_args)

executable('bzip2recover', 'bzip2recover.c',
  install : true,
  c_args: bz2_c_args)

install_man('bzdiff.1', 'bzgrep.1', 'bzip2.1', 'bzmore.1')
install_data('bzgrep', 'bzmore', 'bzdiff', install_dir : 'bin',
             install_mode : 'rwxr-xr-x')

# FIXME: build docs
