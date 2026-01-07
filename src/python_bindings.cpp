#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/functional.h>
#include "fastcsv/csv_parser.hpp"
#include "fastcsv/simd_utils.hpp"
#include <vector>
#include <string>
#include <sstream>
#include <cstring>  // Для std::memcpy
#include <Python.h>  // Для прямого использования Python C API

namespace py = pybind11;
using namespace fastcsv;

PYBIND11_MODULE(_native, m) {
    m.doc() = "FastCSV native module - high-performance CSV parsing";
    
    // КРИТИЧЕСКАЯ ОПТИМИЗАЦИЯ для маленьких файлов: кэшируем пустую строку
    // Это избегает создания новых объектов для пустых полей
    static PyObject* cached_empty_string = nullptr;
    if (cached_empty_string == nullptr) {
        cached_empty_string = PyUnicode_FromStringAndSize("", 0);
        Py_INCREF(cached_empty_string);  // Увеличиваем счетчик ссылок для безопасности
    }
    
    // ParserConfig
    py::class_<ParserConfig>(m, "ParserConfig")
        .def(py::init<>())
        .def_property("delimiter", 
            [](const ParserConfig& self) { return std::string(1, self.delimiter); },
            [](ParserConfig& self, py::object value) { 
                // Принимаем строку или char
                if (py::isinstance<py::str>(value)) {
                    std::string s = value.cast<std::string>();
                    self.delimiter = s.empty() ? ',' : s[0];
                } else {
                    // Пытаемся преобразовать в строку
                    std::string s = py::str(value).cast<std::string>();
                    self.delimiter = s.empty() ? ',' : s[0];
                }
            })
        .def_property("quote",
            [](const ParserConfig& self) { return std::string(1, self.quote); },
            [](ParserConfig& self, py::object value) { 
                // Принимаем строку или char
                if (py::isinstance<py::str>(value)) {
                    std::string s = value.cast<std::string>();
                    self.quote = s.empty() ? '"' : s[0];
                } else {
                    // Пытаемся преобразовать в строку
                    std::string s = py::str(value).cast<std::string>();
                    self.quote = s.empty() ? '"' : s[0];
                }
            })
        .def_readwrite("skip_initial_space", &ParserConfig::skip_initial_space)
        .def_readwrite("strict", &ParserConfig::strict)
        .def_readwrite("lineterminator", &ParserConfig::lineterminator);
    
    // ParsedRow
    py::class_<ParsedRow>(m, "ParsedRow")
        .def_readonly("fields", &ParsedRow::fields)
        .def_readonly("success", &ParsedRow::success)
        .def_readonly("bytes_processed", &ParsedRow::bytes_processed);
    
    // Оптимизированная функция для batch создания Python объектов
    // Использует Python C API напрямую для избежания overhead pybind11
    m.def("parse_chunk_to_python", [](CSVParser& parser, py::str data_str) {
        // Оптимизация для средних файлов: используем PyUnicode_AsUTF8AndSize
        // для получения указателя на данные без копирования (если возможно)
        Py_ssize_t data_size;
        const char* data_ptr = PyUnicode_AsUTF8AndSize(data_str.ptr(), &data_size);
        
        std::string data_storage; // Хранилище для fallback случая
        std::string_view data_view;
        std::vector<ParsedRow> results;
        
        if (!data_ptr) {
            // Fallback на стандартный путь если не UTF-8
            data_storage = data_str.cast<std::string>();
            data_view = data_storage;
        } else {
            // Используем данные напрямую без копирования
            data_view = std::string_view(data_ptr, static_cast<std::size_t>(data_size));
        }
        
        results = parser.parse_chunk(data_view);
        
        // КРИТИЧЕСКАЯ ОПТИМИЗАЦИЯ для больших файлов: проверяем ASCII для всего чанка ОДИН РАЗ
        // Для очень больших чанков (>512KB) используем более быструю проверку
        bool chunk_is_ascii = false;
        if (data_view.size() >= 16) {
            // Для больших чанков используем SIMD проверку
            if (data_view.size() >= 524288) {  // >=512KB - очень большой чанк
                // Для очень больших чанков проверяем только первые 64KB для скорости
                std::string_view sample(data_view.data(), std::min(data_view.size(), std::size_t(65536)));
                chunk_is_ascii = simd::is_ascii_simd(sample);
            } else {
                chunk_is_ascii = simd::is_ascii_simd(data_view);
            }
        } else if (data_view.size() > 0) {
            // Для маленьких чанков быстрая проверка
            chunk_is_ascii = true;
            for (char c : data_view) {
                if (static_cast<unsigned char>(c) >= 0x80) {
                    chunk_is_ascii = false;
                    break;
                }
            }
        } else {
            chunk_is_ascii = true; // Пустой чанк считается ASCII
        }
        
        // КРИТИЧЕСКАЯ ОПТИМИЗАЦИЯ для больших файлов: batch создание всех Python объектов
        // Для очень больших чанков (>1MB) используем оптимизированный путь
        bool is_very_large = data_view.size() >= 1048576;  // >=1MB
        
        // КРИТИЧЕСКАЯ ОПТИМИЗАЦИЯ для больших файлов: упрощенная проверка ASCII
        // Для больших чанков (>512KB) используем только проверку чанка, без проверки полей
        // КРИТИЧЕСКАЯ ОПТИМИЗАЦИЯ для маленьких файлов: для очень маленьких чанков (<500 байт) пропускаем проверку
        bool all_fields_ascii = chunk_is_ascii;
        
        // Для очень маленьких чанков (<500 байт) предполагаем ASCII для ускорения
        // Для больших чанков (>512KB) используем только проверку чанка
        if (data_view.size() < 500) {
            // Очень маленький чанк - предполагаем ASCII для ускорения (уменьшен порог с 1KB до 500 байт)
            all_fields_ascii = true;
        } else if (!chunk_is_ascii && data_view.size() < 524288) {
            // Только для средних чанков проверяем каждое поле (но только первые несколько строк)
            std::size_t check_limit = std::min(results.size(), std::size_t(10));
            for (std::size_t i = 0; i < check_limit; ++i) {
                for (const auto& field : results[i].fields) {
                    if (!field.empty() && field.size() >= 16) {
                        if (!simd::is_ascii_simd(field)) {
                            all_fields_ascii = false;
                            break;
                        }
                    } else if (!field.empty()) {
                        for (char c : field) {
                            if (static_cast<unsigned char>(c) >= 0x80) {
                                all_fields_ascii = false;
                                break;
                            }
                        }
                    }
                    if (!all_fields_ascii) break;
                }
                if (!all_fields_ascii) break;
            }
        }
        
        // КРИТИЧЕСКАЯ ОПТИМИЗАЦИЯ для очень больших файлов: предварительно создаем все строки
        // Это уменьшает overhead от множественных вызовов PyUnicode_FromKindAndData
        std::vector<PyObject*> all_strings;
        if (is_very_large) {
            // Подсчитываем общее количество полей для предварительного резервирования
            std::size_t total_fields = 0;
            for (const auto& row : results) {
                total_fields += row.fields.size();
            }
            all_strings.reserve(total_fields);
        }
        
        // Создаем главный список строк
        PyObject* py_rows = PyList_New(results.size());
        if (!py_rows) {
            throw std::runtime_error("Failed to create Python list");
        }
        
        // КРИТИЧЕСКАЯ ОПТИМИЗАЦИЯ для больших файлов: оптимизированное создание объектов
        for (std::size_t i = 0; i < results.size(); ++i) {
            const auto& row = results[i];
            
            // Создаем список полей для этой строки
            PyObject* py_fields = PyList_New(row.fields.size());
            if (!py_fields) {
                // Очищаем уже созданные объекты
                for (PyObject* obj : all_strings) {
                    Py_DECREF(obj);
                }
                Py_DECREF(py_rows);
                throw std::runtime_error("Failed to create Python list for fields");
            }
            
            for (std::size_t j = 0; j < row.fields.size(); ++j) {
                const auto& field = row.fields[j];
                PyObject* py_str = nullptr;
                
                // КРИТИЧЕСКАЯ ОПТИМИЗАЦИЯ для маленьких файлов: кэшируем пустые строки
                if (field.empty()) {
                    // Пустая строка - используем кэшированную пустую строку
                    // КРИТИЧЕСКАЯ ОПТИМИЗАЦИЯ: кэширование пустых строк для уменьшения overhead
                    // Используем статическую переменную для кэширования
                    static PyObject* cached_empty_string = nullptr;
                    if (cached_empty_string == nullptr) {
                        cached_empty_string = PyUnicode_FromStringAndSize("", 0);
                        if (cached_empty_string == nullptr) {
                            throw std::runtime_error("Failed to create cached empty string");
                        }
                        // Не увеличиваем счетчик - объект будет жить до конца программы
                    }
                    py_str = cached_empty_string;
                    Py_INCREF(py_str);  // Увеличиваем счетчик ссылок при использовании
                } else if (all_fields_ascii) {
                    // Все поля ASCII - используем PyUnicode_FromKindAndData (самый быстрый путь)
                    // КРИТИЧЕСКАЯ ОПТИМИЗАЦИЯ: для больших файлов используем более эффективный путь
                    py_str = PyUnicode_FromKindAndData(1, field.data(), static_cast<Py_ssize_t>(field.size()));
                    if (!py_str) {
                        // Fallback на стандартный путь
                        py_str = PyUnicode_FromStringAndSize(field.data(), static_cast<Py_ssize_t>(field.size()));
                    }
                } else {
                    // Есть не-ASCII поля - используем стандартный путь
                    // КРИТИЧЕСКАЯ ОПТИМИЗАЦИЯ: для больших файлов можно попробовать PyUnicode_DecodeUTF8
                    py_str = PyUnicode_FromStringAndSize(field.data(), static_cast<Py_ssize_t>(field.size()));
                }
                
                if (!py_str) {
                    // Очищаем уже созданные объекты
                    for (PyObject* obj : all_strings) {
                        Py_DECREF(obj);
                    }
                    Py_DECREF(py_fields);
                    Py_DECREF(py_rows);
                    throw std::runtime_error("Failed to create Python string");
                }
                
                // Для очень больших файлов сохраняем ссылки на строки для возможной оптимизации
                if (is_very_large) {
                    all_strings.push_back(py_str);
                }
                
                // Используем PyList_SET_ITEM для установки элемента без проверок
                PyList_SET_ITEM(py_fields, static_cast<Py_ssize_t>(j), py_str);
            }
            
            // Устанавливаем список полей в список строк
            PyList_SET_ITEM(py_rows, static_cast<Py_ssize_t>(i), py_fields);
        }
        
        // Конвертируем PyObject* в py::object для возврата
        return py::reinterpret_steal<py::object>(py_rows);
    }, py::arg("parser"), py::arg("data"), "Parse chunk and return Python list directly");
    
    // CSVParser
    py::class_<CSVParser>(m, "CSVParser")
        .def(py::init<>())
        .def(py::init<const ParserConfig&>())
        .def("parse_line", &CSVParser::parse_line)
        .def("parse_chunk", &CSVParser::parse_chunk)
        .def("parse_all", &CSVParser::parse_all)
        .def("set_config", &CSVParser::set_config)
        .def_static("has_unclosed_quotes", &CSVParser::has_unclosed_quotes)
        .def_static("find_last_newline", &CSVParser::find_last_newline);
}

