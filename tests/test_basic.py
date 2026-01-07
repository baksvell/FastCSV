"""Базовые тесты для FastCSV"""

import pytest
import fastcsv
import io


def test_simple_parsing():
    """Тест простого парсинга CSV"""
    data = "name,age,city\nJohn,30,New York\nJane,25,Boston"
    f = io.StringIO(data)
    
    reader = fastcsv.reader(f)
    rows = list(reader)
    
    assert len(rows) == 3  # Заголовок + 2 строки данных
    assert rows[0] == ["name", "age", "city"]
    assert rows[1] == ["John", "30", "New York"]
    assert rows[2] == ["Jane", "25", "Boston"]


def test_quoted_fields():
    """Тест полей в кавычках"""
    data = 'name,description\nJohn,"A person, who likes coding"'
    f = io.StringIO(data)
    
    reader = fastcsv.reader(f)
    rows = list(reader)
    
    assert rows[1][1] == "A person, who likes coding"


def test_dict_reader():
    """Тест DictReader"""
    data = "name,age\nJohn,30\nJane,25"
    f = io.StringIO(data)
    
    reader = fastcsv.DictReader(f)
    rows = list(reader)
    
    assert len(rows) == 2
    assert rows[0]["name"] == "John"
    assert rows[0]["age"] == "30"
    assert rows[1]["name"] == "Jane"
    assert rows[1]["age"] == "25"


def test_writer():
    """Тест writer"""
    f = io.StringIO()
    writer = fastcsv.writer(f)
    
    writer.writerow(["name", "age"])
    writer.writerow(["John", "30"])
    
    result = f.getvalue()
    assert "name,age" in result
    assert "John,30" in result


def test_dict_writer():
    """Тест DictWriter"""
    f = io.StringIO()
    fieldnames = ["name", "age"]
    writer = fastcsv.DictWriter(f, fieldnames)
    
    writer.writeheader()
    writer.writerow({"name": "John", "age": "30"})
    
    result = f.getvalue()
    assert "name,age" in result
    assert "John,30" in result

