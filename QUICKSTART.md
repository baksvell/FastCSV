# Быстрый старт FastCSV

## Установка

```bash
# Клонируйте репозиторий
git clone <repository-url>
cd FastCSV

# Установите зависимости
pip install -e ".[dev]"
```

## Первое использование

```python
import fastcsv

# Чтение CSV файла
with open('data.csv', 'r') as f:
    reader = fastcsv.reader(f)
    for row in reader:
        print(row)

# Использование DictReader
with open('data.csv', 'r') as f:
    reader = fastcsv.DictReader(f)
    for row in reader:
        print(row['column_name'])
```

## Сборка из исходников

### Windows (MSVC)

```bash
python setup.py build_ext --inplace
```

### Linux/macOS (GCC/Clang)

```bash
python setup.py build_ext --inplace
```

## Запуск тестов

```bash
pytest tests/
```

## Запуск примеров

```bash
python examples/basic_usage.py
```

## Производительность

FastCSV автоматически использует SIMD инструкции (AVX2/SSE4.2) если они доступны на вашем процессоре. Это дает значительный прирост производительности по сравнению со стандартным модулем `csv`.

## Совместимость

FastCSV полностью совместим с Python `csv` модулем. Вы можете заменить:

```python
import csv
```

на:

```python
import fastcsv as csv
```

И ваш код продолжит работать без изменений!








