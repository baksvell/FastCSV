import os
import re
import sys
import subprocess
from pathlib import Path
from pybind11.setup_helpers import Pybind11Extension, build_ext
from pybind11 import get_cmake_dir
from setuptools import setup, Extension
import pybind11

# Получаем путь к pybind11
pybind11_path = pybind11.get_include()

# Настройки компиляции
if sys.platform == 'win32':
    compile_args = ['/std:c++17', '/O2', '/DNDEBUG', '/D__AVX2__', '/D__SSE4_2__']
    link_args = []
elif sys.platform == 'darwin':
    # macOS: проверяем архитектуру
    import platform
    machine = platform.machine()
    if machine == 'arm64':
        # Apple Silicon (M1, M2, M3, etc.) - не поддерживает x86 SIMD инструкции
        compile_args = ['-std=c++17', '-O3']
    else:
        # Intel Mac - поддерживает AVX2/SSE4.2
        compile_args = ['-std=c++17', '-O3', '-mavx2', '-msse4.2']
    link_args = []
else:
    # Linux и другие Unix-системы
    compile_args = ['-std=c++17', '-O3', '-march=native', '-mavx2', '-msse4.2']
    link_args = []

# Список исходных файлов
sources = [
    'src/csv_parser.cpp',
    'src/simd_utils.cpp',
    'src/python_bindings.cpp',
]

ext_modules = [
    Pybind11Extension(
        'fastcsv._native',
        sources,
        include_dirs=[
            pybind11_path,
            'include',
        ],
        cxx_std=17,
        language='c++',
        extra_compile_args=compile_args,
        extra_link_args=link_args,
    ),
]

setup(
    ext_modules=ext_modules,
    cmdclass={"build_ext": build_ext},
    zip_safe=False,
)

