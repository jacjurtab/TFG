project('dnp3', 'c',
    version: '0.8.10',
    license: 'GPLv2+',
    default_options: [
        'warning_level=2',
        'buildtype=release',
        'c_std=gnu99'
    ],
    #meson_version: '>= 0.45.0',
)

plugin_name = meson.project_name()
#plugin_number = '999'

cc = meson.get_compiler('c')
os = host_machine.system()
#perl = find_program('perl')

if os == 'darwin'
    add_project_arguments('-D_DARWIN_C_SOURCE', language: 'c')
elif os == 'linux'
    add_project_arguments('-D_GNU_SOURCE', language: 'c')
#elif os == 'windows'
#    message('You are using Windows...')
#else
#    warning('OS not recognized...')
endif

libm = cc.find_library('m')
threads_dep = dependency('threads')
zlib_dep = dependency('zlib', version: '>=1.2.8', required: false)
if zlib_dep.found()
    message('ZLIB >= 1.2.8 found')
else
    warning('ZLIB >= 1.2.8 not found')
endif

deps = [
    libm,
    threads_dep,
]

inc = include_directories(
    join_paths('..', '..', 'utils'),
    join_paths('..', '..', 'tranalyzer2', 'src'),
    #join_paths('..', 'tcpFlags', 'src'),   # tell the compiler where to find header
                                            # files from dependent plugins
)

#subdir('doc')

src = [
    join_paths('src', plugin_name + '.c'),
    #join_paths('..', '..', 'utils', 't2buf.c'),
]

#cmd = run_command(perl, '-nle', 'print $1 if /^#define\s+DNP3_VAR1\s+(\d+).*$/', 'src/' + plugin_name + '.h')
#if cmd.returncode() != 0
#    error('Could not determine value of \'DNP3_VAR1\' in \'src/' + plugin_name + '.h\'')
#endif

shared_module(plugin_name,
    sources: src,
    dependencies: deps,
    include_directories: inc,
    #name_prefix: plugin_number + '_',
    name_suffix: 'so',
)
