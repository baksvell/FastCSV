"""Примеры использования Sniffer в FastCSV"""

import fastcsv
import io

# Пример 1: Автоопределение формата
print("=== Пример 1: Автоопределение формата ===")
sample = "name,age,city\nJohn,30,New York\nJane,25,Boston"
sniffer = fastcsv.Sniffer()
dialect = sniffer.sniff(sample)

print(f"Определенный delimiter: {repr(dialect.delimiter)}")
print(f"Определенный quotechar: {repr(dialect.quotechar)}")
print(f"Определенный lineterminator: {repr(dialect.lineterminator)}")

# Используем определенный диалект
f = io.StringIO(sample)
reader = fastcsv.reader(f, dialect=dialect)
print("Результат парсинга:", list(reader))

# Пример 2: Определение табуляции
print("\n=== Пример 2: Определение табуляции ===")
sample2 = "name\tage\tcity\nJohn\t30\tNYC"
dialect2 = sniffer.sniff(sample2)
print(f"Определенный delimiter: {repr(dialect2.delimiter)}")

# Пример 3: Определение pipe разделителя
print("\n=== Пример 3: Определение pipe разделителя ===")
sample3 = "name|age|city\nJohn|30|NYC"
dialect3 = sniffer.sniff(sample3, delimiters='|,;')
print(f"Определенный delimiter: {repr(dialect3.delimiter)}")

# Пример 4: Определение заголовка
print("\n=== Пример 4: Определение заголовка ===")
sample4 = "name,age,city\nJohn,30,NYC\nJane,25,Boston"
has_header = sniffer.has_header(sample4)
print(f"Есть заголовок: {has_header}")

sample5 = "John,30,NYC\nJane,25,Boston\nBob,35,Chicago"
has_header2 = sniffer.has_header(sample5)
print(f"Есть заголовок (без заголовка): {has_header2}")

# Пример 5: Практическое использование
print("\n=== Пример 5: Практическое использование ===")
# Читаем первые несколько строк файла
with open('unknown_format.csv', 'w', encoding='utf-8') as f:
    f.write("name;age;city\nJohn;30;NYC\nJane;25;Boston")

with open('unknown_format.csv', 'r', encoding='utf-8') as f:
    sample = f.read(1024)  # Читаем первые 1KB
    
sniffer = fastcsv.Sniffer()
dialect = sniffer.sniff(sample)

# Используем определенный диалект для чтения всего файла
with open('unknown_format.csv', 'r', encoding='utf-8') as f:
    reader = fastcsv.reader(f, dialect=dialect)
    for row in reader:
        print(row)








