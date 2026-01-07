# FastCSV API Documentation

## Основные классы

### `reader(csvfile, dialect='excel', **fmtparams)`

CSV reader, совместимый с `csv.reader`.

**Параметры:**
- `csvfile`: Файловый объект для чтения
- `dialect`: Имя диалекта (строка), объект Dialect, или 'excel' по умолчанию
- `**fmtparams`: Дополнительные параметры форматирования

**Пример:**
```python
import fastcsv

with open('data.csv', 'r') as f:
    reader = fastcsv.reader(f)
    for row in reader:
        print(row)
```

### `DictReader(csvfile, fieldnames=None, restkey=None, restval=None, dialect='excel', **fmtparams)`

CSV DictReader, совместимый с `csv.DictReader`.

**Параметры:**
- `csvfile`: Файловый объект для чтения
- `fieldnames`: Список имен полей (если None, читается из первой строки)
- `restkey`: Ключ для лишних полей
- `restval`: Значение для отсутствующих полей
- `dialect`: Диалект для парсинга
- `**fmtparams`: Дополнительные параметры

**Пример:**
```python
with open('data.csv', 'r') as f:
    reader = fastcsv.DictReader(f)
    for row in reader:
        print(row['name'])
```

### `writer(csvfile, dialect='excel', **fmtparams)`

CSV writer, совместимый с `csv.writer`.

**Методы:**
- `writerow(row)`: Записывает одну строку
- `writerows(rows)`: Записывает несколько строк

### `DictWriter(csvfile, fieldnames, restval='', extrasaction='raise', dialect='excel', **fmtparams)`

CSV DictWriter, совместимый с `csv.DictWriter`.

**Методы:**
- `writeheader()`: Записывает заголовки
- `writerow(rowdict)`: Записывает строку из словаря
- `writerows(rowdicts)`: Записывает несколько строк

## Работа с диалектами

### `Dialect(delimiter=',', quotechar='"', doublequote=True, escapechar=None, lineterminator='\r\n', quoting=QUOTE_MINIMAL, skipinitialspace=False, strict=False)`

Класс для создания кастомных диалектов CSV.

**Параметры:**
- `delimiter`: Разделитель полей (по умолчанию ',')
- `quotechar`: Символ кавычки (по умолчанию '"')
- `doublequote`: Экранирование кавычек удвоением (по умолчанию True)
- `escapechar`: Символ экранирования (по умолчанию None)
- `lineterminator`: Терминатор строки (по умолчанию '\r\n')
- `quoting`: Режим кавычек (QUOTE_MINIMAL, QUOTE_ALL, QUOTE_NONNUMERIC, QUOTE_NONE)
- `skipinitialspace`: Пропускать начальные пробелы (по умолчанию False)
- `strict`: Строгий режим (по умолчанию False)

### `register_dialect(name, dialect=None, **fmtparams)`

Регистрирует диалект с указанным именем.

**Параметры:**
- `name`: Имя диалекта
- `dialect`: Объект Dialect или None
- `**fmtparams`: Параметры для создания диалекта

**Пример:**
```python
fastcsv.register_dialect('pipe', delimiter='|')
fastcsv.register_dialect('semicolon', delimiter=';', quotechar="'")
```

### `unregister_dialect(name)`

Удаляет диалект из реестра.

**Параметры:**
- `name`: Имя диалекта для удаления

**Raises:**
- `KeyError`: Если диалект не найден

### `get_dialect(name)`

Возвращает диалект по имени.

**Параметры:**
- `name`: Имя диалекта

**Returns:**
- Объект Dialect

**Raises:**
- `KeyError`: Если диалект не найден

### `list_dialects()`

Возвращает список всех зарегистрированных диалектов.

**Returns:**
- Список имен диалектов

## Sniffer

### `Sniffer()`

Класс для автоопределения формата CSV файла.

### `Sniffer.sniff(sample, delimiters=None)`

Анализирует образец данных и определяет параметры формата.

**Параметры:**
- `sample`: Образец CSV данных (строка)
- `delimiters`: Строка возможных разделителей (по умолчанию: ',;\\t|')

**Returns:**
- Объект Dialect с определенными параметрами

**Пример:**
```python
sniffer = fastcsv.Sniffer()
with open('unknown.csv', 'r') as f:
    sample = f.read(1024)
    dialect = sniffer.sniff(sample)
    f.seek(0)
    reader = fastcsv.reader(f, dialect=dialect)
    for row in reader:
        print(row)
```

### `Sniffer.has_header(sample)`

Определяет, есть ли заголовок в CSV файле.

**Параметры:**
- `sample`: Образец CSV данных

**Returns:**
- `True` если первая строка похожа на заголовок, `False` иначе

## Константы

- `QUOTE_MINIMAL` (0): Кавычки только при необходимости
- `QUOTE_ALL` (1): Кавычки вокруг всех полей
- `QUOTE_NONNUMERIC` (2): Кавычки вокруг нечисловых полей
- `QUOTE_NONE` (3): Без кавычек

## Исключения

### `Error`

Базовый класс для ошибок CSV. Наследуется от `Exception`.

## mmap для больших файлов

### `mmap_reader(filepath, dialect='excel', access=mmap.ACCESS_READ, **fmtparams)`

CSV reader с использованием memory-mapped файлов для эффективной работы с очень большими файлами (>100MB).

**Преимущества:**
- Эффективная работа с файлами, которые не помещаются в RAM
- 2-5x ускорение для файлов >1000 строк по сравнению с обычным reader
- Минимальное использование памяти

**Параметры:**
- `filepath`: Путь к CSV файлу (строка или PathLike)
- `dialect`: Диалект для парсинга
- `access`: Режим доступа mmap (по умолчанию `mmap.ACCESS_READ`)
- `**fmtparams`: Дополнительные параметры форматирования

**Пример:**
```python
import fastcsv

# Базовое использование
with fastcsv.mmap_reader('large_file.csv') as reader:
    for row in reader:
        print(row)

# С кастомным разделителем
with fastcsv.mmap_reader('data.csv', delimiter='|') as reader:
    for row in reader:
        print(row)
```

### `mmap_DictReader(filepath, fieldnames=None, restkey=None, restval=None, dialect='excel', **fmtparams)`

CSV DictReader с использованием memory-mapped файлов.

**Параметры:**
- `filepath`: Путь к CSV файлу
- `fieldnames`: Список имен полей (если None, читается из первой строки)
- `restkey`: Ключ для лишних полей
- `restval`: Значение для отсутствующих полей
- `dialect`: Диалект для парсинга
- `**fmtparams`: Дополнительные параметры

**Пример:**
```python
with fastcsv.mmap_DictReader('large_file.csv') as reader:
    for row in reader:
        print(row['name'], row['age'])
```

**Когда использовать mmap:**
- Файлы больше 100MB
- Необходимость обработать файл, который не помещается в RAM
- Требуется максимальная производительность для больших файлов
- Обработка файлов >10k строк

## Стандартные диалекты

- `excel`: Стандартный Excel формат (delimiter=',', quotechar='"')
- `excel-tab`: Табуляция как разделитель (delimiter='\\t')
- `unix`: Unix формат (lineterminator='\\n', quoting=QUOTE_ALL)

