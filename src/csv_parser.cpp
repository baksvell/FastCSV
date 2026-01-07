#include "fastcsv/csv_parser.hpp"
#include "fastcsv/simd_utils.hpp"
#include <algorithm>
#include <cctype>
#include <sstream>
#include <vector>
#include <fstream>
#include <chrono>
#include <cstdio>

namespace fastcsv {

CSVParser::CSVParser(const ParserConfig& config) : config_(config) {}

// Вспомогательная функция: проверяет, есть ли экранированные кавычки в строке
static bool has_escaped_quotes(std::string_view line, char quote_char) {
    if (line.length() < 2) {
        return false;
    }
    
    // Быстрая проверка: ищем пары кавычек ""
    // Используем SIMD для больших строк
    if (line.length() >= 16) {
        std::size_t quote_pos = 0;
        while (true) {
            quote_pos = simd::find_char_simd(line, quote_char, quote_pos);
            if (quote_pos == std::string_view::npos) {
                break;
            }
            // Проверяем, не является ли следующая кавычка экранированной
            if (quote_pos + 1 < line.length() && line[quote_pos + 1] == quote_char) {
                return true; // Нашли экранированную кавычку ""
            }
            quote_pos++;
        }
        return false;
    } else {
        // Для маленьких строк используем простой поиск
        for (std::size_t i = 0; i < line.length() - 1; ++i) {
            if (line[i] == quote_char && line[i + 1] == quote_char) {
                return true;
            }
        }
        return false;
    }
}

ParsedRow CSVParser::parse_line(std::string_view line) {
    ParsedRow result;
    // Фиксированное резервирование - избегаем лишнего прохода std::count()
    // Большинство CSV строк имеют <16 полей, но резервируем больше для избежания реаллокаций
    result.fields.reserve(16);
    
    if (line.empty()) {
        // Оптимизация: резервируем только одно поле для пустой строки
        result.fields.reserve(1);
        result.fields.push_back("");
        return result;
    }
    
    // Оптимизация: проверяем, есть ли кавычки в строке
    // Если кавычек нет, используем быстрый путь
    bool has_quotes = false;
    if (line.length() >= 16) {
        std::size_t quote_pos = simd::find_char_simd(line, config_.quote, 0);
        has_quotes = (quote_pos != std::string_view::npos);
    } else {
        for (char c : line) {
            if (c == config_.quote) {
                has_quotes = true;
                break;
            }
        }
    }
    
    if (!has_quotes) {
        // Нет кавычек - используем быстрый путь
        return parse_line_no_quotes(line);
    }
    
    // Есть кавычки - проверяем, есть ли экранированные кавычки
    bool has_escaped = has_escaped_quotes(line, config_.quote);
    if (!has_escaped) {
        // Нет экранированных кавычек - используем оптимизированный путь
        return parse_line_quotes_no_escape(line);
    }
    
    // Есть экранированные кавычки - используем полный путь
    const char* data = line.data();
    std::size_t pos = 0;
    std::size_t len = line.length();
    bool in_quotes = false;
    std::string current_field;
    // Оптимизация: более точное резервирование на основе средней длины поля
    // Предполагаем среднюю длину поля ~32 символа (меньше для лучшего использования памяти)
    current_field.reserve(32);
    
    constexpr std::size_t SIMD_THRESHOLD = 16; // Снижено для более частого использования SIMD
    
    while (pos < len) {
        // Пропускаем символы конца строки
        if (data[pos] == '\r' || data[pos] == '\n') {
            break;
        }
        
        if (!in_quotes) {
            // Вне кавычек: ищем разделитель или кавычку
            const char* start = data + pos;
            std::size_t remaining = len - pos;
            std::size_t next_special = std::string_view::npos;
            
            // Используем оптимизированный поиск
            if (remaining >= SIMD_THRESHOLD) {
                // Используем find_any_char_simd для поиска обоих символов за один проход
                next_special = simd::find_any_char_simd(line, config_.delimiter, config_.quote, pos);
            } else {
                // Для коротких строк используем прямой поиск
                for (std::size_t i = pos; i < len; ++i) {
                    char c = data[i];
                    if (c == '\r' || c == '\n') {
                        next_special = i;
                        break;
                    } else if (c == config_.delimiter || c == config_.quote) {
                        next_special = i;
                        break;
                    }
                }
            }
            
            if (next_special == std::string_view::npos) {
                // До конца строки нет специальных символов - копируем все оставшееся одним блоком
                current_field.append(start, remaining);
                pos = len;
                break;
            }
            
            // Копируем данные до специального символа одним блоком
            std::size_t copy_len = next_special - pos;
            if (copy_len > 0) {
                current_field.append(start, copy_len);
            }
            
            char special_char = data[next_special];
            pos = next_special;
            
            if (special_char == config_.quote) {
                // Начало поля в кавычках
                in_quotes = true;
                pos++;  // Пропускаем открывающую кавычку
            } else if (special_char == config_.delimiter) {
                // Конец поля
                if (config_.skip_initial_space) {
                    trim_whitespace(current_field);
                }
                result.fields.push_back(std::move(current_field));
                current_field.clear();
                // capacity сохраняется после clear(), не нужно резервировать снова
                pos++;  // Пропускаем разделитель
            }
        } else {
            // Внутри кавычек: ищем кавычку (возможно экранированную)
            // Оптимизация: копируем большие блоки данных за раз, обрабатываем экранированные кавычки эффективно
            const char* start = data + pos;
            std::size_t remaining = len - pos;
            
            // Используем SIMD для поиска кавычек
            std::size_t quote_pos = std::string_view::npos;
            if (remaining >= SIMD_THRESHOLD) {
                quote_pos = simd::find_char_simd(line, config_.quote, pos);
            } else {
                // Прямой поиск для коротких строк
                for (std::size_t i = pos; i < len; ++i) {
                    if (data[i] == config_.quote) {
                        quote_pos = i;
                        break;
                    }
                }
            }
            
            if (quote_pos == std::string_view::npos) {
                // Кавычка не найдена - копируем все оставшееся одним блоком
                current_field.append(start, remaining);
                pos = len;
                break;
            }
            
            // Копируем данные до кавычки одним блоком
            std::size_t copy_len = quote_pos - pos;
            if (copy_len > 0) {
                current_field.append(start, copy_len);
            }
            
            pos = quote_pos;
            
            // Оптимизированная проверка экранированной кавычки ""
            // Проверяем, не является ли следующая кавычка экранированной
            if (pos + 1 < len && data[pos + 1] == config_.quote) {
                // Экранированная кавычка "" - добавляем одну кавычку в поле
                current_field.append(1, config_.quote);
                pos += 2;  // Пропускаем обе кавычки, остаемся внутри кавычек
            } else {
                // Обычная закрывающая кавычка - выходим из кавычек
                in_quotes = false;
                pos++;  // Пропускаем закрывающую кавычку
            }
        }
    }
    
    // Добавляем последнее поле
    if (config_.skip_initial_space) {
        trim_whitespace(current_field);
    }
    result.fields.push_back(std::move(current_field));
    
    result.bytes_processed = pos;
    result.success = !in_quotes;
    
    return result;
}

ParsedRow CSVParser::parse_line_no_quotes(std::string_view line) {
    ParsedRow result;
    result.fields.reserve(16);
    
    if (line.empty()) {
        // Оптимизация: резервируем только одно поле для пустой строки
        result.fields.reserve(1);
        result.fields.push_back("");
        return result;
    }
    
    const char* data = line.data();
    std::size_t len = line.length();
    
    // Оптимизация: строка уже не содержит newline (он был удален в parse_chunk)
    // Поэтому используем всю длину строки
    // ВАЖНО: line_end должен быть равен len, а не больше!
    std::size_t line_end = len;
    
    // Оптимизация: используем SIMD для поиска всех разделителей за один проход
    // Для строк без кавычек можем обрабатывать более эффективно
    std::vector<std::size_t> delimiter_positions;
    
    if (len >= 16) {
        // Для больших строк используем SIMD
        std::string_view line_view(data, len);
        simd::find_all_chars_simd(line_view, config_.delimiter, delimiter_positions, 0);
    } else {
        // Для маленьких строк используем простой поиск
        for (std::size_t i = 0; i < len; ++i) {
            if (data[i] == config_.delimiter) {
                delimiter_positions.push_back(i);
            }
        }
    }
    
    // Улучшенное резервирование: количество полей = количество разделителей + 1
    // Это точная оценка, поэтому резервируем именно столько, сколько нужно
    std::size_t num_fields = delimiter_positions.size() + 1;
    result.fields.reserve(num_fields);
    
    // Оптимизация: если количество полей небольшое (<10), можем использовать более эффективный путь
    // Но для простоты оставляем общий путь
    
    // Оптимизация: обрабатываем поля между разделителями
    // Используем emplace_back для прямого создания строк без промежуточных копий
    std::size_t field_start = 0;
    
    for (std::size_t delim_pos : delimiter_positions) {
        if (delim_pos >= field_start) {
            std::size_t field_len = delim_pos - field_start;
            
            // Оптимизация: используем emplace_back для прямого создания строки
            // Это избегает промежуточных копий и аллокаций
            if (field_len > 0) {
                if (!config_.skip_initial_space) {
                    // Нет необходимости в trim - создаем строку напрямую
                    result.fields.emplace_back(data + field_start, field_len);
                } else {
                    // Нужен trim - создаем временную строку
                    std::string field(data + field_start, field_len);
                    trim_whitespace(field);
                    result.fields.push_back(std::move(field));
                }
            } else {
                // Пустое поле - используем интернированную пустую строку
                result.fields.emplace_back();
            }
            field_start = delim_pos + 1;
        }
    }
    
    // Оптимизация: добавляем последнее поле
    // Используем emplace_back для прямого создания без промежуточных копий
    if (delimiter_positions.empty()) {
        // Нет разделителей - вся строка это одно поле
        if (line_end > 0) {
            if (!config_.skip_initial_space) {
                result.fields.emplace_back(data, line_end);
            } else {
                std::string field(data, line_end);
                trim_whitespace(field);
                result.fields.push_back(std::move(field));
            }
        } else {
            // Пустая строка - используем emplace_back для пустой строки
            result.fields.emplace_back();
        }
        } else {
            // Есть разделители - добавляем последнее поле после последнего разделителя
            
            if (field_start < len) {
                std::size_t field_len = len - field_start;
                
                if (field_len > 0) {
                    if (!config_.skip_initial_space) {
                        result.fields.emplace_back(data + field_start, field_len);
                    } else {
                        std::string field(data + field_start, field_len);
                        trim_whitespace(field);
                        result.fields.push_back(std::move(field));
                    }
                } else {
                    // КРИТИЧЕСКАЯ ИСПРАВЛЕНИЕ: field_len == 0 означает, что после последнего разделителя
                    // нет данных. Но это может быть trailing разделитель, который был добавлен неправильно.
                    // Проверяем, не является ли это ошибкой в parse_chunk
                    // Если строка передается правильно, field_len == 0 означает trailing разделитель,
                    // и мы должны добавить пустое поле
                    result.fields.emplace_back();
                }
            } else {
                // КРИТИЧЕСКАЯ ИСПРАВЛЕНИЕ: field_start >= len означает, что строка заканчивается на разделитель
                // Но это может быть ошибка в parse_chunk, где строка передается с trailing разделителем
                // Проверяем, действительно ли строка заканчивается на разделитель
                // Если строка передается правильно из parse_chunk, она не должна заканчиваться на разделитель
                // Поэтому мы НЕ должны добавлять пустое поле в этом случае
                // Пустое поле добавляется только если строка явно заканчивается на разделитель (например, 'name,description,')
                
                // КРИТИЧЕСКАЯ ИСПРАВЛЕНИЕ: field_start >= len означает, что строка заканчивается на разделитель
                // Но для строк без trailing разделителя field_start должен быть < len
                // Если field_start >= len, это означает trailing разделитель, и мы добавляем пустое поле
                // НО: проверяем, действительно ли строка заканчивается на разделитель
                // Если строка не заканчивается на разделитель, но field_start >= len, это ошибка
                // и мы НЕ должны добавлять пустое поле
                if (len > 0 && data[len - 1] == config_.delimiter) {
                    // Строка действительно заканчивается на разделитель - добавляем пустое поле
                    result.fields.emplace_back();
                }
                // Иначе: field_start >= len, но строка не заканчивается на разделитель
                // Это ошибка в parse_chunk - не добавляем пустое поле
                // Это может произойти, если строка передается неправильно из parse_chunk
            }
        }
    
    result.bytes_processed = line_end;
    result.success = true;
    
    
    
    return result;
}

ParsedRow CSVParser::parse_line_quotes_no_escape(std::string_view line) {
    
    
    ParsedRow result;
    result.fields.reserve(16);
    
    if (line.empty()) {
        result.fields.reserve(1);
        result.fields.push_back("");
        return result;
    }
    
    const char* data = line.data();
    std::size_t pos = 0;
    std::size_t len = line.length();
    bool in_quotes = false;
    std::string current_field;
    // КРИТИЧЕСКАЯ ОПТИМИЗАЦИЯ: более агрессивное резервирование для уменьшения реаллокаций
    // Для строк с кавычками средняя длина поля может быть больше
    // Используем более точную оценку на основе длины строки и количества разделителей
    std::size_t estimated_field_len = 128; // Увеличено базовое значение для уменьшения реаллокаций
    if (len > 0) {
        // Оцениваем среднюю длину поля: длина строки / (примерное количество полей)
        // Для CSV с кавычками обычно 5-10 полей на строку
        std::size_t estimated_fields = std::max(std::size_t(5), std::min(len / 20, std::size_t(20)));
        estimated_field_len = std::max(std::size_t(64), std::min(len / estimated_fields, std::size_t(512)));
    }
    current_field.reserve(estimated_field_len);
    
    // КРИТИЧЕСКАЯ ОПТИМИЗАЦИЯ: предварительно резервируем место для полей
    // Оцениваем количество полей на основе количества разделителей
    std::size_t estimated_num_fields = 16;
    if (len >= 16) {
        std::size_t delimiter_count = simd::count_chars_simd(line, config_.delimiter);
        estimated_num_fields = delimiter_count + 1;
        estimated_num_fields = std::min(estimated_num_fields, std::size_t(256));
    }
    result.fields.reserve(estimated_num_fields);
    
    constexpr std::size_t SIMD_THRESHOLD = 16;
    
    // КРИТИЧЕСКАЯ ОПТИМИЗАЦИЯ: строка уже не содержит newline (он был удален в parse_chunk)
    // Поэтому можем убрать проверку newline в цикле и упростить логику
    while (pos < len) {
        if (!in_quotes) {
            // Вне кавычек: ищем разделитель или кавычку
            const char* start = data + pos;
            std::size_t remaining = len - pos;
            std::size_t next_special = std::string_view::npos;
            
            // КРИТИЧЕСКАЯ ОПТИМИЗАЦИЯ: используем SIMD для одновременного поиска delimiter и quote
            // КРИТИЧЕСКАЯ ОПТИМИЗАЦИЯ: порог для SIMD 24 байт (уменьшен для более частого использования SIMD)
            const char* search_start = data + pos;
            if (remaining >= 24) {
                // Для блоков >=24 байт используем SIMD
                std::string_view line_view(data, len);
                next_special = simd::find_any_char_simd(line_view, config_.delimiter, config_.quote, pos);
            } else {
                // Для маленьких блоков используем прямой поиск - это быстрее из-за меньшего overhead
                for (std::size_t i = 0; i < remaining; ++i) {
                    char c = search_start[i];
                    if (c == config_.delimiter || c == config_.quote) {
                        next_special = pos + i;
                        break;
                    }
                }
            }
            
            if (next_special == std::string_view::npos) {
                // До конца строки нет специальных символов - последнее поле
                // КРИТИЧЕСКАЯ ОПТИМИЗАЦИЯ: объединяем проверки для уменьшения ветвлений
                if (current_field.empty()) {
                    result.fields.emplace_back(start, remaining);
                } else {
                    current_field.append(start, remaining);
                    result.fields.emplace_back(std::move(current_field));
                    current_field.clear();
                }
                pos = len;
                // КРИТИЧЕСКАЯ ИСПРАВЛЕНИЕ: последнее поле уже добавлено, не нужно добавлять его снова в конце функции
                // Устанавливаем флаг, чтобы пропустить добавление последнего поля в конце функции
                current_field = "\0";  // Используем специальное значение для обозначения, что последнее поле уже добавлено
                break;
            }
            
            // КРИТИЧЕСКАЯ ОПТИМИЗАЦИЯ: вычисляем field_len до проверки символа
            std::size_t field_len = next_special - pos;
            char special_char = data[next_special];
            pos = next_special + 1;  // Пропускаем специальный символ сразу
            
            if (special_char == config_.quote) {
                // Начало поля в кавычках
                // КРИТИЧЕСКАЯ ОПТИМИЗАЦИЯ: если current_field пустое и field_len = 0, 
                // то это начало поля в кавычках - не нужно ничего добавлять
                // КРИТИЧЕСКАЯ ОПТИМИЗАЦИЯ: оптимизируем обработку начала поля в кавычках
                if (field_len > 0) {
                    if (current_field.empty()) {
                        // Поле пустое - используем assign для лучшей производительности
                        current_field.assign(start, field_len);
                    } else {
                        // Поле не пустое - резервируем только если нужно
                        std::size_t total_size = current_field.size() + field_len;
                        if (current_field.capacity() < total_size) {
                            current_field.reserve(total_size + estimated_field_len);
                        }
                        current_field.append(start, field_len);
                    }
                }
                in_quotes = true;
            } else {
                // Конец поля (разделитель)
                // КРИТИЧЕСКАЯ ОПТИМИЗАЦИЯ: оптимизируем создание поля - избегаем лишних копирований
                if (current_field.empty() && field_len > 0) {
                    // Поле пустое - создаем напрямую из string_view (без промежуточных копий)
                    if (!config_.skip_initial_space) {
                        result.fields.emplace_back(start, field_len);
                    } else {
                        // КРИТИЧЕСКАЯ ОПТИМИЗАЦИЯ: резервируем место заранее для trim
                        std::string field_str;
                        field_str.reserve(field_len);
                        field_str.assign(start, field_len);
                        trim_whitespace(field_str);
                        result.fields.emplace_back(std::move(field_str));
                    }
                } else {
                    // Поле не пустое - добавляем данные и создаем поле
                    if (field_len > 0) {
                        // КРИТИЧЕСКАЯ ОПТИМИЗАЦИЯ: резервируем только если нужно, но с запасом
                        std::size_t total_size = current_field.size() + field_len;
                        if (current_field.capacity() < total_size) {
                            current_field.reserve(total_size + estimated_field_len / 2);
                        }
                        current_field.append(start, field_len);
                    }
                    if (config_.skip_initial_space) {
                        trim_whitespace(current_field);
                    }
                    result.fields.emplace_back(std::move(current_field));
                    current_field.clear();
                    // КРИТИЧЕСКАЯ ОПТИМИЗАЦИЯ: сохраняем capacity для следующего поля
                    // Используем более агрессивное резервирование для уменьшения реаллокаций
                    if (current_field.capacity() < estimated_field_len) {
                        current_field.reserve(estimated_field_len);
                    }
                }
            }
        } else {
            // Внутри кавычек: ищем только закрывающую кавычку (без экранированных)
            // КРИТИЧЕСКАЯ ОПТИМИЗАЦИЯ: используем SIMD для поиска кавычки и минимизируем append вызовы
            const char* start = data + pos;
            std::size_t remaining = len - pos;
            std::size_t quote_pos = std::string_view::npos;
            
            // КРИТИЧЕСКАЯ ОПТИМИЗАЦИЯ: используем SIMD для поиска кавычки
            // КРИТИЧЕСКАЯ ОПТИМИЗАЦИЯ: порог для SIMD 32 байт (оптимальный баланс между overhead и производительностью)
            const char* search_start = data + pos;
            if (remaining >= 32) {
                // Для блоков >=32 байт используем SIMD
                std::string_view line_view(data, len);
                quote_pos = simd::find_char_simd(line_view, config_.quote, pos);
            } else {
                // Для маленьких блоков используем прямой поиск - это быстрее из-за меньшего overhead
                for (std::size_t i = 0; i < remaining; ++i) {
                    if (search_start[i] == config_.quote) {
                        quote_pos = pos + i;
                        break;
                    }
                }
            }
            
            if (quote_pos == std::string_view::npos) {
                // Кавычка не найдена - копируем все оставшееся одним блоком
                // КРИТИЧЕСКАЯ ОПТИМИЗАЦИЯ: упрощаем логику
                if (current_field.empty()) {
                    current_field.assign(start, remaining);
                } else {
                    std::size_t total_size = current_field.size() + remaining;
                    if (current_field.capacity() < total_size) {
                        current_field.reserve(total_size + estimated_field_len);
                    }
                    current_field.append(start, remaining);
                }
                pos = len;
                break;
            }
            
            // КРИТИЧЕСКАЯ ОПТИМИЗАЦИЯ: копируем данные до кавычки одним блоком
            // Оптимизация: используем assign для пустых полей, append для непустых
            std::size_t copy_len = quote_pos - pos;
            if (copy_len > 0) {
                if (current_field.empty()) {
                    current_field.assign(start, copy_len);
                } else {
                    // Резервируем только если нужно
                    std::size_t total_size = current_field.size() + copy_len;
                    if (current_field.capacity() < total_size) {
                        current_field.reserve(total_size + estimated_field_len);
                    }
                    current_field.append(start, copy_len);
                }
            }
            
            // Обычная закрывающая кавычка (без экранирования)
            in_quotes = false;
            pos = quote_pos + 1;  // Пропускаем закрывающую кавычку
            
            // КРИТИЧЕСКАЯ ОПТИМИЗАЦИЯ: после закрывающей кавычки должен быть разделитель или конец строки
            // Проверяем это сразу, чтобы избежать лишних итераций
            // КРИТИЧЕСКАЯ ОПТИМИЗАЦИЯ: объединяем проверки для уменьшения ветвлений
            if (pos < len) {
                if (data[pos] == config_.delimiter) {
                    // Сразу после кавычки разделитель - завершаем поле
                    // КРИТИЧЕСКАЯ ОПТИМИЗАЦИЯ: проверяем skip_initial_space только если поле не пустое
                    if (!current_field.empty()) {
                        if (config_.skip_initial_space) {
                            trim_whitespace(current_field);
                        }
                        result.fields.emplace_back(std::move(current_field));
                        current_field.clear();
                        // Сохраняем capacity для следующего поля
                        if (current_field.capacity() < estimated_field_len) {
                            current_field.reserve(estimated_field_len);
                        }
                    } else {
                        // Пустое поле - добавляем напрямую
                        result.fields.emplace_back("");
                    }
                    pos++; // Пропускаем разделитель
                    continue; // Переходим к следующему полю
                }
                // Если не разделитель, возможно пробел (для skip_initial_space) - продолжаем цикл
            }
        }
    }
    
    // Добавляем последнее поле
    // КРИТИЧЕСКАЯ ИСПРАВЛЕНИЕ: проверяем, не было ли последнее поле уже добавлено в цикле
    // Если current_field == "\0", это означает, что последнее поле уже добавлено
    if (current_field.size() == 1 && current_field[0] == '\0') {
        // Последнее поле уже добавлено в цикле - ничего не делаем
    } else if (pos >= len && !current_field.empty()) {
        // КРИТИЧЕСКАЯ ИСПРАВЛЕНИЕ: если pos >= len, это означает, что мы дошли до конца строки
        // и последнее поле уже должно быть добавлено в цикле (строка 537-548)
        // Поэтому не нужно добавлять его снова
        // Но если current_field не пустое, это означает, что мы не дошли до конца строки в цикле
        // и нужно добавить последнее поле
        if (config_.skip_initial_space) {
            trim_whitespace(current_field);
        }
        result.fields.emplace_back(std::move(current_field));
    } else if (pos < len && !current_field.empty()) {
        // pos < len означает, что мы не дошли до конца строки в цикле
        // и нужно добавить последнее поле
        if (config_.skip_initial_space) {
            trim_whitespace(current_field);
        }
        result.fields.emplace_back(std::move(current_field));
    } else if (pos >= len && current_field.empty()) {
        // pos >= len и current_field пустое - это означает, что последнее поле уже добавлено в цикле
        // и не нужно добавлять пустое поле
    } else {
        // Пустое поле - добавляем напрямую только если мы не дошли до конца строки
        if (pos < len) {
            result.fields.emplace_back("");
        }
    }
    
    result.bytes_processed = pos;
    result.success = !in_quotes;
    
    
    
    return result;
}

std::vector<ParsedRow> CSVParser::parse_chunk(std::string_view data) {
    std::vector<ParsedRow> results;
    
    // Улучшенное резервирование: более точная оценка количества строк
    // Подсчитываем количество newline символов для более точной оценки
    std::size_t newline_count = 0;
    if (data.length() >= 16) {
        // Используем SIMD для подсчета newline - это быстрее чем простой цикл
        newline_count = simd::count_chars_simd(data, '\n');
        // Также учитываем \r\n - проверяем количество \r
        std::size_t cr_count = simd::count_chars_simd(data, '\r');
        // Для Windows (\r\n) количество строк = количество \n
        // Для старых Mac (\r) количество строк = количество \r
        // Берем максимум для консервативной оценки
        if (cr_count > newline_count) {
            // Возможно старый Mac формат или смешанный
            newline_count = cr_count;
        }
    } else {
        // Для маленьких блоков используем простой подсчет
        for (char c : data) {
            if (c == '\n') {
                newline_count++;
            }
        }
    }
    
    // Оценка: количество строк ≈ количество newline + 1 (последняя строка может не иметь newline)
    std::size_t estimated_rows = newline_count + 1;
    
    // Адаптивное резервирование: упрощенная версия для лучшей производительности
    // Используем более простую оценку без полного анализа первых строк
    std::size_t avg_fields_per_row = 16; // Значение по умолчанию
    if (data.length() > 100 && estimated_rows > 0) {
        // Быстрая оценка: используем SIMD для подсчета разделителей в первых 512 байтах
        std::size_t sample_size = std::min(data.length(), std::size_t(512));
        std::size_t sample_delimiters = 0;
        if (sample_size >= 16) {
            std::string_view sample(data.data(), sample_size);
            sample_delimiters = simd::count_chars_simd(sample, config_.delimiter);
        } else {
            for (std::size_t i = 0; i < sample_size; ++i) {
                if (data[i] == config_.delimiter) {
                    sample_delimiters++;
                }
            }
        }
        
        // Оцениваем количество строк в выборке
        std::size_t sample_newlines = 0;
        for (std::size_t i = 0; i < sample_size && i < data.length(); ++i) {
            if (data[i] == '\n') {
                sample_newlines++;
            }
        }
        
        if (sample_newlines > 0) {
            // Среднее количество полей на строку
            avg_fields_per_row = (sample_delimiters / sample_newlines) + 1;
            // Ограничиваем разумными пределами
            avg_fields_per_row = std::max(std::size_t(1), std::min(avg_fields_per_row, std::size_t(256)));
        }
    }
    
    const char* ptr = data.data();
    std::size_t pos = 0;
    std::size_t len = data.length();
    
    // КРИТИЧЕСКАЯ ОПТИМИЗАЦИЯ для больших файлов: увеличиваем максимальное резервирование
    // Для больших чанков резервируем больше памяти для уменьшения реаллокаций
    std::size_t max_reserve = 2048; // По умолчанию
    if (len >= 1048576) {  // >=1MB - огромный чанк
        max_reserve = 8192;  // Максимальное резервирование для огромных чанков
    } else if (len >= 524288) {  // >=512KB - очень большой чанк
        max_reserve = 4096;  // Увеличиваем для больших чанков
    } else if (len >= 262144) {  // >=256KB - большой чанк
        max_reserve = 3072;  // Среднее значение
    }
    results.reserve(std::min(estimated_rows, max_reserve));
    bool in_quotes = false;
    std::size_t line_start = 0;
    
    // Оптимизация: предварительно проверяем, есть ли кавычки в чанке
    // Если кавычек нет, можем использовать более быстрый путь
    bool chunk_has_quotes = false;
    if (len >= 16) {
        std::size_t quote_pos = simd::find_char_simd(data, config_.quote, 0);
        chunk_has_quotes = (quote_pos != std::string_view::npos);
    } else {
        for (std::size_t k = 0; k < len; ++k) {
            if (ptr[k] == config_.quote) {
                chunk_has_quotes = true;
                break;
            }
        }
    }
    
    
    
    // Если кавычек нет, используем оптимизированный путь для всех строк
    if (!chunk_has_quotes) {
        // КРИТИЧЕСКАЯ ОПТИМИЗАЦИЯ: умное резервирование в зависимости от размера файла
        // Для маленьких файлов полное резервирование полезно (исправляет регрессии)
        // Для больших файлов ограничиваем резервирование (избегаем overhead)
        std::size_t reserve_size = newline_count > 0 ? newline_count : 1;
        if (reserve_size > 5000) {
            // КРИТИЧЕСКАЯ ОПТИМИЗАЦИЯ для очень больших файлов (>5000 строк)
            // Используем менее агрессивное ограничение для лучшего баланса между overhead и реаллокациями
            // Формула: базовое (2000) + средний диапазон (3000/2=1500) + остаток/4
            reserve_size = 2000 + (3000) / 2 + (reserve_size - 5000) / 4;
            // Для 10000 строк: 2000 + 1500 + 5000/4 = 2000 + 1500 + 1250 = 4750
        } else if (reserve_size > 2000) {
            // Для средних больших файлов (2000-5000 строк) используем текущую формулу
            reserve_size = 2000 + (reserve_size - 2000) / 4;
        }
        // Проверяем, не было ли уже резервирование на строке 670
        // Если уже резервировали больше, не перезаписываем
        if (results.capacity() < reserve_size) {
            results.reserve(reserve_size);
        }
        
        // КРИТИЧЕСКАЯ ОПТИМИЗАЦИЯ для больших файлов: предварительно находим все newline позиции
        // Для очень больших файлов (>1MB) используем более агрессивную оптимизацию
        bool is_very_large = (len >= 1048576);  // >=1MB
        // КРИТИЧЕСКАЯ ОПТИМИЗАЦИЯ: уменьшен порог до 256KB для лучшей производительности
        // Для больших чанков (>256KB) это быстрее чем искать каждый раз
        std::vector<std::size_t> newline_positions;
        if (len >= 262144) {  // >=256KB - большой чанк (уменьшен порог)
            // Предварительно находим все позиции \n за один проход
            simd::find_all_chars_simd(data, '\n', newline_positions, 0);
        }
        
        // Быстрый путь: обрабатываем все строки без кавычек пакетно
        std::size_t line_start = 0;
        std::size_t newline_idx = 0;
        
        while (line_start < len) {
            // Ищем следующий newline
            std::size_t line_end = std::string_view::npos;
            
            if (len >= 262144 && !newline_positions.empty() && newline_idx < newline_positions.size()) {
                // Используем предварительно найденные позиции (уменьшен порог)
                line_end = newline_positions[newline_idx++];
            } else {
                // Для средних чанков ищем каждый раз
                std::size_t newline_pos = simd::find_char_simd(data, '\n', line_start);
                if (newline_pos != std::string_view::npos) {
                    line_end = newline_pos;
                } else {
                    // Нет \n - ищем \r (только если нужно)
                    std::size_t cr_pos = simd::find_char_simd(data, '\r', line_start);
                    if (cr_pos != std::string_view::npos) {
                        if (cr_pos + 1 < len && ptr[cr_pos + 1] == '\n') {
                            line_end = cr_pos + 1;
                        } else {
                            line_end = cr_pos;
                        }
                    }
                }
            }
            
            if (line_end == std::string_view::npos) {
                // Последняя строка без newline
                line_end = len;
            }
            
            // Парсим строку без кавычек
            std::size_t line_len = line_end - line_start;
            
            
            
            // КРИТИЧЕСКАЯ ИСПРАВЛЕНИЕ: для Windows \r\n нужно исключить \r из строки
            // line_end указывает на позицию \n, а line_len включает все до \n (включая \r, если он есть)
            // Для \r\n нужно исключить \r из line_len
            bool is_crlf = false;
            if (line_len > 0 && line_end > line_start && ptr[line_end - 1] == '\r') {
                // Это \r\n - исключаем \r из line_len
                line_len--;
                is_crlf = true;
            }
            
            
            // Для обычного \n line_len уже не включает \n (так как line_end указывает на позицию \n, а не после него)
            
            if (line_len > 0) {
                // КРИТИЧЕСКАЯ ИСПРАВЛЕНИЕ: убеждаемся, что line не выходит за границы данных
                // Это защита от выхода за границы буфера
                // КРИТИЧЕСКАЯ ИСПРАВЛЕНИЕ: если is_crlf == true, то line_len уже не включает \r,
                // и мы НЕ должны использовать len - line_start, так как оно включает \r\n
                // Проверяем границы только если is_crlf == false
                if (!is_crlf && line_start + line_len > len) {
                    // Ошибка: line выходит за границы данных
                    // Это не должно происходить, но на всякий случай ограничиваем
                    line_len = len - line_start;
                }
                // АЛЬТЕРНАТИВНЫЙ ПОДХОД: создаем string_view с явной проверкой границ
                // Убеждаемся, что line_len не превышает доступные данные
                // КРИТИЧЕСКАЯ ИСПРАВЛЕНИЕ: line_len уже был уменьшен на 1 для \r\n (если is_crlf == true)
                // Поэтому actual_line_len должен быть равен line_len, а не min(line_len, len - line_start)
                // Потому что len - line_start может включать \r\n, а line_len уже не включает \r
                // КРИТИЧЕСКАЯ ИСПРАВЛЕНИЕ: line_len уже был уменьшен на 1 для \r\n (если is_crlf == true)
                // Поэтому actual_line_len должен быть равен line_len
                // НЕ используем len - line_start, так как для многострочного буфера оно включает следующие строки
                std::size_t actual_line_len = line_len;
                std::string line_str(ptr + line_start, actual_line_len);
                
                std::string_view line(line_str);
                
                
                
                ParsedRow row = parse_line_no_quotes(line);
                
                // Вычисляем bytes_processed: учитываем newline символы
                std::size_t bytes_processed = actual_line_len;
                if (is_crlf) {
                    // Это \r\n - пропускаем оба символа
                    // line_end указывает на \n, поэтому пропускаем line_end + 1 (пропускаем \n)
                    // \r уже был исключен из line_len, поэтому он не попадет в следующую строку
                    bytes_processed += 2;
                    line_start = line_end + 1;  // line_end указывает на \n, пропускаем его
                } else if (line_end < len && ptr[line_end] == '\r' && line_end + 1 < len && ptr[line_end + 1] == '\n') {
                    // Это \r\n, но line_end указывает на \r
                    bytes_processed += 2;
                    line_start = line_end + 2;
                } else if (line_end < len && (ptr[line_end] == '\n' || ptr[line_end] == '\r')) {
                    bytes_processed += 1;
                    line_start = line_end + 1;
                } else {
                    line_start = line_end;
                }
                
                row.bytes_processed = bytes_processed;
                results.push_back(std::move(row));
            } else {
                // Пустая строка
                ParsedRow row;
                row.fields.push_back("");
                row.success = true;
                if (line_end < len && ptr[line_end] == '\r' && line_end + 1 < len && ptr[line_end + 1] == '\n') {
                    line_start = line_end + 2;
                    row.bytes_processed = 2;
                } else if (line_end < len && (ptr[line_end] == '\n' || ptr[line_end] == '\r')) {
                    line_start = line_end + 1;
                    row.bytes_processed = 1;
                } else {
                    line_start = line_end;
                    row.bytes_processed = 0;
                }
                results.push_back(std::move(row));
            }
        }
        
        return results;
    }
    
    // Есть кавычки - проверяем, есть ли экранированные кавычки в чанке
    // Если экранированных кавычек нет, можем использовать быстрый путь
    // КРИТИЧЕСКАЯ ОПТИМИЗАЦИЯ: используем SIMD для больших чанков
    bool chunk_has_escaped_quotes = false;
    if (chunk_has_quotes && len >= 2) {
        if (len >= 64) {
            // Для больших чанков используем оптимизированный поиск
            // Ищем первую кавычку, затем проверяем следующую
            std::size_t quote_pos = 0;
            while (quote_pos < len - 1) {
                std::string_view search_view(ptr + quote_pos, len - quote_pos);
                std::size_t next_quote = simd::find_char_simd(search_view, config_.quote, 0);
                if (next_quote == std::string_view::npos) {
                    break;
                }
                std::size_t actual_pos = quote_pos + next_quote;
                if (actual_pos + 1 < len && ptr[actual_pos + 1] == config_.quote) {
                    chunk_has_escaped_quotes = true;
                    break;
                }
                quote_pos = actual_pos + 1;
            }
        } else {
            // Для маленьких чанков используем прямой поиск
            for (std::size_t k = 0; k < len - 1; ++k) {
                if (ptr[k] == config_.quote && ptr[k + 1] == config_.quote) {
                    chunk_has_escaped_quotes = true;
                    break;
                }
            }
        }
    }
    
    // ВКЛЮЧЕНО: упрощенная batch обработка для файлов с кавычками (но без экранированных)
    // КРИТИЧЕСКАЯ ОПТИМИЗАЦИЯ: используем batch обработку для ускорения файлов с кавычками
    // Это критично для производительности - без этого файлы с кавычками в 3x медленнее
    if (chunk_has_quotes && !chunk_has_escaped_quotes) {
        // КРИТИЧЕСКАЯ ОПТИМИЗАЦИЯ: предварительно резервируем место для результатов
        // Оцениваем количество строк на основе количества newline
        std::size_t estimated_rows = newline_count;
        if (estimated_rows == 0) {
            estimated_rows = 1; // Минимум одна строка
        }
        results.reserve(estimated_rows);
        
        // Быстрый путь: обрабатываем все строки с кавычками (но без экранированных) пакетно
        std::size_t line_start = 0;
        while (line_start < len) {
            // Ищем следующий newline (учитывая, что newline внутри кавычек не является концом строки)
            std::size_t line_end = std::string_view::npos;
            bool in_quotes_line = false;
            
            // КРИТИЧЕСКАЯ ОПТИМИЗАЦИЯ: упрощенный и быстрый поиск концов строк для файлов с кавычками
            // Используем прямой поиск с отслеживанием состояния кавычек - это быстрее для большинства случаев
            std::size_t i = line_start;
            std::size_t remaining = len - line_start;
            
            // КРИТИЧЕСКАЯ ОПТИМИЗАЦИЯ: для больших блоков используем SIMD для одновременного поиска кавычек и newline
            // Для маленьких блоков прямой поиск быстрее из-за меньшего overhead
            // КРИТИЧЕСКАЯ ОПТИМИЗАЦИЯ: уменьшен порог с 64 до 48 байт для более частого использования SIMD
            if (remaining >= 48) {
                // Для больших блоков используем оптимизированный поиск с SIMD
                // Ищем кавычки и newline одновременно, отслеживая состояние кавычек
                std::size_t pos = 0;
                while (pos < remaining) {
                    std::size_t search_remaining = remaining - pos;
                    // КРИТИЧЕСКАЯ ОПТИМИЗАЦИЯ: уменьшен порог с 32 до 24 байт для более частого использования SIMD
                    if (search_remaining >= 24) {
                        // Используем SIMD для одновременного поиска кавычек и newline
                        std::string_view sub_view(ptr + line_start + pos, search_remaining);
                        std::size_t quote_pos = simd::find_char_simd(sub_view, config_.quote, 0);
                        std::size_t newline_pos = simd::find_char_simd(sub_view, '\n', 0);
                        
                        // Находим ближайший специальный символ
                        std::size_t next_special = std::string_view::npos;
                        bool is_quote = false;
                        if (quote_pos != std::string_view::npos) {
                            next_special = quote_pos;
                            is_quote = true;
                        }
                        if (newline_pos != std::string_view::npos && 
                            (next_special == std::string_view::npos || newline_pos < next_special)) {
                            next_special = newline_pos;
                            is_quote = false;
                        }
                        
                        if (next_special != std::string_view::npos) {
                            pos += next_special;
                            if (is_quote) {
                                in_quotes_line = !in_quotes_line;
                                pos++;
                            } else {
                                // Нашли newline
                                if (!in_quotes_line) {
                                    line_end = line_start + pos;
                                    break;
                                } else {
                                    // Newline внутри кавычек - пропускаем
                                    pos++;
                                }
                            }
                        } else {
                            // Не нашли специальных символов - до конца
                            break;
                        }
                    } else {
                        // Для маленьких блоков используем прямой поиск
                        bool found = false;
                        for (std::size_t j = pos; j < remaining; ++j) {
                            char c = ptr[line_start + j];
                            if (c == config_.quote) {
                                in_quotes_line = !in_quotes_line;
                                pos = j + 1;
                                found = true;
                                break;
                            } else if (c == '\n' && !in_quotes_line) {
                                line_end = line_start + j;
                                found = true;
                                break;
                            } else if (c == '\r' && !in_quotes_line) {
                                if (j + 1 < remaining && ptr[line_start + j + 1] == '\n') {
                                    line_end = line_start + j + 1;
                                } else {
                                    line_end = line_start + j;
                                }
                                found = true;
                                break;
                            }
                        }
                        if (!found) {
                            break;
                        }
                    }
                }
            } else {
                // Для маленьких и средних блоков используем прямой поиск - это быстрее
                while (i < len) {
                    char c = ptr[i];
                    
                    if (c == config_.quote) {
                        // Нашли кавычку - переключаем состояние
                        in_quotes_line = !in_quotes_line;
                        i++;
                    } else if (!in_quotes_line) {
                        // Вне кавычек - проверяем на newline
                        if (c == '\n') {
                            line_end = i;
                            break;
                        } else if (c == '\r') {
                            // Проверяем, не является ли это \r\n
                            if (i + 1 < len && ptr[i + 1] == '\n') {
                                // Это \r\n - используем позицию \n
                                line_end = i + 1;
                            } else {
                                // Это просто \r
                                line_end = i;
                            }
                            break;
                        } else {
                            i++;
                        }
                    } else {
                        // Внутри кавычек - просто пропускаем символ
                        i++;
                    }
                }
            }
            
            if (line_end == std::string_view::npos) {
                // Не нашли newline - до конца данных
                line_end = len;
            }
            
            // Парсим строку с кавычками (но без экранированных)
            std::size_t line_len = line_end - line_start;
            
            // КРИТИЧЕСКАЯ ИСПРАВЛЕНИЕ: для Windows \r\n нужно исключить \r из строки
            // Если line_end указывает на \n, а перед ним \r, то исключаем \r из line_len
            bool is_crlf = false;
            if (line_len > 0 && line_end > line_start && ptr[line_end - 1] == '\r') {
                line_len--;
                is_crlf = true;
            }
            
            if (line_len > 0) {
                // АЛЬТЕРНАТИВНЫЙ ПОДХОД: используем std::string для гарантии границ
                std::size_t actual_line_len = std::min(line_len, len - line_start);
                std::string line_str(ptr + line_start, actual_line_len);
                
                
                
                std::string_view line(line_str);
                ParsedRow row = parse_line_quotes_no_escape(line);
                
                // Вычисляем bytes_processed
                std::size_t bytes_processed = line_len;
                if (is_crlf) {
                    // Это \r\n - пропускаем оба символа
                    bytes_processed += 2;
                    line_start = line_end + 1;  // line_end указывает на \n, пропускаем его
                } else if (line_end < len && ptr[line_end] == '\r' && line_end + 1 < len && ptr[line_end + 1] == '\n') {
                    bytes_processed += 2;
                    line_start = line_end + 2;
                } else if (line_end < len && (ptr[line_end] == '\n' || ptr[line_end] == '\r')) {
                    bytes_processed += 1;
                    line_start = line_end + 1;
                } else {
                    // line_end == len - конец данных, цикл завершится
                    line_start = line_end;
                }
                
                row.bytes_processed = bytes_processed;
                results.push_back(std::move(row));
            } else {
                // Пустая строка
                ParsedRow row;
                row.fields.push_back("");
                row.success = true;
                // КРИТИЧЕСКАЯ ОПТИМИЗАЦИЯ: гарантируем, что line_start всегда увеличивается
                if (line_end < len && ptr[line_end] == '\r' && line_end + 1 < len && ptr[line_end + 1] == '\n') {
                    line_start = line_end + 2;
                    row.bytes_processed = 2;
                } else if (line_end < len && (ptr[line_end] == '\n' || ptr[line_end] == '\r')) {
                    line_start = line_end + 1;
                    row.bytes_processed = 1;
                } else {
                    // line_end == len - конец данных, цикл завершится
                    line_start = line_end;
                    row.bytes_processed = 0;
                }
                results.push_back(std::move(row));
            }
        }
        
        return results;
    }
    
    // Есть экранированные кавычки - используем полный путь
    
    
    while (pos < len) {
        // Ищем конец строки, учитывая кавычки
        // Если мы внутри кавычек, newline не является концом строки
        std::size_t line_end = std::string_view::npos;
        
        // КРИТИЧЕСКАЯ ОПТИМИЗАЦИЯ: оптимизированный поиск конца строки с учетом кавычек
        // Используем SIMD для ускорения поиска кавычек и newline
        std::size_t i = pos;
        const char* search_ptr = ptr + i;
        std::size_t remaining = len - i;
        
        // КРИТИЧЕСКАЯ ОПТИМИЗАЦИЯ для больших файлов: используем SIMD для больших блоков
        // Для очень больших чанков (>512KB) используем более агрессивный SIMD
        if (remaining >= 16) {
            std::string_view search_view(search_ptr, remaining);
            
            if (!in_quotes) {
                // Вне кавычек - ищем newline и кавычку одновременно
                // КРИТИЧЕСКАЯ ОПТИМИЗАЦИЯ: используем find_any_char_simd для поиска обоих символов за один проход
                std::size_t quote_or_newline = simd::find_any_char_simd(search_view, config_.quote, '\n', 0);
                std::size_t cr_pos = std::string_view::npos;
                // Для больших блоков (>64 байт) ищем \r отдельно, для маленьких - объединяем проверки
                if (remaining >= 64) {
                    cr_pos = simd::find_char_simd(search_view, '\r', 0);
                } else {
                    // Для маленьких блоков проверяем \r вручную при необходимости
                    if (quote_or_newline != std::string_view::npos) {
                        for (std::size_t k = 0; k < remaining && k < quote_or_newline; ++k) {
                            if (search_ptr[k] == '\r') {
                                cr_pos = k;
                                break;
                            }
                        }
                    } else {
                        // quote_or_newline не найден - проверяем весь блок
                        for (std::size_t k = 0; k < remaining; ++k) {
                            if (search_ptr[k] == '\r') {
                                cr_pos = k;
                                break;
                            }
                        }
                    }
                }
                
                // Находим ближайший из найденных символов
                std::size_t first_special = quote_or_newline;
                if (cr_pos != std::string_view::npos && (first_special == std::string_view::npos || cr_pos < first_special)) {
                    first_special = cr_pos;
                }
                
                if (first_special != std::string_view::npos) {
                    // Проверяем, что это за символ
                    char found_char = search_ptr[first_special];
                    if (found_char == '\n' || found_char == '\r') {
                        // Найден newline - это конец строки
                        line_end = i + first_special;
                    } else if (found_char == config_.quote) {
                        // Найдена кавычка - переходим в режим кавычек и продолжаем поиск
                        in_quotes = true;
                        i += first_special + 1;
                        // Продолжаем поиск с новой позиции
                        search_ptr = ptr + i;
                        remaining = len - i;
                        if (remaining >= 16) {
                            search_view = std::string_view(search_ptr, remaining);
                            std::size_t quote_pos = simd::find_char_simd(search_view, config_.quote, 0);
                            if (quote_pos != std::string_view::npos) {
                                // Проверяем, не экранированная ли это кавычка
                                if (i + quote_pos + 1 < len && ptr[i + quote_pos + 1] == config_.quote) {
                                    // Экранированная кавычка "" - пропускаем обе
                                    i += quote_pos + 2;
                                } else {
                                    // Обычная закрывающая кавычка
                                    in_quotes = false;
                                    i += quote_pos + 1;
                                }
                                // Продолжаем поиск newline - обновляем pos и продолжаем
                                pos = i;
                                continue;
                            }
                        }
                        // Обновляем pos для следующей итерации
                        pos = i;
                    }
                } else {
                    // Не нашли ни newline, ни кавычку - пропускаем большой блок
                    i += (remaining & ~15); // Выравниваем на 16 байт
                    pos = i; // Обновляем pos перед continue
                    continue;
                }
            } else {
                // Внутри кавычек - ищем только кавычку
                std::size_t quote_pos = simd::find_char_simd(search_view, config_.quote, 0);
                
                if (quote_pos != std::string_view::npos) {
                    // Проверяем, не экранированная ли это кавычка
                    if (i + quote_pos + 1 < len && ptr[i + quote_pos + 1] == config_.quote) {
                        // Экранированная кавычка "" - пропускаем обе
                        i += quote_pos + 2;
                        pos = i; // Обновляем pos перед continue
                        continue;
                    } else {
                        // Обычная закрывающая кавычка
                        in_quotes = false;
                        i += quote_pos + 1;
                        pos = i; // Обновляем pos перед continue
                        continue;
                    }
                } else {
                    // Не нашли кавычку - пропускаем большой блок
                    i += (remaining & ~15);
                    pos = i; // Обновляем pos перед continue
                    continue;
                }
            }
        } else {
            // КРИТИЧЕСКАЯ ОПТИМИЗАЦИЯ: для маленьких блоков используем прямой поиск
            for (; i < len; ++i) {
                char c = ptr[i];
                
                if (c == config_.quote) {
                    // Обрабатываем кавычки
                    if (in_quotes && i + 1 < len && ptr[i + 1] == config_.quote) {
                        // Экранированная кавычка "" - пропускаем следующую кавычку
                        i++;
                        continue;
                    }
                    in_quotes = !in_quotes;
                } else if (!in_quotes && (c == '\n' || c == '\r')) {
                    // Нашли конец строки вне кавычек
                    line_end = i;
                    break;
                }
            }
        }
        
        if (line_end == std::string_view::npos) {
            // Не нашли конец строки (последняя строка или многострочное поле)
            // Если мы не внутри кавычек, это последняя строка
            if (!in_quotes) {
                line_end = len;
            } else {
                // Многострочное поле - не можем обработать в этом чанке
                // Возвращаем то, что уже распарсили
                break;
            }
        }
        
        // Нашли конец строки (или конец данных)
        // line_end указывает на позицию \n или \r (или на конец данных)
        // line_len не включает символы конца строки, так как line_end указывает на них
        std::size_t line_len = line_end - line_start;
        
        
        
        // КРИТИЧЕСКАЯ ИСПРАВЛЕНИЕ: для Windows \r\n нужно исключить \r из строки
        // Если line_end указывает на \n, а перед ним \r, то исключаем \r из line_len
        // ИЛИ если line_end указывает на \r, а следующий символ - \n, то это \r\n
        bool is_crlf = false;
        if (line_end < len && ptr[line_end] == '\r' && line_end + 1 < len && ptr[line_end + 1] == '\n') {
            // line_end указывает на \r, следующий символ - \n
            // Это \r\n - исключаем \r из line_len
            line_len--;
            is_crlf = true;
        } else if (line_len > 0 && line_end > line_start && ptr[line_end - 1] == '\r') {
            // line_end указывает на \n, перед ним \r
            // Это \r\n - исключаем \r из line_len
            line_len--;
            is_crlf = true;
        }
        
        
        
        // КРИТИЧЕСКАЯ ОПТИМИЗАЦИЯ для больших файлов: отладочный код удален
        
        // Определяем, сколько символов нужно пропустить после line_end
        std::size_t next_pos = line_end;
        if (is_crlf) {
            // Это \r\n - пропускаем оба символа
            next_pos = line_end + 1;  // line_end указывает на \n, пропускаем его
        } else if (line_end < len) {
            if (ptr[line_end] == '\r' && line_end + 1 < len && ptr[line_end + 1] == '\n') {
                // \r\n
                next_pos = line_end + 2;
            } else if (ptr[line_end] == '\n' || ptr[line_end] == '\r') {
                // \n или \r
                next_pos = line_end + 1;
            }
        }
        
        // Парсим строку (line_len уже не включает символы конца строки)
        if (line_len > 0) {
            // parse_len = line_len (line_end указывает на позицию \n или \r, поэтому line_len не включает их)
            std::size_t parse_len = line_len;
            
            if (parse_len > 0) {
                // АЛЬТЕРНАТИВНЫЙ ПОДХОД: используем std::string для гарантии границ
                // КРИТИЧЕСКАЯ ИСПРАВЛЕНИЕ: parse_len уже был уменьшен на 1 для \r\n (если is_crlf == true)
                // Поэтому actual_parse_len должен быть равен parse_len, а не min(parse_len, len - line_start)
                // Потому что len - line_start может включать следующие строки для многострочного буфера
                std::size_t actual_parse_len = parse_len;
                
                std::string line_str(ptr + line_start, actual_parse_len);
                
                std::string_view line(line_str);
                
                
                
                // КРИТИЧЕСКАЯ ОПТИМИЗАЦИЯ для больших файлов: отладочный код удален
                // КРИТИЧЕСКАЯ ОПТИМИЗАЦИЯ: используем знание о чанке для пропуска проверок
                // Если в чанке есть кавычки, все строки тоже имеют кавычки (для упрощения)
                // Если в чанке нет кавычек, все строки тоже не имеют кавычек
                ParsedRow row;
                
                // КРИТИЧЕСКАЯ ОПТИМИЗАЦИЯ: резервируем память заранее на основе статистики
                row.fields.reserve(avg_fields_per_row);
                
                // Выбираем оптимальный путь парсинга на основе знания о чанке
                if (!chunk_has_quotes) {
                    // Нет кавычек в чанке - используем быстрый путь
                    row = parse_line_no_quotes(line);
                } else {
                    // Есть кавычки в чанке
                    // КРИТИЧЕСКАЯ ОПТИМИЗАЦИЯ: проверяем, есть ли кавычки в текущей строке
                    // Если в строке нет кавычек, используем быстрый путь parse_line_no_quotes
                    bool line_has_quotes = false;
                    if (parse_len >= 16) {
                        std::size_t quote_pos = simd::find_char_simd(line, config_.quote, 0);
                        line_has_quotes = (quote_pos != std::string_view::npos);
                    } else {
                        for (std::size_t k = 0; k < parse_len; ++k) {
                            if (ptr[line_start + k] == config_.quote) {
                                line_has_quotes = true;
                                break;
                            }
                        }
                    }
                    
                    
                    
                    if (!line_has_quotes) {
                        // В строке нет кавычек - используем быстрый путь
                        row = parse_line_no_quotes(line);
                    } else {
                        // В строке есть кавычки
                        // Если мы уже знаем, что в чанке нет экранированных кавычек,
                        // используем быстрый путь без проверки
                        if (!chunk_has_escaped_quotes) {
                            row = parse_line_quotes_no_escape(line);
                        } else {
                            // Есть экранированные кавычки - проверяем строку
                            // КРИТИЧЕСКАЯ ОПТИМИЗАЦИЯ: используем быструю проверку для маленьких строк
                            bool has_escaped = false;
                            if (parse_len >= 16) {
                                has_escaped = has_escaped_quotes(line, config_.quote);
                            } else {
                                // Для маленьких строк используем быструю проверку
                                for (std::size_t k = 0; k < parse_len - 1; ++k) {
                                    if (ptr[line_start + k] == config_.quote && ptr[line_start + k + 1] == config_.quote) {
                                        has_escaped = true;
                                        break;
                                    }
                                }
                            }
                            
                            if (!has_escaped) {
                                row = parse_line_quotes_no_escape(line);
                            } else {
                                row = parse_line(line);
                            }
                        }
                    }
                }
                results.push_back(std::move(row));
            } else {
                // Пустая строка
                ParsedRow row;
                row.fields.push_back("");
                results.push_back(std::move(row));
            }
        } else {
            // Пустая строка
            ParsedRow row;
            row.fields.push_back("");
            results.push_back(std::move(row));
        }
        
        // Переходим к следующей строке
        pos = next_pos;
        line_start = next_pos;  // Исправлено: используем next_pos напрямую
        in_quotes = false; // Сбрасываем состояние кавычек для новой строки
    }
    
    return results;
}

std::vector<ParsedRow> CSVParser::parse_all(std::string_view data) {
    return parse_chunk(data);
}

void CSVParser::set_config(const ParserConfig& config) {
    config_ = config;
}

std::string CSVParser::unescape_field(std::string_view field) {
    std::string result;
    result.reserve(field.length());
    
    bool in_quotes = false;
    for (std::size_t i = 0; i < field.length(); ++i) {
        char c = field[i];
        if (c == config_.quote) {
            if (i + 1 < field.length() && field[i + 1] == config_.quote) {
                result += config_.quote;
                i++; // Пропускаем следующую кавычку
            } else {
                in_quotes = !in_quotes;
            }
        } else {
            result += c;
        }
    }
    
    return result;
}

bool CSVParser::is_whitespace(char c) const {
    return c == ' ' || c == '\t';
}

void CSVParser::trim_whitespace(std::string& str) const {
    if (str.empty()) {
        return;
    }
    
    // Быстрая проверка: если первый и последний символ не пробелы, не нужно обрезать
    if (str.length() > 0 && !is_whitespace(str.front()) && !is_whitespace(str.back())) {
        // Проверяем, нет ли пробелов внутри (редкий случай)
        bool has_leading_trailing_ws = false;
        for (std::size_t i = 0; i < str.length(); ++i) {
            if (is_whitespace(str[i]) && (i == 0 || i == str.length() - 1)) {
                has_leading_trailing_ws = true;
                break;
            }
        }
        if (!has_leading_trailing_ws) {
            return; // Нет пробелов в начале/конце - не нужно обрезать
        }
    }
    
    // Оптимизированная версия: используем find_first_not_of и find_last_not_of
    const char* whitespace_chars = " \t";
    
    // Находим первый не-whitespace символ
    std::size_t first = str.find_first_not_of(whitespace_chars);
    if (first == std::string::npos) {
        // Вся строка состоит из пробелов
        str.clear();
        return;
    }
    
    // Находим последний не-whitespace символ
    std::size_t last = str.find_last_not_of(whitespace_chars);
    
    // Используем substr один раз вместо множественных erase/pop_back
    if (first > 0 || last < str.length() - 1) {
        str = str.substr(first, last - first + 1);
    }
}

std::size_t CSVParser::find_delimiter_simd(std::string_view data, std::size_t start_pos) const {
    return simd::find_char_simd(data, config_.delimiter, start_pos);
}

std::size_t CSVParser::find_quote_simd(std::string_view data, std::size_t start_pos) const {
    return simd::find_char_simd(data, config_.quote, start_pos);
}

bool CSVParser::has_unclosed_quotes(std::string_view data, char quote_char) {
    // Оптимизированная проверка: считаем кавычки, учитывая экранированные кавычки ""
    // Экранированные кавычки "" не влияют на состояние открыто/закрыто
    const char* ptr = data.data();
    std::size_t len = data.length();
    
    // Для файлов без кавычек сразу возвращаем false
    if (len == 0) {
        return false;
    }
    
    // Быстрая проверка: если файл очень большой, можем использовать упрощенную проверку
    // Но для точности используем полную проверку для всех размеров
    std::size_t count = 0;
    
    if (len >= 64) {
        // Используем SIMD для поиска кавычек, затем проверяем экранированные
        // Оптимизация: обрабатываем большие блоки за раз
        std::size_t pos = 0;
        const std::size_t block_size = 1024; // Обрабатываем по 1KB за раз для лучшей локальности
        
        while (pos < len) {
            std::size_t search_len = std::min(block_size, len - pos);
            std::string_view search_view(ptr + pos, search_len);
            
            std::size_t found = simd::find_char_simd(search_view, quote_char, 0);
            if (found == std::string_view::npos) {
                // Не нашли кавычку в этом блоке - переходим к следующему
                pos += search_len;
                continue;
            }
            
            // Нашли кавычку - проверяем, не экранированная ли она
            std::size_t absolute_pos = pos + found;
            if (absolute_pos + 1 < len && ptr[absolute_pos + 1] == quote_char) {
                // Экранированная кавычка "" - пропускаем обе (не считаем)
                pos = absolute_pos + 2;
            } else {
                // Обычная кавычка - считаем её
                count++;
                pos = absolute_pos + 1;
            }
        }
    } else {
        // Для маленьких данных используем простой цикл
        for (std::size_t i = 0; i < len; ++i) {
            if (ptr[i] == quote_char) {
                // Быстрая проверка экранированной кавычки
                if (i + 1 < len && ptr[i + 1] == quote_char) {
                    // Экранированная кавычка "" - пропускаем следующую
                    i++;
                } else {
                    // Обычная кавычка - считаем её
                    count++;
                }
            }
        }
    }
    
    // Если нечетное количество обычных кавычек - есть незакрытая кавычка
    return (count % 2) != 0;
}

std::size_t CSVParser::find_last_newline(std::string_view data) {
    const char* ptr = data.data();
    std::size_t len = data.length();
    
    // Ищем с конца для эффективности
    for (std::size_t i = len; i > 0; --i) {
        if (ptr[i - 1] == '\n') {
            return i - 1;
        }
    }
    
    // Возвращаем максимальное значение size_t, которое в Python будет преобразовано
    // pybind11 автоматически обработает это как -1 для Python
    return static_cast<std::size_t>(-1);
}

} // namespace fastcsv

