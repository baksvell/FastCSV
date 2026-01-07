#pragma once

#include <cstddef>
#include <string_view>
#include <vector>

#ifdef __AVX2__
#include <immintrin.h>
#endif

#ifdef __SSE4_2__
#include <nmmintrin.h>
#endif

namespace fastcsv {
namespace simd {

// Проверка доступности SIMD инструкций
inline bool has_avx2() {
#ifdef __AVX2__
    return true;
#else
    return false;
#endif
}

inline bool has_sse42() {
#ifdef __SSE4_2__
    return true;
#else
    return false;
#endif
}

// Поиск символа с использованием SIMD
// Возвращает позицию первого вхождения или string_view::npos
std::size_t find_char_simd(std::string_view data, char target, std::size_t start_pos = 0);

// Поиск любого из символов (delimiter или quote)
std::size_t find_any_char_simd(std::string_view data, char delim, char quote, std::size_t start_pos = 0);

// Подсчет количества символов в блоке
std::size_t count_chars_simd(std::string_view data, char target);

// Находит все вхождения символа и возвращает их позиции
// Используется для быстрого парсинга CSV без кавычек
void find_all_chars_simd(std::string_view data, char target, std::vector<std::size_t>& positions, std::size_t start_pos = 0);

// Проверяет, является ли строка ASCII (все байты < 128)
// Используется для оптимизации создания Python строк
bool is_ascii_simd(std::string_view data);

} // namespace simd
} // namespace fastcsv


