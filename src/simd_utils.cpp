#include "fastcsv/simd_utils.hpp"
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <vector>

#ifdef _WIN32
#include <intrin.h>
#endif

namespace fastcsv {
namespace simd {

// Кроссплатформенная функция для поиска первого установленного бита
inline int find_first_set_bit(int mask) {
    if (mask == 0) return -1;
    
#ifdef _WIN32
    unsigned long index;
    _BitScanForward(&index, mask);
    return static_cast<int>(index);
#else
    return __builtin_ctz(mask);
#endif
}

// Кроссплатформенная функция для подсчета установленных битов
inline int count_bits(int mask) {
#ifdef _WIN32
    return __popcnt(mask);
#else
    return __builtin_popcount(mask);
#endif
}

std::size_t find_char_simd(std::string_view data, char target, std::size_t start_pos) {
    if (start_pos >= data.length()) {
        return std::string_view::npos;
    }
    
    const char* ptr = data.data() + start_pos;
    std::size_t len = data.length() - start_pos;
    
#ifdef __AVX2__
    if (has_avx2() && len >= 32) {
        __m256i target_vec = _mm256_set1_epi8(target);
        const char* end = ptr + len;
        const char* aligned_start = ptr + ((32 - (reinterpret_cast<std::uintptr_t>(ptr) & 31)) & 31);
        const char* aligned_end = ptr + (len & ~31);
        
        // Обработка невыровненных байтов в начале
        while (ptr < aligned_start && ptr < end) {
            if (*ptr == target) {
                return ptr - data.data();
            }
            ptr++;
        }
        
        // AVX2 обработка выровненных блоков
        while (ptr < aligned_end) {
            __m256i data_vec = _mm256_load_si256(reinterpret_cast<const __m256i*>(ptr));
            __m256i cmp = _mm256_cmpeq_epi8(data_vec, target_vec);
            int mask = _mm256_movemask_epi8(cmp);
            
            if (mask != 0) {
                // Найден символ
                int index = find_first_set_bit(mask);
                return ptr + index - data.data();
            }
            
            ptr += 32;
        }
        
        // Обработка оставшихся байтов
        while (ptr < end) {
            if (*ptr == target) {
                return ptr - data.data();
            }
            ptr++;
        }
        
        return std::string_view::npos;
    }
#endif

#ifdef __SSE4_2__
    if (has_sse42() && len >= 16) {
        __m128i target_vec = _mm_set1_epi8(target);
        const char* end = ptr + len;
        const char* aligned_start = ptr + ((16 - (reinterpret_cast<std::uintptr_t>(ptr) & 15)) & 15);
        const char* aligned_end = ptr + (len & ~15);
        
        // Обработка невыровненных байтов
        while (ptr < aligned_start && ptr < end) {
            if (*ptr == target) {
                return ptr - data.data();
            }
            ptr++;
        }
        
        // SSE4.2 обработка выровненных блоков
        while (ptr < aligned_end) {
            __m128i data_vec = _mm_load_si128(reinterpret_cast<const __m128i*>(ptr));
            __m128i cmp = _mm_cmpeq_epi8(data_vec, target_vec);
            int mask = _mm_movemask_epi8(cmp);
            
            if (mask != 0) {
                int index = find_first_set_bit(mask);
                return ptr + index - data.data();
            }
            
            ptr += 16;
        }
        
        // Обработка оставшихся байтов
        while (ptr < end) {
            if (*ptr == target) {
                return ptr - data.data();
            }
            ptr++;
        }
        
        return std::string_view::npos;
    }
#endif

    // Fallback на стандартный поиск
    const char* result = static_cast<const char*>(std::memchr(ptr, target, len));
    if (result) {
        return result - data.data();
    }
    return std::string_view::npos;
}

std::size_t find_any_char_simd(std::string_view data, char delim, char quote, std::size_t start_pos) {
    if (start_pos >= data.length()) {
        return std::string_view::npos;
    }
    
    const char* ptr = data.data() + start_pos;
    std::size_t len = data.length() - start_pos;
    
#ifdef __AVX2__
    if (has_avx2() && len >= 32) {
        __m256i delim_vec = _mm256_set1_epi8(delim);
        __m256i quote_vec = _mm256_set1_epi8(quote);
        const char* end = ptr + len;
        const char* aligned_start = ptr + ((32 - (reinterpret_cast<std::uintptr_t>(ptr) & 31)) & 31);
        const char* aligned_end = ptr + (len & ~31);
        
        // Обработка невыровненных байтов
        while (ptr < aligned_start && ptr < end) {
            if (*ptr == delim || *ptr == quote) {
                return ptr - data.data();
            }
            ptr++;
        }
        
        // AVX2 обработка - ищем оба символа одновременно
        while (ptr < aligned_end) {
            __m256i data_vec = _mm256_load_si256(reinterpret_cast<const __m256i*>(ptr));
            __m256i cmp_delim = _mm256_cmpeq_epi8(data_vec, delim_vec);
            __m256i cmp_quote = _mm256_cmpeq_epi8(data_vec, quote_vec);
            __m256i cmp_any = _mm256_or_si256(cmp_delim, cmp_quote);
            int mask = _mm256_movemask_epi8(cmp_any);
            
            if (mask != 0) {
                int index = find_first_set_bit(mask);
                return ptr + index - data.data();
            }
            
            ptr += 32;
        }
        
        // Оставшиеся байты
        while (ptr < end) {
            if (*ptr == delim || *ptr == quote) {
                return ptr - data.data();
            }
            ptr++;
        }
        
        return std::string_view::npos;
    }
#endif

#ifdef __SSE4_2__
    if (has_sse42() && len >= 16) {
        __m128i delim_vec = _mm_set1_epi8(delim);
        __m128i quote_vec = _mm_set1_epi8(quote);
        const char* end = ptr + len;
        const char* aligned_start = ptr + ((16 - (reinterpret_cast<std::uintptr_t>(ptr) & 15)) & 15);
        const char* aligned_end = ptr + (len & ~15);
        
        // Обработка невыровненных байтов
        while (ptr < aligned_start && ptr < end) {
            if (*ptr == delim || *ptr == quote) {
                return ptr - data.data();
            }
            ptr++;
        }
        
        // SSE4.2 обработка
        while (ptr < aligned_end) {
            __m128i data_vec = _mm_load_si128(reinterpret_cast<const __m128i*>(ptr));
            __m128i cmp_delim = _mm_cmpeq_epi8(data_vec, delim_vec);
            __m128i cmp_quote = _mm_cmpeq_epi8(data_vec, quote_vec);
            __m128i cmp_any = _mm_or_si128(cmp_delim, cmp_quote);
            int mask = _mm_movemask_epi8(cmp_any);
            
            if (mask != 0) {
                int index = find_first_set_bit(mask);
                return ptr + index - data.data();
            }
            
            ptr += 16;
        }
        
        // Оставшиеся байты
        while (ptr < end) {
            if (*ptr == delim || *ptr == quote) {
                return ptr - data.data();
            }
            ptr++;
        }
        
        return std::string_view::npos;
    }
#endif

    // Fallback - ищем оба символа по очереди
    std::size_t delim_pos = find_char_simd(data, delim, start_pos);
    std::size_t quote_pos = find_char_simd(data, quote, start_pos);
    
    if (delim_pos == std::string_view::npos) {
        return quote_pos;
    }
    if (quote_pos == std::string_view::npos) {
        return delim_pos;
    }
    
    return std::min(delim_pos, quote_pos);
}

void find_all_chars_simd(std::string_view data, char target, std::vector<std::size_t>& positions, std::size_t start_pos) {
    if (start_pos >= data.length()) {
        return;
    }
    
    const char* ptr = data.data() + start_pos;
    std::size_t len = data.length() - start_pos;
    positions.clear();
    positions.reserve(len / 10); // Предполагаем примерно 1 разделитель на 10 символов
    
#ifdef __AVX2__
    if (has_avx2() && len >= 32) {
        __m256i target_vec = _mm256_set1_epi8(target);
        const char* end = ptr + len;
        const char* aligned_start = ptr + ((32 - (reinterpret_cast<std::uintptr_t>(ptr) & 31)) & 31);
        const char* aligned_end = ptr + (len & ~31);
        
        // Обработка невыровненных байтов в начале
        while (ptr < aligned_start && ptr < end) {
            if (*ptr == target) {
                positions.push_back(ptr - data.data());
            }
            ptr++;
        }
        
        // AVX2 обработка выровненных блоков
        while (ptr < aligned_end) {
            __m256i data_vec = _mm256_load_si256(reinterpret_cast<const __m256i*>(ptr));
            __m256i cmp = _mm256_cmpeq_epi8(data_vec, target_vec);
            int mask = _mm256_movemask_epi8(cmp);
            
            // Обрабатываем все найденные символы в этом блоке
            while (mask != 0) {
                int index = find_first_set_bit(mask);
                positions.push_back(ptr + index - data.data());
                mask &= mask - 1; // Убираем обработанный бит
            }
            
            ptr += 32;
        }
        
        // Обработка оставшихся байтов
        while (ptr < end) {
            if (*ptr == target) {
                positions.push_back(ptr - data.data());
            }
            ptr++;
        }
        
        return;
    }
#endif

#ifdef __SSE4_2__
    if (has_sse42() && len >= 16) {
        __m128i target_vec = _mm_set1_epi8(target);
        const char* end = ptr + len;
        const char* aligned_start = ptr + ((16 - (reinterpret_cast<std::uintptr_t>(ptr) & 15)) & 15);
        const char* aligned_end = ptr + (len & ~15);
        
        // Обработка невыровненных байтов
        while (ptr < aligned_start && ptr < end) {
            if (*ptr == target) {
                positions.push_back(ptr - data.data());
            }
            ptr++;
        }
        
        // SSE4.2 обработка
        while (ptr < aligned_end) {
            __m128i data_vec = _mm_load_si128(reinterpret_cast<const __m128i*>(ptr));
            __m128i cmp = _mm_cmpeq_epi8(data_vec, target_vec);
            int mask = _mm_movemask_epi8(cmp);
            
            while (mask != 0) {
                int index = find_first_set_bit(mask);
                positions.push_back(ptr + index - data.data());
                mask &= mask - 1;
            }
            
            ptr += 16;
        }
        
        // Оставшиеся байты
        while (ptr < end) {
            if (*ptr == target) {
                positions.push_back(ptr - data.data());
            }
            ptr++;
        }
        
        return;
    }
#endif

    // Fallback - простой поиск
    for (std::size_t i = start_pos; i < data.length(); ++i) {
        if (data[i] == target) {
            positions.push_back(i);
        }
    }
}

std::size_t count_chars_simd(std::string_view data, char target) {
    std::size_t count = 0;
    const char* ptr = data.data();
    std::size_t len = data.length();
    
#ifdef __AVX2__
    if (has_avx2() && len >= 32) {
        __m256i target_vec = _mm256_set1_epi8(target);
        const char* end = ptr + len;
        const char* aligned_end = ptr + (len & ~31);
        
        while (ptr < aligned_end) {
            __m256i data_vec = _mm256_load_si256(reinterpret_cast<const __m256i*>(ptr));
            __m256i cmp = _mm256_cmpeq_epi8(data_vec, target_vec);
            int mask = _mm256_movemask_epi8(cmp);
            count += count_bits(mask);
            ptr += 32;
        }
        
        // Оставшиеся байты
        while (ptr < end) {
            if (*ptr == target) count++;
            ptr++;
        }
        
        return count;
    }
#endif

    // Fallback
    return std::count(data.begin(), data.end(), target);
}

bool is_ascii_simd(std::string_view data) {
    if (data.empty()) {
        return true;
    }
    
    const char* ptr = data.data();
    std::size_t len = data.length();
    
    // ASCII: все байты должны быть < 128 (0x80)
    // Проверяем, что нет байтов >= 0x80
    
#ifdef __AVX2__
    if (has_avx2() && len >= 32) {
        const char* end = ptr + len;
        const char* aligned_end = ptr + (len & ~31);
        
        // Обрабатываем выровненные блоки
        while (ptr < aligned_end) {
            __m256i data_vec = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(ptr));
            // Сравниваем: если data_vec >= 0x80, то не ASCII
            // Используем сравнение с 0x7F (127) и проверяем знак
            __m256i cmp = _mm256_cmpgt_epi8(data_vec, _mm256_set1_epi8(127));
            int mask = _mm256_movemask_epi8(cmp);
            
            if (mask != 0) {
                // Найден не-ASCII символ
                return false;
            }
            
            ptr += 32;
        }
        
        // Обрабатываем оставшиеся байты
        while (ptr < end) {
            if (static_cast<unsigned char>(*ptr) >= 0x80) {
                return false;
            }
            ptr++;
        }
        
        return true;
    }
#endif

#ifdef __SSE4_2__
    if (has_sse42() && len >= 16) {
        // Создаем вектор с 0x80 для сравнения
        __m128i threshold = _mm_set1_epi8(static_cast<char>(0x80));
        const char* end = ptr + len;
        const char* aligned_end = ptr + (len & ~15);
        
        // Обрабатываем выровненные блоки
        while (ptr < aligned_end) {
            __m128i data_vec = _mm_loadu_si128(reinterpret_cast<const __m128i*>(ptr));
            // Сравниваем: если data_vec >= 0x80, то не ASCII
            // Используем сравнение с 0x7F (127) и проверяем знак
            __m128i cmp = _mm_cmpgt_epi8(data_vec, _mm_set1_epi8(127));
            int mask = _mm_movemask_epi8(cmp);
            
            if (mask != 0) {
                // Найден не-ASCII символ (>= 0x80)
                return false;
            }
            
            ptr += 16;
        }
        
        // Обрабатываем оставшиеся байты
        while (ptr < end) {
            if (static_cast<unsigned char>(*ptr) >= 0x80) {
                return false;
            }
            ptr++;
        }
        
        return true;
    }
#endif

    // Fallback: простая проверка
    for (std::size_t i = 0; i < len; ++i) {
        if (static_cast<unsigned char>(ptr[i]) >= 0x80) {
            return false;
        }
    }
    
    return true;
}

} // namespace simd
} // namespace fastcsv

