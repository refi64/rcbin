from fbuild.builders.platform import guess_platform
from fbuild.builders.c import guess

from fbuild.config.c import cacheproperty, header_test, Test

from fbuild.record import Record
import fbuild.db

from optparse import make_option
import ast


def arguments(parser):
    group = parser.add_argument_group('config options')
    group.add_argument('--cc', help='Use the given C compiler'),
    group.add_argument('--cflag', help='Pass the given flag to the C compiler',
                       action='append', default=[]),
    group.add_argument('--use-color', help='Force C++ compiler colored output',
                       action='store_true', default=True),
    group.add_argument('--release', help='Build in release mode',
                       action='store_true', default=False),
    group.add_argument('--platform', help='Use the given target platform for building'),


class libelf(Test):
    libelf_h = header_test('libelf.h')
    gelf_h = header_test('gelf.h')


@fbuild.db.caches
def configure(ctx):
    if ctx.options.platform:
        platform = ast.literal_eval(ctx.options.platform)
    else:
        platform = guess_platform(ctx)

    flags = ctx.options.cflag
    posix_flags = ['-Wall', '-Werror']
    clang_flags = []

    if ctx.options.use_color:
        posix_flags.append('-fdiagnostics-color')
    if ctx.options.release:
        debug = False
        optimize = True
    else:
        debug = True
        optimize = False
        clang_flags.append('-fno-limit-debug-info')

    c = guess.static(ctx, platform=platform, exe=ctx.options.cc, flags=flags,
                     debug=debug, optimize=optimize, platform_options=[
                        ({'clang'}, {'flags+': clang_flags}),
                        ({'posix'}, {'flags+': posix_flags,
                                     'external_libs+': ['m'],
                                     'cross_compiler': True}),
                    ])

    external_libs = []

    if platform & {'posix'} and not platform & {'mingw'}:
        libelf_test = libelf(c)
        if not libelf_test.libelf_h or not libelf_test.gelf_h:
            raise fbuild.ConfigFailed('libelf is required')
        external_libs.append('elf')

    return Record(c=c, external_libs=external_libs)


def build(ctx):
    rec = configure(ctx)
    librcbin = rec.c.build_lib('rcbin', ['src/librcbin.c'], includes=['include'])
    rcbin = rec.c.build_exe('rcbin', ['src/rcbin.c'], includes=['include'],
                            external_libs=rec.external_libs)
    rec.c.build_exe('tst', ['tst.c'], includes=['include'], libs=[librcbin])

    ctx.install('include/rcbin.h', 'include')
    ctx.install(librcbin, 'lib')
    ctx.install(rcbin, 'bin')
