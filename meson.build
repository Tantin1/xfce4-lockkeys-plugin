project(
  'xfce4-lockkeys-plugin',
  'c',
  version: '1.0.0',
  license: 'LGPL-2.1-or-later',
  meson_version: '>= 0.54.0',
  default_options: ['c_std=c11', 'warning_level=2'],
)

# ── Dependencias ──────────────────────────────────────────────────────────────

libxfce4panel = dependency('libxfce4panel-2.0', version: '>= 4.12.0')
libxfce4util  = dependency('libxfce4util-1.0',  version: '>= 4.12.0')
gtk           = dependency('gtk+-3.0',           version: '>= 3.20.0')
x11           = dependency('x11')

cc = meson.get_compiler('c')
cc.check_header('X11/XKBlib.h', required: true)

# ── Directorio del plugin ─────────────────────────────────────────────────────

plugins_dir = libxfce4panel.get_variable(pkgconfig: 'pluginsdir',
                                          default_value: get_option('libdir') / 'xfce4/panel/plugins')

# ── i18n ──────────────────────────────────────────────────────────────────────

i18n = import('i18n')
gettext_package = meson.project_name()

add_project_arguments(
  '-DGETTEXT_PACKAGE="@0@"'.format(gettext_package),
  '-DPACKAGE_LOCALE_DIR="@0@"'.format(get_option('prefix') / get_option('datadir') / 'locale'),
  '-DHAVE_CONFIG_H',
  language: 'c',
)

# ── Plugin ────────────────────────────────────────────────────────────────────

shared_module(
  'lockkeys',
  'panel-plugin/lockkeys.c',
  dependencies: [libxfce4panel, libxfce4util, gtk, x11],
  install: true,
  install_dir: plugins_dir,
  name_prefix: 'lib',
)

# ── Desktop file ──────────────────────────────────────────────────────────────

install_data(
  'panel-plugin/lockkeys.desktop.in',
  install_dir: get_option('datadir') / 'xfce4/panel/plugins',
  rename: 'lockkeys.desktop',
)
