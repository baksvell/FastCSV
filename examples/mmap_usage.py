"""
Пример использования mmap для работы с большими CSV файлами

mmap (memory-mapped files) позволяет эффективно работать с файлами,
которые не помещаются в оперативную память, отображая их напрямую
в адресное пространство процесса.
"""

import fastcsv
import tempfile
import os


def example_basic_mmap():
    """Базовый пример использования mmap_reader"""
    print("=== Базовый пример mmap_reader ===")
    
    # Создаем временный CSV файл
    with tempfile.NamedTemporaryFile(mode='w', suffix='.csv', delete=False) as f:
        f.write("name,age,city\n")
        f.write("John,30,New York\n")
        f.write("Jane,25,Boston\n")
        f.write("Bob,35,Chicago\n")
        temp_path = f.name
    
    try:
        # Используем mmap_reader
        with fastcsv.mmap_reader(temp_path) as reader:
            for row in reader:
                print(f"Row {reader.line_num}: {row}")
    finally:
        os.unlink(temp_path)
    
    print()


def example_mmap_dict_reader():
    """Пример использования mmap_DictReader"""
    print("=== Пример mmap_DictReader ===")
    
    with tempfile.NamedTemporaryFile(mode='w', suffix='.csv', delete=False) as f:
        f.write("name,age,city\n")
        f.write("John,30,New York\n")
        f.write("Jane,25,Boston\n")
        temp_path = f.name
    
    try:
        with fastcsv.mmap_DictReader(temp_path) as reader:
            for row in reader:
                print(f"{row['name']} is {row['age']} years old, lives in {row['city']}")
    finally:
        os.unlink(temp_path)
    
    print()


def example_mmap_custom_delimiter():
    """Пример mmap_reader с кастомным разделителем"""
    print("=== mmap_reader с кастомным разделителем ===")
    
    with tempfile.NamedTemporaryFile(mode='w', suffix='.csv', delete=False) as f:
        f.write("name|age|city\n")
        f.write("John|30|New York\n")
        f.write("Jane|25|Boston\n")
        temp_path = f.name
    
    try:
        with fastcsv.mmap_reader(temp_path, delimiter='|') as reader:
            for row in reader:
                print(row)
    finally:
        os.unlink(temp_path)
    
    print()


def example_mmap_large_file():
    """Пример работы с большим файлом через mmap"""
    print("=== Работа с большим файлом через mmap ===")
    
    # Создаем файл с 10000 строками
    with tempfile.NamedTemporaryFile(mode='w', suffix='.csv', delete=False) as f:
        f.write("id,name,value\n")
        for i in range(10000):
            f.write(f"{i},Item{i},{i * 10}\n")
        temp_path = f.name
    
    try:
        # mmap позволяет эффективно работать с большими файлами
        # без загрузки всего файла в память
        with fastcsv.mmap_reader(temp_path) as reader:
            count = 0
            for row in reader:
                count += 1
                if count <= 3:
                    print(f"First rows: {row}")
                elif count == 10000:
                    print(f"Last row: {row}")
            
            print(f"Total rows processed: {count}")
    finally:
        os.unlink(temp_path)
    
    print()


def example_mmap_vs_regular():
    """Сравнение mmap_reader и обычного reader"""
    print("=== Сравнение mmap_reader и обычного reader ===")
    
    with tempfile.NamedTemporaryFile(mode='w', suffix='.csv', delete=False) as f:
        f.write("col1,col2,col3\n")
        for i in range(1000):
            f.write(f"val{i},val{i+1},val{i+2}\n")
        temp_path = f.name
    
    try:
        # Обычный reader
        import time
        start = time.perf_counter()
        with open(temp_path, 'r') as f:
            reader = fastcsv.reader(f)
            rows_regular = list(reader)
        time_regular = time.perf_counter() - start
        
        # mmap_reader
        start = time.perf_counter()
        with fastcsv.mmap_reader(temp_path) as reader:
            rows_mmap = list(reader)
        time_mmap = time.perf_counter() - start
        
        print(f"Regular reader: {time_regular*1000:.2f} ms, {len(rows_regular)} rows")
        print(f"mmap_reader: {time_mmap*1000:.2f} ms, {len(rows_mmap)} rows")
        print(f"Speedup: {time_regular/time_mmap:.2f}x")
        print(f"Results match: {rows_regular == rows_mmap}")
    finally:
        os.unlink(temp_path)
    
    print()


if __name__ == "__main__":
    example_basic_mmap()
    example_mmap_dict_reader()
    example_mmap_custom_delimiter()
    example_mmap_large_file()
    example_mmap_vs_regular()
    
    print("Все примеры выполнены успешно!")








