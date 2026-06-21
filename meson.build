project('MVTools', 'cpp',
    default_options: [
        'buildtype=release', 'warning_level=2', 'b_lto=true', 'b_ndebug=if-release', 'cpp_std=c++17',
    ],
    license: 'GPL-2.0-or-later',
    license_files: 'LICENSE',
    meson_version: '>=1.2.3',
    version: '29',
)

py = import('python').find_installation(pure: false)

incdir = include_directories(
    run_command(py, '-c', 'import vapoursynth as vs; print(vs.get_include())', check: true).stdout().strip(),
)

libs = []

add_project_arguments('-DPACKAGE_VERSION="@0@"'.format(meson.project_version()), language: ['cpp'])

if host_machine.cpu_family() == 'x86_64'
    add_languages('nasm', native: false)

    add_project_arguments('-DMVTOOLS_X86', language: ['cpp'])

    add_project_arguments(
        '-DARCH_X86_64=1',
        '-DBIT_DEPTH=8',
        '-DHIGH_BIT_DEPTH=0',
        '-DPIC',
        '-Dprivate_prefix=mvtools',
        language: 'nasm',
    )

    if host_machine.system() == 'darwin'
        add_project_arguments('-DPREFIX', language: ['cpp', 'nasm'])
    endif

    libs += static_library('avx2',
        files(
            'src/MVDegrains_AVX2.cpp',
            'src/MVFrame_AVX2.cpp',
            'src/Overlap_AVX2.cpp',
            'src/SADFunctions_AVX2.cpp',
            'src/SimpleResize_AVX2.cpp',
        ),
        cpp_args: '-march=x86-64-v3',
        gnu_symbol_visibility: 'hidden',
        include_directories: incdir,
    )

    libs += static_library('asm',
        files(
            'src/asm/const-a.asm',
            'src/asm/cpu-a.asm',
            'src/asm/pixel-a.asm',
            'src/asm/sad-a.asm',
        ),
        gnu_symbol_visibility: 'hidden',
        include_directories: include_directories('src/asm/include'),
    )
elif host_machine.cpu_family() == 'aarch64'
    add_project_arguments('-DMVTOOLS_ARM', language: ['cpp'])
endif

shared_module('mvtools',
    files(
        'src/CopyCode.cpp',
        'src/CPU.cpp',
        'src/DCTFFTW.cpp',
        'src/EntryPoint.cpp',
        'src/Fakery.cpp',
        'src/GroupOfPlanes.cpp',
        'src/Luma.cpp',
        'src/MaskFun.cpp',
        'src/MVAnalyse.cpp',
        'src/MVAnalysisData.cpp',
        'src/MVBlockFPS.cpp',
        'src/MVCompensate.cpp',
        'src/MVDegrains.cpp',
        'src/MVDepan.cpp',
        'src/MVFinest.cpp',
        'src/MVFlow.cpp',
        'src/MVFlowBlur.cpp',
        'src/MVFlowFPS.cpp',
        'src/MVFlowFPSHelper.cpp',
        'src/MVFlowInter.cpp',
        'src/MVFrame.cpp',
        'src/MVMask.cpp',
        'src/MVRecalculate.cpp',
        'src/MVSCDetection.cpp',
        'src/MVSuper.cpp',
        'src/Overlap.cpp',
        'src/PlaneOfBlocks.cpp',
        'src/SADFunctions.cpp',
        'src/SimpleResize.cpp',
    ),
    dependencies: dependency('fftw3f'),
    gnu_symbol_visibility: 'hidden',
    include_directories: incdir,
    install: true,
    install_dir: py.get_install_dir() / 'vapoursynth/plugins',
    link_with: libs,
    name_prefix: '',
)
