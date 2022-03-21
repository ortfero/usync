project('usync', 'cpp',
    license: 'MIT',
    version: '0.1.1',
    default_options: [
        'cpp_std=c++17',
        'warning_level=3'])

headers = [
    'include/usync/usync.hpp'
]

incdirs = include_directories('./include')

usync = declare_dependency(
    version: meson.project_version(),
    include_directories: incdirs,
    sources: headers
)

subdir('test')

install_headers(headers, subdir: 'usync')

pkg = import('pkgconfig')
pkg.generate(
    name: 'usync',
    filebase: 'usync',
    subdirs: ['usync'],
    version: meson.project_version(),
    description: 'C++17 header-only library for spinlocks'
)
