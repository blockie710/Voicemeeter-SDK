project('voicemeeter-sdk', ['c', 'cpp'],
  version : '1.0.0',
  default_options : ['warning_level=3', 'cpp_std=c++17', 'c_std=c11'])

# Check for platform
is_windows = host_machine.system() == 'windows'
is_linux = host_machine.system() == 'linux'
is_macos = host_machine.system() == 'darwin'

# Compiler setup
cc = meson.get_compiler('c')
cpp = meson.get_compiler('cpp')

# Global dependencies
deps = []

# Platform-specific dependencies and compiler flags
if is_windows
  deps += [cc.find_library('winmm'), cc.find_library('user32'), cc.find_library('gdi32')]
  add_project_arguments('-D_WIN32', language : ['c', 'cpp'])
elif is_linux
  deps += [dependency('alsa', required : false), dependency('threads')]
  add_project_arguments('-D__linux__', language : ['c', 'cpp'])
elif is_macos
  add_project_arguments('-D__APPLE__', language : ['c', 'cpp'])
endif

# Check for optional components to build
build_plugin_host = get_option('build_plugin_host')
build_example0 = get_option('build_example0')
build_matrix8x8 = get_option('build_matrix8x8')
build_vmr_osd = get_option('build_vmr_osd')
build_vmr_play = get_option('build_vmr_play')
build_vmr_streamer = get_option('build_vmr_streamer')

# Optionally include subprojects
if build_plugin_host
  subdir('PluginHost')
endif

if build_example0
  subdir('example0')
endif

if build_matrix8x8
  subdir('matrix8x8')
endif

if build_vmr_osd
  subdir('vmr_osd')
endif

if build_vmr_play
  subdir('vmr_play')
endif

if build_vmr_streamer
  subdir('vmr_streamer')
endif

# Installation of common files
install_data('README.md', 'LICENSE', 'VoicemeeterRemote.h', 'VoicemeeterRemoteAPI.pdf',
  install_dir : get_option('datadir') / 'voicemeeter-sdk')