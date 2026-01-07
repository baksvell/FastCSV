"""
Тесты для mmap функциональности FastCSV
"""

import pytest
import fastcsv
import tempfile
import os


def test_mmap_reader_basic():
    """Тест базового использования mmap_reader"""
    # Создаем временный CSV файл
    with tempfile.NamedTemporaryFile(mode='w', suffix='.csv', delete=False) as f:
        f.write("name,age,city\n")
        f.write("John,30,New York\n")
        f.write("Jane,25,Boston\n")
        temp_path = f.name
    
    try:
        # Тестируем mmap_reader
        reader = fastcsv.mmap_reader(temp_path)
        rows = list(reader)
        
        assert len(rows) == 3
        assert rows[0] == ["name", "age", "city"]
        assert rows[1] == ["John", "30", "New York"]
        assert rows[2] == ["Jane", "25", "Boston"]
        
        reader.close()
    finally:
        # Убеждаемся, что файл закрыт перед удалением
        try:
            if 'reader' in locals():
                reader.close()
        except:
            pass
        # Небольшая задержка для Windows, чтобы файл был освобожден
        import time
        time.sleep(0.01)
        try:
            os.unlink(temp_path)
        except PermissionError:
            # Если файл все еще заблокирован, пробуем еще раз после небольшой задержки
            time.sleep(0.1)
            os.unlink(temp_path)


def test_mmap_reader_context_manager():
    """Тест mmap_reader с context manager"""
    with tempfile.NamedTemporaryFile(mode='w', suffix='.csv', delete=False) as f:
        f.write("a,b,c\n")
        f.write("1,2,3\n")
        temp_path = f.name
    
    try:
        with fastcsv.mmap_reader(temp_path) as reader:
            rows = list(reader)
            assert len(rows) == 2
            assert rows[0] == ["a", "b", "c"]
            assert rows[1] == ["1", "2", "3"]
    finally:
        os.unlink(temp_path)


def test_mmap_reader_custom_delimiter():
    """Тест mmap_reader с кастомным разделителем"""
    with tempfile.NamedTemporaryFile(mode='w', suffix='.csv', delete=False) as f:
        f.write("name|age|city\n")
        f.write("John|30|New York\n")
        temp_path = f.name
    
    try:
        reader = fastcsv.mmap_reader(temp_path, delimiter='|')
        rows = list(reader)
        
        assert len(rows) == 2
        assert rows[0] == ["name", "age", "city"]
        assert rows[1] == ["John", "30", "New York"]
        
        reader.close()
    finally:
        os.unlink(temp_path)


def test_mmap_reader_quoted_fields():
    """Тест mmap_reader с полями в кавычках"""
    with tempfile.NamedTemporaryFile(mode='w', suffix='.csv', delete=False) as f:
        f.write('name,description\n')
        f.write('John,"Hello, world"\n')
        f.write('Jane,"Multi\nline\nfield"\n')
        temp_path = f.name
    
    try:
        reader = fastcsv.mmap_reader(temp_path)
        rows = list(reader)
        # #region agent log
        import json
        try:
            with open(r"c:\Users\Admin\PycharmProjects\FastCSV\.cursor\debug.log", 'a', encoding='utf-8') as f:
                f.write(json.dumps({"sessionId": "debug-session", "runId": "run1", "hypothesisId": "B", "location": "tests/test_mmap.py:95", "message": "Test results", "data": {"num_rows": len(rows), "rows": rows}, "timestamp": __import__('time').time() * 1000}) + "\n")
        except: pass
        # #endregion

        assert len(rows) == 3
        assert rows[0] == ["name", "description"]
        assert rows[1] == ["John", "Hello, world"]
        # Нормализуем \r\n в \n для сравнения (Windows использует \r\n)
        field_value = rows[2][1].replace('\r\n', '\n').replace('\r', '\n')
        assert rows[2][0] == "Jane"
        assert field_value == "Multi\nline\nfield"
        
        reader.close()
    finally:
        # Используем context manager для гарантированного закрытия
        try:
            if 'reader' in locals():
                reader.close()
        except:
            pass
        # Даем время на закрытие файла (особенно на Windows)
        import time
        time.sleep(0.1)
        try:
            os.unlink(temp_path)
        except (PermissionError, FileNotFoundError):
            # На Windows файл может быть еще заблокирован, игнорируем
            pass


def test_mmap_dict_reader():
    """Тест mmap_DictReader"""
    with tempfile.NamedTemporaryFile(mode='w', suffix='.csv', delete=False) as f:
        f.write("name,age,city\n")
        f.write("John,30,New York\n")
        f.write("Jane,25,Boston\n")
        temp_path = f.name
    
    try:
        reader = fastcsv.mmap_DictReader(temp_path)
        rows = list(reader)
        
        assert len(rows) == 2
        assert rows[0] == {"name": "John", "age": "30", "city": "New York"}
        assert rows[1] == {"name": "Jane", "age": "25", "city": "Boston"}
        
        reader.close()
    finally:
        os.unlink(temp_path)


def test_mmap_dict_reader_context_manager():
    """Тест mmap_DictReader с context manager"""
    with tempfile.NamedTemporaryFile(mode='w', suffix='.csv', delete=False) as f:
        f.write("a,b\n")
        f.write("1,2\n")
        temp_path = f.name
    
    try:
        with fastcsv.mmap_DictReader(temp_path) as reader:
            rows = list(reader)
            assert len(rows) == 1
            assert rows[0] == {"a": "1", "b": "2"}
    finally:
        os.unlink(temp_path)


def test_mmap_dict_reader_custom_fieldnames():
    """Тест mmap_DictReader с кастомными fieldnames"""
    with tempfile.NamedTemporaryFile(mode='w', suffix='.csv', delete=False) as f:
        f.write("1,2,3\n")
        f.write("4,5,6\n")
        temp_path = f.name
    
    try:
        reader = fastcsv.mmap_DictReader(temp_path, fieldnames=['x', 'y', 'z'])
        rows = list(reader)
        
        assert len(rows) == 2
        assert rows[0] == {"x": "1", "y": "2", "z": "3"}
        assert rows[1] == {"x": "4", "y": "5", "z": "6"}
        
        reader.close()
    finally:
        os.unlink(temp_path)


def test_mmap_reader_large_file():
    """Тест mmap_reader с большим файлом"""
    # Создаем файл с 1000 строками
    with tempfile.NamedTemporaryFile(mode='w', suffix='.csv', delete=False) as f:
        f.write("col1,col2,col3\n")
        for i in range(1000):
            f.write(f"val{i},val{i+1},val{i+2}\n")
        temp_path = f.name
    
    try:
        reader = fastcsv.mmap_reader(temp_path)
        rows = list(reader)
        
        assert len(rows) == 1001  # 1 заголовок + 1000 строк
        assert rows[0] == ["col1", "col2", "col3"]
        assert rows[1] == ["val0", "val1", "val2"]
        assert rows[1000] == ["val999", "val1000", "val1001"]
        
        reader.close()
    finally:
        os.unlink(temp_path)


def test_mmap_reader_dialect():
    """Тест mmap_reader с диалектом"""
    # Регистрируем диалект
    fastcsv.register_dialect('pipe', delimiter='|')
    
    with tempfile.NamedTemporaryFile(mode='w', suffix='.csv', delete=False) as f:
        f.write("name|age\n")
        f.write("John|30\n")
        temp_path = f.name
    
    try:
        reader = fastcsv.mmap_reader(temp_path, dialect='pipe')
        rows = list(reader)
        
        assert len(rows) == 2
        assert rows[0] == ["name", "age"]
        assert rows[1] == ["John", "30"]
        
        reader.close()
    finally:
        os.unlink(temp_path)
        fastcsv.unregister_dialect('pipe')

