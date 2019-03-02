project(
	'wf-simple-osk',
    'c',
	'cpp',
	version: '0.1',
	license: 'MIT',
	meson_version: '>=0.43.0',
	default_options: [
		'cpp_std=c++14',
        'c_std=c11',
		'warning_level=2',
		'werror=false',
	],
)

gtkmm = dependency('gtkmm-3.0')
wayland_client = dependency('wayland-client')
wayland_protos = dependency('wayland-protocols')

add_project_link_arguments(['-rdynamic'], language:'cpp')
add_project_arguments(['-Wno-unused-parameter'], language: 'cpp')

subdir('proto')
subdir('src')
