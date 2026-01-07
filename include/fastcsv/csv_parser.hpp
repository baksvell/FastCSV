#pragma once

#include <vector>
#include <string>
#include <string_view>
#include <memory>
#include <cstddef>

namespace fastcsv {

// Конфигурация парсера
struct ParserConfig {
    char delimiter = ',';
    char quote = '"';
    bool skip_initial_space = false;
    bool strict = false;
    std::string lineterminator = "\r\n";
};

// Результат парсинга строки
struct ParsedRow {
    std::vector<std::string> fields;
    bool success = true;
    std::size_t bytes_processed = 0;
};

// Основной класс парсера CSV
class CSVParser {
public:
    explicit CSVParser(const ParserConfig& config = ParserConfig());
    
    // Парсинг одной строки
    ParsedRow parse_line(std::string_view line);
    
    // Быстрый парсинг строки без кавычек (оптимизированный путь)
    ParsedRow parse_line_no_quotes(std::string_view line);
    
    // Быстрый парсинг строки с кавычками, но без экранированных кавычек
    ParsedRow parse_line_quotes_no_escape(std::string_view line);
    
    // Парсинг блока данных (для streaming)
    std::vector<ParsedRow> parse_chunk(std::string_view data);
    
    // Парсинг всего файла
    std::vector<ParsedRow> parse_all(std::string_view data);
    
    // Обновление конфигурации
    void set_config(const ParserConfig& config);
    
    // Вспомогательные функции для оптимизации batch processing
    // Проверяет, есть ли нечетное количество кавычек (многострочное поле)
    static bool has_unclosed_quotes(std::string_view data, char quote_char);
    
    // Находит последний newline в данных
    static std::size_t find_last_newline(std::string_view data);
    
private:
    ParserConfig config_;
    
    // Вспомогательные методы
    std::string unescape_field(std::string_view field);
    bool is_whitespace(char c) const;
    void trim_whitespace(std::string& str) const;
    
    // SIMD-оптимизированные методы
    std::size_t find_delimiter_simd(std::string_view data, std::size_t start_pos) const;
    std::size_t find_quote_simd(std::string_view data, std::size_t start_pos) const;
};

} // namespace fastcsv

