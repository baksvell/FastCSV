"""
Полная проверка производительности FastCSV vs стандартный csv модуль
Множественные прогоны для получения стабильных результатов
"""

import time
import csv
import fastcsv
import io
import random
import statistics

def generate_csv_data(num_rows, num_fields=10, has_quotes=False, has_unicode=False):
    """Генерирует тестовые CSV данные"""
    header = [f"col{i}" for i in range(num_fields)]
    
    rows = []
    for i in range(num_rows):
        row = []
        for j in range(num_fields):
            if has_unicode and random.random() < 0.1:
                value = f"Значение{i}_{j}_тест"
            else:
                value = f"value{i}_{j}_{random.randint(1000, 9999)}"
            
            if has_quotes and random.random() < 0.2:
                value = f'"{value}"'
            
            row.append(value)
        rows.append(row)
    
    lines = [','.join(header)]
    for row in rows:
        lines.append(','.join(row))
    
    return '\n'.join(lines) + '\n'

def benchmark_csv_module(data, module_name, iterations=5):
    """Бенчмарк с множественными прогонами"""
    times = []
    row_count = 0
    
    for _ in range(iterations):
        f = io.StringIO(data)
        start = time.perf_counter()
        
        if module_name == 'csv':
            reader = csv.reader(f)
            rows = list(reader)
        else:
            reader = fastcsv.reader(f)
            rows = list(reader)
        
        elapsed = time.perf_counter() - start
        times.append(elapsed)
        row_count = len(rows)
    
    # Возвращаем медиану для более стабильных результатов
    return statistics.median(times), row_count

def run_full_benchmark():
    """Запускает полный бенчмарк с множественными прогонами"""
    
    test_cases = [
        # (название, количество строк, количество полей, есть кавычки, есть unicode)
        ("Tiny (5 rows)", 5, 10, False, False),
        ("Small (10 rows)", 10, 10, False, False),
        ("Small (50 rows)", 50, 10, False, False),
        ("Small (100 rows)", 100, 10, False, False),
        ("Medium (500 rows)", 500, 10, False, False),
        ("Medium (1000 rows)", 1000, 10, False, False),
        ("Medium (2000 rows)", 2000, 10, False, False),
        ("Medium (5000 rows)", 5000, 10, False, False),
        ("Large (10000 rows)", 10000, 10, False, False),
        ("Medium with quotes (1000 rows)", 1000, 10, True, False),
        ("Medium with unicode (1000 rows)", 1000, 10, False, True),
        ("Medium with quotes+unicode (1000 rows)", 1000, 10, True, True),
        ("Wide (1000 rows, 50 fields)", 1000, 50, False, False),
        ("Wide with quotes (1000 rows, 50 fields)", 1000, 50, True, False),
    ]
    
    print("=" * 100)
    print("ПОЛНАЯ ПРОВЕРКА ПРОИЗВОДИТЕЛЬНОСТИ: FastCSV vs стандартный csv")
    print("=" * 100)
    print(f"{'Тест':<50} {'csv (мс)':<12} {'FastCSV (мс)':<15} {'Speedup':<12} {'Улучшение':<12}")
    print("-" * 100)
    
    results = []
    
    for test_name, num_rows, num_fields, has_quotes, has_unicode in test_cases:
        # Генерируем данные
        data = generate_csv_data(num_rows, num_fields, has_quotes, has_unicode)
        file_size_kb = len(data) / 1024
        
        # Разогрев
        benchmark_csv_module(data, 'csv', 2)
        benchmark_csv_module(data, 'fastcsv', 2)
        
        # Бенчмарк стандартного csv (7 прогонов для стабильности)
        csv_time, row_count = benchmark_csv_module(data, 'csv', 7)
        csv_time_ms = csv_time * 1000
        
        # Бенчмарк FastCSV (7 прогонов для стабильности)
        fastcsv_time, _ = benchmark_csv_module(data, 'fastcsv', 7)
        fastcsv_time_ms = fastcsv_time * 1000
        
        # Вычисляем speedup
        speedup = csv_time / fastcsv_time if fastcsv_time > 0 else 0
        
        # Форматируем speedup
        if speedup >= 1:
            speedup_str = f"{speedup:.2f}x faster"
            improvement = ((csv_time_ms - fastcsv_time_ms) / csv_time_ms) * 100
            improvement_str = f"{improvement:.1f}%"
        else:
            speedup_str = f"{1/speedup:.2f}x slower"
            regression = ((fastcsv_time_ms - csv_time_ms) / csv_time_ms) * 100
            improvement_str = f"-{regression:.1f}%"
        
        print(f"{test_name:<50} {csv_time_ms:>10.2f} {fastcsv_time_ms:>13.2f} {speedup_str:>12} {improvement_str:>12}")
        
        results.append({
            'test': test_name,
            'rows': num_rows,
            'fields': num_fields,
            'size_kb': file_size_kb,
            'csv_ms': csv_time_ms,
            'fastcsv_ms': fastcsv_time_ms,
            'speedup': speedup
        })
    
    print("=" * 100)
    
    # Детальный анализ
    print("\n" + "=" * 100)
    print("ДЕТАЛЬНЫЙ АНАЛИЗ РЕЗУЛЬТАТОВ")
    print("=" * 100)
    
    # Группировка по категориям
    categories = {
        'Small files (<100 rows)': [r for r in results if r['rows'] < 100],
        'Medium files (100-5000 rows)': [r for r in results if 100 <= r['rows'] <= 5000],
        'Large files (>5000 rows)': [r for r in results if r['rows'] > 5000],
        'Files with quotes': [r for r in results if 'quotes' in r['test'].lower()],
        'Files with unicode': [r for r in results if 'unicode' in r['test'].lower()],
        'Wide files (50 fields)': [r for r in results if r['fields'] == 50],
    }
    
    for category, category_results in categories.items():
        if not category_results:
            continue
        
        print(f"\n{category}:")
        print("-" * 100)
        faster_count = sum(1 for r in category_results if r['speedup'] >= 1)
        slower_count = len(category_results) - faster_count
        
        print(f"  Тестов: {len(category_results)}")
        print(f"  FastCSV быстрее: {faster_count}")
        print(f"  FastCSV медленнее: {slower_count}")
        
        if faster_count > 0:
            avg_speedup = statistics.mean([r['speedup'] for r in category_results if r['speedup'] >= 1])
            print(f"  Среднее ускорение (где быстрее): {avg_speedup:.2f}x")
        
        if slower_count > 0:
            avg_slowdown = statistics.mean([1/r['speedup'] for r in category_results if r['speedup'] < 1])
            print(f"  Среднее замедление (где медленнее): {avg_slowdown:.2f}x")
    
    # Общая статистика
    print("\n" + "=" * 100)
    print("ОБЩАЯ СТАТИСТИКА")
    print("=" * 100)
    
    faster_count = sum(1 for r in results if r['speedup'] >= 1)
    slower_count = len(results) - faster_count
    
    print(f"Всего тестов: {len(results)}")
    print(f"Тестов где FastCSV быстрее: {faster_count} ({faster_count/len(results)*100:.1f}%)")
    print(f"Тестов где FastCSV медленнее: {slower_count} ({slower_count/len(results)*100:.1f}%)")
    
    if faster_count > 0:
        avg_speedup_faster = statistics.mean([r['speedup'] for r in results if r['speedup'] >= 1])
        print(f"Среднее ускорение (где быстрее): {avg_speedup_faster:.2f}x")
    
    if slower_count > 0:
        avg_slowdown = statistics.mean([1/r['speedup'] for r in results if r['speedup'] < 1])
        print(f"Среднее замедление (где медленнее): {avg_slowdown:.2f}x")
    
    # Топ результатов
    print("\n" + "=" * 100)
    print("ТОП-5 САМЫХ БЫСТРЫХ РЕЗУЛЬТАТОВ (FastCSV)")
    print("=" * 100)
    top_faster = sorted([r for r in results if r['speedup'] >= 1], key=lambda x: x['speedup'], reverse=True)[:5]
    for i, r in enumerate(top_faster, 1):
        print(f"{i}. {r['test']}: {r['speedup']:.2f}x быстрее ({r['csv_ms']:.2f}ms -> {r['fastcsv_ms']:.2f}ms)")
    
    print("\n" + "=" * 100)
    print("ТОП-5 САМЫХ МЕДЛЕННЫХ РЕЗУЛЬТАТОВ (FastCSV)")
    print("=" * 100)
    top_slower = sorted([r for r in results if r['speedup'] < 1], key=lambda x: 1/x['speedup'], reverse=True)[:5]
    for i, r in enumerate(top_slower, 1):
        print(f"{i}. {r['test']}: {1/r['speedup']:.2f}x медленнее ({r['csv_ms']:.2f}ms -> {r['fastcsv_ms']:.2f}ms)")
    
    print("\n" + "=" * 100)
    print("КЛЮЧЕВЫЕ МЕТРИКИ")
    print("=" * 100)
    
    # Ключевые тесты
    key_tests = {
        'Medium (1000 rows)': [r for r in results if r['test'] == 'Medium (1000 rows)'],
        'Medium with quotes (1000 rows)': [r for r in results if r['test'] == 'Medium with quotes (1000 rows)'],
        'Medium with unicode (1000 rows)': [r for r in results if r['test'] == 'Medium with unicode (1000 rows)'],
    }
    
    for test_name, test_results in key_tests.items():
        if test_results:
            r = test_results[0]
            print(f"\n{test_name}:")
            print(f"  Размер файла: {r['size_kb']:.2f} KB")
            print(f"  Стандартный csv: {r['csv_ms']:.2f} мс")
            print(f"  FastCSV: {r['fastcsv_ms']:.2f} мс")
            if r['speedup'] >= 1:
                improvement = ((r['csv_ms'] - r['fastcsv_ms']) / r['csv_ms']) * 100
                print(f"  Ускорение: {r['speedup']:.2f}x")
                print(f"  Улучшение: {improvement:.1f}%")
            else:
                regression = ((r['fastcsv_ms'] - r['csv_ms']) / r['csv_ms']) * 100
                print(f"  Замедление: {1/r['speedup']:.2f}x")
                print(f"  Регрессия: {regression:.1f}%")
    
    print("\n" + "=" * 100)
    print("Бенчмарк завершен!")
    print("=" * 100)

if __name__ == "__main__":
    run_full_benchmark()




