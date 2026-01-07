"""Бенчмарки для сравнения производительности FastCSV и стандартного csv модуля"""

import time
import csv
import fastcsv
import io
import random
import string
import tempfile
import os

def generate_csv_data(num_rows, num_cols, use_quotes=False):
    """Генерирует тестовые CSV данные"""
    data = []
    for i in range(num_rows):
        row = []
        for j in range(num_cols):
            if use_quotes and random.random() < 0.3:
                # Иногда добавляем кавычки и запятые
                value = f'"{random.choice(string.ascii_letters)} {random.choice(string.ascii_letters)}, value"'
            else:
                value = f"col{j}_row{i}_{random.randint(1000, 9999)}"
            row.append(value)
        data.append(row)
    
    # Конвертируем в строку
    output = io.StringIO()
    writer = csv.writer(output)
    writer.writerows(data)
    return output.getvalue()


def benchmark_reader(module, data, name):
    """Бенчмарк для reader"""
    f = io.StringIO(data)
    reader = module.reader(f)
    
    start = time.perf_counter()
    rows = list(reader)
    end = time.perf_counter()
    
    elapsed = end - start
    print(f"{name:20s}: {elapsed*1000:8.2f} ms ({len(rows)} rows)")
    return elapsed, len(rows)


def benchmark_dict_reader(module, data, name):
    """Бенчмарк для DictReader"""
    f = io.StringIO(data)
    reader = module.DictReader(f)
    
    start = time.perf_counter()
    rows = list(reader)
    end = time.perf_counter()
    
    elapsed = end - start
    print(f"{name:20s}: {elapsed*1000:8.2f} ms ({len(rows)} rows)")
    return elapsed, len(rows)


def benchmark_mmap_reader(filepath, name):
    """Бенчмарк для mmap_reader"""
    start = time.perf_counter()
    with fastcsv.mmap_reader(filepath) as reader:
        rows = list(reader)
    end = time.perf_counter()
    
    elapsed = end - start
    print(f"{name:20s}: {elapsed*1000:8.2f} ms ({len(rows)} rows)")
    return elapsed, len(rows)


def run_benchmarks():
    """Запускает все бенчмарки"""
    print("=" * 60)
    print("FastCSV vs Standard CSV Benchmark")
    print("=" * 60)
    
    test_cases = [
        (100, 10, False, "Small file (100 rows, 10 cols)"),
        (1000, 20, False, "Medium file (1000 rows, 20 cols)"),
        (10000, 50, False, "Large file (10k rows, 50 cols)"),
        (1000, 20, True, "With quotes (1000 rows, 20 cols)"),
    ]
    
    for num_rows, num_cols, use_quotes, description in test_cases:
        print(f"\n{description}:")
        print("-" * 60)
        
        data = generate_csv_data(num_rows, num_cols, use_quotes)
        
        # Разогрев
        for _ in range(2):
            f = io.StringIO(data)
            list(csv.reader(f))
            f = io.StringIO(data)
            try:
                list(fastcsv.reader(f))
            except Exception as e:
                print(f"Error during warmup: {e}")
                raise
        
        # Бенчмарк reader
        csv_time, csv_rows = benchmark_reader(csv, data, "csv.reader")
        try:
            fastcsv_time, fastcsv_rows = benchmark_reader(fastcsv, data, "fastcsv.reader")
        except Exception as e:
            print(f"Error in fastcsv.reader: {e}")
            # Попробуем найти проблемную строку
            lines = data.split('\n')
            print(f"Problematic line (around line 20): {repr(lines[19] if len(lines) > 19 else 'N/A')}")
            print(f"Previous line: {repr(lines[18] if len(lines) > 18 else 'N/A')}")
            # Попробуем распарсить проблемную строку отдельно
            if len(lines) > 19:
                test_line = lines[19]
                f = io.StringIO(test_line + '\n')
                reader = fastcsv.reader(f)
                try:
                    row = next(reader)
                    print(f"Line parsed successfully: {row}")
                except Exception as e2:
                    print(f"Error parsing line separately: {e2}")
            raise
        
        if csv_rows == fastcsv_rows:
            speedup = csv_time / fastcsv_time if fastcsv_time > 0 else 0
            print(f"{'Speedup':20s}: {speedup:8.2f}x")
        else:
            print(f"WARNING: Row count mismatch! csv={csv_rows}, fastcsv={fastcsv_rows}")
        
        # Бенчмарк DictReader
        print()
        csv_time, csv_rows = benchmark_dict_reader(csv, data, "csv.DictReader")
        fastcsv_time, fastcsv_rows = benchmark_dict_reader(fastcsv, data, "fastcsv.DictReader")
        
        if csv_rows == fastcsv_rows:
            speedup = csv_time / fastcsv_time if fastcsv_time > 0 else 0
            print(f"{'Speedup':20s}: {speedup:8.2f}x")
        else:
            print(f"WARNING: Row count mismatch! csv={csv_rows}, fastcsv={fastcsv_rows}")
    
    # Бенчмарк mmap для больших файлов
    print("\n" + "=" * 60)
    print("mmap_reader Benchmark (Large Files)")
    print("=" * 60)
    
    mmap_test_cases = [
        (1000, 20, "Medium file (1000 rows)"),
        (10000, 50, "Large file (10k rows)"),
        (50000, 30, "Very large file (50k rows)"),
    ]
    
    for num_rows, num_cols, description in mmap_test_cases:
        print(f"\n{description}:")
        print("-" * 60)
        
        # Создаем временный файл
        with tempfile.NamedTemporaryFile(mode='w', suffix='.csv', delete=False, newline='') as f:
            # Генерируем данные напрямую в файл
            writer = csv.writer(f)
            # Заголовок
            writer.writerow([f'col{i}' for i in range(num_cols)])
            # Данные
            for i in range(num_rows):
                row = [f"col{j}_row{i}_{random.randint(1000, 9999)}" for j in range(num_cols)]
                writer.writerow(row)
            temp_path = f.name
        
        try:
            # Разогрев
            for _ in range(2):
                with open(temp_path, 'r') as f:
                    list(csv.reader(f))
                with fastcsv.mmap_reader(temp_path) as reader:
                    list(reader)
            
            # Бенчмарк обычного reader
            start = time.perf_counter()
            with open(temp_path, 'r') as f:
                reader = fastcsv.reader(f)
                regular_rows = list(reader)
            regular_time = time.perf_counter() - start
            
            # Бенчмарк mmap_reader
            mmap_time, mmap_rows = benchmark_mmap_reader(temp_path, "fastcsv.mmap_reader")
            
            if len(regular_rows) == mmap_rows:
                speedup = regular_time / mmap_time if mmap_time > 0 else 0
                print(f"{'Regular reader':20s}: {regular_time*1000:8.2f} ms ({len(regular_rows)} rows)")
                print(f"{'mmap speedup':20s}: {speedup:8.2f}x")
            else:
                print(f"WARNING: Row count mismatch! regular={len(regular_rows)}, mmap={mmap_rows}")
        finally:
            try:
                os.unlink(temp_path)
            except:
                pass


if __name__ == "__main__":
    run_benchmarks()

