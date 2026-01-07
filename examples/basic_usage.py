"""Примеры использования FastCSV"""

import fastcsv
import io

# Пример 1: Простое чтение CSV
print("=== Пример 1: Простое чтение ===")
data = "name,age,city\nJohn,30,New York\nJane,25,Boston"
f = io.StringIO(data)

reader = fastcsv.reader(f)
for row in reader:
    print(row)

# Пример 2: DictReader
print("\n=== Пример 2: DictReader ===")
data = "name,age,city\nJohn,30,New York\nJane,25,Boston"
f = io.StringIO(data)

reader = fastcsv.DictReader(f)
for row in reader:
    print(f"{row['name']} is {row['age']} years old, lives in {row['city']}")

# Пример 3: Запись CSV
print("\n=== Пример 3: Запись CSV ===")
f = io.StringIO()
writer = fastcsv.writer(f)
writer.writerow(["name", "age", "city"])
writer.writerow(["Alice", "28", "Seattle"])
writer.writerow(["Bob", "35", "Chicago"])

print(f.getvalue())

# Пример 4: DictWriter
print("\n=== Пример 4: DictWriter ===")
f = io.StringIO()
fieldnames = ["name", "age", "city"]
writer = fastcsv.DictWriter(f, fieldnames)
writer.writeheader()
writer.writerow({"name": "Alice", "age": "28", "city": "Seattle"})
writer.writerow({"name": "Bob", "age": "35", "city": "Chicago"})

print(f.getvalue())

# Пример 5: Кастомный разделитель
print("\n=== Пример 5: Кастомный разделитель ===")
data = "name|age|city\nJohn|30|New York"
f = io.StringIO(data)
reader = fastcsv.reader(f, delimiter='|')
for row in reader:
    print(row)








