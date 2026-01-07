"""Примеры использования диалектов в FastCSV"""

import fastcsv
import io

# Пример 1: Использование стандартных диалектов
print("=== Пример 1: Стандартные диалекты ===")
print("Доступные диалекты:", fastcsv.list_dialects())

# Excel диалект (по умолчанию)
data = "name,age,city\nJohn,30,New York"
f = io.StringIO(data)
reader = fastcsv.reader(f, dialect='excel')
print("Excel:", list(reader))

# Excel-tab диалект
data = "name\tage\tcity\nJohn\t30\tNew York"
f = io.StringIO(data)
reader = fastcsv.reader(f, dialect='excel-tab')
print("Excel-tab:", list(reader))

# Unix диалект
data = 'name,age,city\n"John","30","New York"'
f = io.StringIO(data)
reader = fastcsv.reader(f, dialect='unix')
print("Unix:", list(reader))

# Пример 2: Регистрация кастомного диалекта
print("\n=== Пример 2: Кастомный диалект ===")
fastcsv.register_dialect('pipe', delimiter='|', quotechar="'")

data = "name|age|city\nJohn|30|New York"
f = io.StringIO(data)
reader = fastcsv.reader(f, dialect='pipe')
print("Pipe dialect:", list(reader))

# Пример 3: Получение и использование диалекта
print("\n=== Пример 3: Получение диалекта ===")
dialect = fastcsv.get_dialect('pipe')
print(f"Dialect delimiter: {dialect.delimiter}")
print(f"Dialect quotechar: {dialect.quotechar}")

# Пример 4: Создание и использование объекта Dialect
print("\n=== Пример 4: Объект Dialect ===")
custom_dialect = fastcsv.Dialect(
    delimiter=';',
    quotechar="'",
    skipinitialspace=True
)

data = "name; age; city\nJohn; 30; New York"
f = io.StringIO(data)
reader = fastcsv.reader(f, dialect=custom_dialect)
print("Custom dialect:", list(reader))

# Пример 5: Удаление диалекта
print("\n=== Пример 5: Удаление диалекта ===")
print("До удаления:", fastcsv.list_dialects())
fastcsv.unregister_dialect('pipe')
print("После удаления:", fastcsv.list_dialects())








