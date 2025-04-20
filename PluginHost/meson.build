# PluginHost Meson build configuration

# Find dependencies
json_dep = dependency('nlohmann_json', required : false)
if not json_dep.found()
  json_dep = declare_dependency() # Empty dependency if not found
endif

# Additional dependencies based on platform
plugin_host_deps = [json_dep]
if is_linux
  plugin_host_deps += [dependency('gtk+-3.0', required : false)]
endif

# PluginHost source files
plugin_host_sources = [
  'src/main.cpp',
  'src/plugin_interface.cpp',
  'src/plugin_manager.cpp',
  'src/plugin_state_manager.cpp',
  'src/voicemeeter_integration.cpp',
  'src/audio_processor.cpp',
  'src/vst3_plugin.cpp',
  'src/aax_plugin.cpp',
  'src/ara_plugin.cpp'
]

# Include files
plugin_host_includes = include_directories('include')

# Windows-specific resource files
if is_windows
  windows = import('windows')
  plugin_host_sources += windows.compile_resources('src/resources.rc')
endif

# Define the PluginHost executable
plugin_host_exe = executable('VoicemeeterPluginHost',
  plugin_host_sources,
  include_directories : plugin_host_includes,
  dependencies : plugin_host_deps + deps,
  install : true
)

# Custom target to generate documentation
doxygen = find_program('doxygen', required : false)
if doxygen.found()
  custom_target('docs',
    output : 'html',
    command : [doxygen, meson.current_source_dir() / 'Doxyfile'],
    depends : plugin_host_exe
  )
endif

# Install additional plugin host files
install_data('README.md', install_dir : get_option('datadir') / 'voicemeeter-plugin-host')

# Create subdirectory for plugins
install_subdir('plugins', install_dir : get_option('datadir') / 'voicemeeter-plugin-host')