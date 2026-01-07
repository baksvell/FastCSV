"""Тесты для функций работы с диалектами"""

import pytest
import fastcsv
import io


def test_register_dialect():
    """Тест регистрации диалекта"""
    fastcsv.register_dialect('testdialect', delimiter='|', quotechar="'")
    
    assert 'testdialect' in fastcsv.list_dialects()
    
    dialect = fastcsv.get_dialect('testdialect')
    assert dialect.delimiter == '|'
    assert dialect.quotechar == "'"


def test_get_dialect():
    """Тест получения диалекта"""
    # Стандартные диалекты должны быть доступны
    excel = fastcsv.get_dialect('excel')
    assert excel.delimiter == ','
    assert excel.quotechar == '"'
    
    excel_tab = fastcsv.get_dialect('excel-tab')
    assert excel_tab.delimiter == '\t'
    
    unix = fastcsv.get_dialect('unix')
    assert unix.lineterminator == '\n'


def test_get_dialect_not_found():
    """Тест получения несуществующего диалекта"""
    with pytest.raises(KeyError):
        fastcsv.get_dialect('nonexistent')


def test_unregister_dialect():
    """Тест удаления диалекта"""
    fastcsv.register_dialect('tempdialect', delimiter=';')
    assert 'tempdialect' in fastcsv.list_dialects()
    
    fastcsv.unregister_dialect('tempdialect')
    assert 'tempdialect' not in fastcsv.list_dialects()
    
    with pytest.raises(KeyError):
        fastcsv.get_dialect('tempdialect')


def test_unregister_dialect_not_found():
    """Тест удаления несуществующего диалекта"""
    with pytest.raises(KeyError):
        fastcsv.unregister_dialect('nonexistent')


def test_list_dialects():
    """Тест списка диалектов"""
    dialects = fastcsv.list_dialects()
    
    # Должны быть стандартные диалекты
    assert 'excel' in dialects
    assert 'excel-tab' in dialects
    assert 'unix' in dialects
    
    # Это должен быть список
    assert isinstance(dialects, list)


def test_use_registered_dialect():
    """Тест использования зарегистрированного диалекта"""
    fastcsv.register_dialect('pipe', delimiter='|')
    
    data = "name|age|city\nJohn|30|NYC"
    f = io.StringIO(data)
    
    reader = fastcsv.reader(f, dialect='pipe')
    rows = list(reader)
    
    assert len(rows) == 2
    assert rows[0] == ["name", "age", "city"]
    assert rows[1] == ["John", "30", "NYC"]


def test_dialect_object():
    """Тест создания и использования объекта Dialect"""
    dialect = fastcsv.Dialect(delimiter=';', quotechar="'")
    
    assert dialect.delimiter == ';'
    assert dialect.quotechar == "'"
    
    # Можно использовать напрямую
    data = "name;age\nJohn;30"
    f = io.StringIO(data)
    reader = fastcsv.reader(f, dialect=dialect)
    rows = list(reader)
    
    assert rows[0] == ["name", "age"]
    assert rows[1] == ["John", "30"]


def test_register_dialect_with_object():
    """Тест регистрации диалекта с объектом"""
    dialect = fastcsv.Dialect(delimiter=':', quotechar='|')
    fastcsv.register_dialect('colondialect', dialect)
    
    retrieved = fastcsv.get_dialect('colondialect')
    assert retrieved.delimiter == ':'
    assert retrieved.quotechar == '|'


def test_register_dialect_override():
    """Тест переопределения диалекта"""
    fastcsv.register_dialect('mydialect', delimiter='|')
    assert fastcsv.get_dialect('mydialect').delimiter == '|'
    
    fastcsv.register_dialect('mydialect', delimiter=';')
    assert fastcsv.get_dialect('mydialect').delimiter == ';'


def test_dialect_repr():
    """Тест строкового представления диалекта"""
    dialect = fastcsv.Dialect(delimiter='|', quotechar="'")
    repr_str = repr(dialect)
    
    assert 'delimiter' in repr_str
    assert 'quotechar' in repr_str








