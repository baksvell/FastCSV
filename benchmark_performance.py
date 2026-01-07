"""
Бенчмарк производительности FastCSV vs стандартный csv модуль
Особое внимание к средним файлам (1000 строк)
"""

import time
import csv
import fastcsv
import io
import random
import string

def generate_csv_data(num_rows, num_fields=10, has_quotes=False, has_unicode=False):
    """Генерирует тестовые CSV данные"""
    # Генерируем заголовки
    header = [f"col{i}" for i in range(num_fields)]
    
    # Генерируем данные
    rows = []
    for i in range(num_rows):
        row = []
        for j in range(num_fields):
            if has_unicode and random.random() < 0.1:
                # 10% полей с Unicode
                value = f"Значение{i}_{j}_тест"
            else:
                # Обычные ASCII значения
                value = f"value{i}_{j}_{random.randint(1000, 9999)}"
            
            if has_quotes and random.random() < 0.2:
                # 20% полей в кавычках
                value = f'"{value}"'
            
            row.append(value)
        rows.append(row)
    
    # Формируем CSV строку
    lines = [','.join(header)]
    for row in rows:
        lines.append(','.join(row))
    
    return '\n'.join(lines) + '\n'

def benchmark_csv_module(data, module_name):
    """Бенчмарк для стандартного csv модуля"""
    f = io.StringIO(data)
    start = time.perf_counter()
    
    if module_name == 'csv':
        reader = csv.reader(f)
        rows = list(reader)
    else:
        reader = fastcsv.reader(f)
        rows = list(reader)
    
    elapsed = time.perf_counter() - start
    return elapsed, len(rows)

def run_benchmark():
    """Запускает бенчмарк для разных размеров файлов"""
    
    test_cases = [
        # (название, количество строк, количество полей, есть кавычки, есть unicode)
        ("Small (10 rows)", 10, 10, False, False),
        ("Small (100 rows)", 100, 10, False, False),
        ("Medium (500 rows)", 500, 10, False, False),
        ("Medium (1000 rows)", 1000, 10, False, False),  # Ключевой тест
        ("Medium (2000 rows)", 2000, 10, False, False),
        ("Large (10000 rows)", 10000, 10, False, False),
        ("Medium with quotes (1000 rows)", 1000, 10, True, False),
        ("Medium with unicode (1000 rows)", 1000, 10, False, True),
    ]
    
    print("=" * 80)
    print("Бенчмарк производительности: FastCSV vs стандартный csv")
    print("=" * 80)
    print(f"{'Тест':<40} {'csv (мс)':<12} {'FastCSV (мс)':<15} {'Speedup':<10}")
    print("-" * 80)
    
    results = []
    
    for test_name, num_rows, num_fields, has_quotes, has_unicode in test_cases:
        # Генерируем данные
        data = generate_csv_data(num_rows, num_fields, has_quotes, has_unicode)
        file_size_kb = len(data) / 1024
        
        # Разогрев (warmup)
        benchmark_csv_module(data, 'csv')
        benchmark_csv_module(data, 'fastcsv')
        
        # Бенчмарк стандартного csv
        csv_times = []
        for _ in range(5):
            elapsed, row_count = benchmark_csv_module(data, 'csv')
            csv_times.append(elapsed)
        csv_avg = sum(csv_times) / len(csv_times)
        csv_avg_ms = csv_avg * 1000
        
        # Бенчмарк FastCSV
        fastcsv_times = []
        for _ in range(5):
            elapsed, row_count = benchmark_csv_module(data, 'fastcsv')
            fastcsv_times.append(elapsed)
        fastcsv_avg = sum(fastcsv_times) / len(fastcsv_times)
        fastcsv_avg_ms = fastcsv_avg * 1000
        
        # Вычисляем speedup
        speedup = csv_avg / fastcsv_avg if fastcsv_avg > 0 else 0
        
        # Форматируем speedup
        if speedup >= 1:
            speedup_str = f"{speedup:.2f}x faster"
        else:
            speedup_str = f"{1/speedup:.2f}x slower"
        
        print(f"{test_name:<40} {csv_avg_ms:>10.2f} {fastcsv_avg_ms:>13.2f} {speedup_str:>10}")
        
        results.append({
            'test': test_name,
            'rows': num_rows,
            'size_kb': file_size_kb,
            'csv_ms': csv_avg_ms,
            'fastcsv_ms': fastcsv_avg_ms,
            'speedup': speedup
        })
    
    print("=" * 80)
    
    # Анализ результатов для средних файлов
    print("\nАнализ производительности для средних файлов (1000 rows):")
    print("-" * 80)
    
    medium_tests = [r for r in results if r['rows'] == 1000]
    for result in medium_tests:
        print(f"\n{result['test']}:")
        print(f"  Размер файла: {result['size_kb']:.2f} KB")
        print(f"  Стандартный csv: {result['csv_ms']:.2f} мс")
        print(f"  FastCSV: {result['fastcsv_ms']:.2f} мс")
        if result['speedup'] >= 1:
            print(f"  Ускорение: {result['speedup']:.2f}x (FastCSV быстрее)")
        else:
            print(f"  Замедление: {1/result['speedup']:.2f}x (FastCSV медленнее)")
    
    # Общая статистика
    print("\n" + "=" * 80)
    print("Общая статистика:")
    print("-" * 80)
    
    faster_count = sum(1 for r in results if r['speedup'] >= 1)
    slower_count = len(results) - faster_count
    
    print(f"Тестов где FastCSV быстрее: {faster_count}/{len(results)}")
    print(f"Тестов где FastCSV медленнее: {slower_count}/{len(results)}")
    
    if faster_count > 0:
        avg_speedup_faster = sum(r['speedup'] for r in results if r['speedup'] >= 1) / faster_count
        print(f"Среднее ускорение (где быстрее): {avg_speedup_faster:.2f}x")
    
    if slower_count > 0:
        avg_slowdown = sum(1/r['speedup'] for r in results if r['speedup'] < 1) / slower_count
        print(f"Среднее замедление (где медленнее): {avg_slowdown:.2f}x")
    
    # Особое внимание к ключевому тесту
    key_test = next((r for r in results if r['test'] == "Medium (1000 rows)"), None)
    if key_test:
        print("\n" + "=" * 80)
        print("Ключевой тест: Medium (1000 rows)")
        print("-" * 80)
        print(f"Размер файла: {key_test['size_kb']:.2f} KB")
        print(f"Стандартный csv: {key_test['csv_ms']:.2f} мс")
        print(f"FastCSV: {key_test['fastcsv_ms']:.2f} мс")
        if key_test['speedup'] >= 1:
            improvement = ((key_test['csv_ms'] - key_test['fastcsv_ms']) / key_test['csv_ms']) * 100
            print(f"Ускорение: {key_test['speedup']:.2f}x")
            print(f"Улучшение: {improvement:.1f}%")
        else:
            regression = ((key_test['fastcsv_ms'] - key_test['csv_ms']) / key_test['csv_ms']) * 100
            print(f"Замедление: {1/key_test['speedup']:.2f}x")
            print(f"Регрессия: {regression:.1f}%")

if __name__ == "__main__":
    run_benchmark()




