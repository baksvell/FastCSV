cmake_minimum_required(VERSION 3.15)
project(FastCSV VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Оптимизации
if(MSVC)
    add_compile_options(/O2 /DNDEBUG)
    # SIMD для MSVC
    add_compile_definitions(__AVX2__ __SSE4_2__)
else()
    add_compile_options(-O3 -march=native)
    # SIMD поддержка
    add_compile_options(-mavx2 -msse4.2)
endif()

# Поиск pybind11
find_package(pybind11 REQUIRED)

# Исходные файлы
set(SOURCES
    src/csv_parser.cpp
    src/simd_utils.cpp
    src/python_bindings.cpp
)

# Заголовочные файлы
set(HEADERS
    include/fastcsv/csv_parser.hpp
    include/fastcsv/simd_utils.hpp
)

# Создание библиотеки
pybind11_add_module(_native ${SOURCES} ${HEADERS})

target_include_directories(_native PRIVATE include)

# Установка
install(TARGETS _native DESTINATION .)

