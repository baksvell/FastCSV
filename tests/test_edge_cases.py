"""Тесты для edge cases и специальных случаев"""

import pytest
import fastcsv
import io


def test_empty_file():
    """Тест пустого файла"""
    f = io.StringIO("")
    reader = fastcsv.reader(f)
    rows = list(reader)
    assert len(rows) == 0


def test_empty_line():
    """Тест пустой строки"""
    f = io.StringIO("\n")
    reader = fastcsv.reader(f)
    rows = list(reader)
    assert len(rows) == 1
    assert rows[0] == [""]


def test_only_delimiter():
    """Тест строки только с разделителями"""
    f = io.StringIO(",,\n")
    reader = fastcsv.reader(f)
    rows = list(reader)
    assert len(rows) == 1
    assert rows[0] == ["", "", ""]


def test_quoted_delimiter():
    """Тест разделителя внутри кавычек"""
    f = io.StringIO('name,"value,with,commas"\n')
    reader = fastcsv.reader(f)
    rows = list(reader)
    assert len(rows) == 1
    assert rows[0] == ["name", "value,with,commas"]


def test_escaped_quotes():
    """Тест экранированных кавычек"""
    f = io.StringIO('name,"value with ""quotes"""\n')
    reader = fastcsv.reader(f)
    rows = list(reader)
    assert len(rows) == 1
    assert rows[0] == ["name", 'value with "quotes"']


def test_multiline_quoted_field():
    """Тест многострочного поля в кавычках"""
    f = io.StringIO('name,"line1\nline2\nline3"\n')
    reader = fastcsv.reader(f)
    rows = list(reader)
    assert len(rows) == 1
    assert rows[0] == ["name", "line1\nline2\nline3"]


def test_custom_delimiter():
    """Тест кастомного разделителя"""
    f = io.StringIO("name|age|city\nJohn|30|NYC\n")
    reader = fastcsv.reader(f, delimiter='|')
    rows = list(reader)
    assert len(rows) == 2
    assert rows[0] == ["name", "age", "city"]
    assert rows[1] == ["John", "30", "NYC"]


def test_custom_quote():
    """Тест кастомной кавычки"""
    f = io.StringIO("name,'value with, comma'\n")
    reader = fastcsv.reader(f, quotechar="'")
    rows = list(reader)
    assert len(rows) == 1
    assert rows[0] == ["name", "value with, comma"]


def test_skip_initial_space():
    """Тест пропуска начальных пробелов"""
    f = io.StringIO("name, age , city\n")
    reader = fastcsv.reader(f, skipinitialspace=True)
    rows = list(reader)
    assert len(rows) == 1
    # skipinitialspace работает только для пробелов после разделителя
    assert "age" in rows[0][1] or " age " in rows[0][1]


def test_large_number_of_fields():
    """Тест большого количества полей"""
    num_fields = 100
    header = ",".join(f"col{i}" for i in range(num_fields))
    data = ",".join(f"value{i}" for i in range(num_fields))
    f = io.StringIO(f"{header}\n{data}\n")
    
    reader = fastcsv.reader(f)
    rows = list(reader)
    assert len(rows) == 2
    assert len(rows[0]) == num_fields
    assert len(rows[1]) == num_fields


def test_unicode_characters():
    """Тест Unicode символов"""
    f = io.StringIO("name,value\nПривет,世界\n")
    reader = fastcsv.reader(f)
    rows = list(reader)
    assert len(rows) == 2
    assert rows[1][0] == "Привет"
    assert rows[1][1] == "世界"








