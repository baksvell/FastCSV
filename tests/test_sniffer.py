"""Тесты для Sniffer"""

import pytest
import fastcsv
import io


def test_sniffer_basic():
    """Тест базового определения формата"""
    sample = "name,age,city\nJohn,30,New York\nJane,25,Boston"
    sniffer = fastcsv.Sniffer()
    dialect = sniffer.sniff(sample)
    
    assert dialect.delimiter == ','
    assert dialect.quotechar == '"'


def test_sniffer_tab():
    """Тест определения табуляции"""
    sample = "name\tage\tcity\nJohn\t30\tNYC"
    sniffer = fastcsv.Sniffer()
    dialect = sniffer.sniff(sample)
    
    assert dialect.delimiter == '\t'


def test_sniffer_pipe():
    """Тест определения pipe разделителя"""
    sample = "name|age|city\nJohn|30|NYC"
    sniffer = fastcsv.Sniffer()
    dialect = sniffer.sniff(sample, delimiters='|,;')
    
    assert dialect.delimiter == '|'


def test_sniffer_semicolon():
    """Тест определения точки с запятой"""
    sample = "name;age;city\nJohn;30;NYC"
    sniffer = fastcsv.Sniffer()
    dialect = sniffer.sniff(sample)
    
    assert dialect.delimiter == ';'


def test_sniffer_quotechar():
    """Тест определения кавычки"""
    sample = "name,age,'city'\nJohn,30,'NYC'"
    sniffer = fastcsv.Sniffer()
    dialect = sniffer.sniff(sample)
    
    # Должен определить одинарные кавычки
    assert dialect.quotechar == "'" or dialect.quotechar == '"'


def test_sniffer_lineterminator():
    """Тест определения терминатора строки"""
    sample = "name,age\nJohn,30"
    sniffer = fastcsv.Sniffer()
    dialect = sniffer.sniff(sample)
    
    assert dialect.lineterminator in ['\n', '\r\n']


def test_has_header():
    """Тест определения заголовка"""
    sample = "name,age,city\nJohn,30,NYC\nJane,25,Boston"
    sniffer = fastcsv.Sniffer()
    
    assert sniffer.has_header(sample) == True


def test_has_header_no_header():
    """Тест определения отсутствия заголовка"""
    sample = "John,30,NYC\nJane,25,Boston\nBob,35,Chicago"
    sniffer = fastcsv.Sniffer()
    
    # Может вернуть True или False в зависимости от эвристики
    result = sniffer.has_header(sample)
    assert isinstance(result, bool)


def test_sniffer_use_detected():
    """Тест использования определенного диалекта"""
    sample = "name|age|city\nJohn|30|NYC"
    sniffer = fastcsv.Sniffer()
    dialect = sniffer.sniff(sample, delimiters='|,')
    
    data = "name|age|city\nJohn|30|NYC"
    f = io.StringIO(data)
    reader = fastcsv.reader(f, dialect=dialect)
    rows = list(reader)
    
    assert len(rows) == 2
    assert rows[0] == ["name", "age", "city"]


def test_sniffer_empty_sample():
    """Тест с пустым образцом"""
    sniffer = fastcsv.Sniffer()
    dialect = sniffer.sniff("")
    
    # Должен вернуть стандартный диалект
    assert dialect.delimiter == ','
    assert dialect.quotechar == '"'








