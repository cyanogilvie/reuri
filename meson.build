project('reuri', 'c',
  version:          '0.14.4',
  default_options:  ['c_std=c17', 'warning_level=3', 'werror=true'],
  meson_version:    '>=1.3.0',
)

cc = meson.get_compiler('c')
fs = import('fs')
add_project_arguments('-D_DEFAULT_SOURCE', language: 'c')

if get_option('debug')
  add_project_arguments(cc.get_supported_arguments(['-ggdb3']), language: 'c')
endif

add_project_arguments('-Wno-missing-field-initializers', language: 'c')

# GCC 15.2 broke union initialization with designated initializers using = {0}, this restores the (I argue standards-mandated) older behaviour
#add_project_arguments(cc.get_supported_arguments(['-fzero-init-padding-bits=unions']), language: 'c')

conf = configuration_data()
conf.set10('UNLOAD',   get_option('unload'))
conf.set10('TESTMODE', get_option('testmode'))
conf.set10('DEBUG',    get_option('debugmode'))

tea_want_shims = ['tip445']

add_project_arguments('-DHAVE_CONFIG_H', language: 'c')

deps = []
inc = [include_directories('generic')]

subdir('teabase/init')

# re2c lexer generation <<<
re2c_proj = subproject('re2c')
re2c_prog = re2c_proj.get_variable('re2c_exe')

re2c_args = [re2c_prog, '--case-ranges', '-W', '-Wno-nondeterministic-tags', '-Wno-match-empty-string', '--conditions', '-o', '@OUTPUT@', '@INPUT@']

lexer_parse = custom_target('lexer_parse',
  input:   'generic/parse.re',
  output:  'parse.c',
  depend_files: files('generic/parse.h'),
  command: re2c_args,
)

lexer_index = custom_target('lexer_index',
  input:   'generic/index.re',
  output:  'index.c',
  depend_files: files('generic/parse.h'),
  command: re2c_args,
)
# re2c lexer generation >>>

# Stubs generation
deps += declare_dependency(
  sources: custom_target('genstubs',
    output: ['reuriStubInit.c', 'reuriDecls.h'],
    command: [
      build_tclsh,
      meson.project_source_root() / 'teabase/tools/genStubs.tcl',
      meson.project_build_root(),
      meson.project_source_root() / 'generic/reuri.decls',
    ],
    install: true,
    install_dir: [false, pkg_dir],
  ),
)

# Dedup dependency (stubs)
dedup_dep = dependency('dedup')
deps += dedup_dep

pkg_sources = files(
  'generic/reuri.c',
  'generic/type_uri.c',
  'generic/type_uri_part.c',
  'generic/type_index.c',
)
pkg_sources += [lexer_parse, lexer_index]

if get_option('testmode')
  pkg_sources += files('generic/testmode.c')
endif

# Package config values compiled into binary
conf.set_quoted('REURI_LIBRARY_PATH_INSTALL', pkg_dir)
conf.set_quoted('REURI_INCLUDE_PATH_INSTALL', pkg_dir)
conf.set_quoted('REURI_PACKAGE_PATH_INSTALL', pkg_dir)
conf.set_quoted('REURI_LIBRARY', meson.project_name())
conf.set_quoted('REURI_STUBLIB', meson.project_name() + 'stub')
conf.set_quoted('REURI_HEADER', 'reuri.h')
conf.set_quoted('REURI_INCLUDE_SPEC', '-I' + pkg_dir)
conf.set_quoted('REURI_BUILD_INCLUDE_SPEC', '-I' + meson.project_build_root())
conf.set_quoted('REURI_LIB_SPEC', '-L' + pkg_dir)
conf.set_quoted('REURI_BUILD_LIB_SPEC', '-L' + meson.project_build_root() / 'teabase')
conf.set_quoted('REURI_STUB_LIB_SPEC', '-L' + pkg_dir)
conf.set_quoted('REURI_BUILD_STUB_LIB_SPEC', '-L' + meson.project_build_root() / 'teabase')

subdir('teabase')

# Public header
configure_file(
  configuration:  teaconf,
  input:          'generic/reuri.h.in',
  output:         '@BASENAME@',
  install_dir:    pkg_dir,
)

# Build and install stubs library for dependent extensions
stubs_lib = static_library(meson.project_name() + 'stub',
  'generic/reuriStubLib.c',
  dependencies: deps,
  include_directories: inc,
  install: true,
  install_dir: pkg_dir,
)

# Generate relocatable pkg-config file
pc_conf = configuration_data()
pc_conf.set('PACKAGE_NAME', meson.project_name())
pc_conf.set('PACKAGE_VERSION', meson.project_version())
pc_conf.set('PKG_DIR_BASENAME', meson.project_name() + '-' + meson.project_version())
pc_conf.set('STUBLIB', meson.project_name() + 'stub')

configure_file(
  input:          'reuri.pc.in',
  output:         meson.project_name() + '.pc',
  configuration:  pc_conf,
  install_dir:    tcl_dep.get_variable('libdir') / 'pkgconfig',
)

# Test config
test_conf = configuration_data()
test_conf.set('REURI_LIBRARY', meson.project_name())
test_conf.set('REURI_STUBLIB', meson.project_name() + 'stub')

configure_file(
  input:          'tests/test_config.tcl.in',
  output:         'test_config.tcl',
  configuration:  test_conf,
)

subdir('doc')

# vim: foldmethod=marker foldmarker=<<<,>>> ts=2 shiftwidth=2 expandtab
