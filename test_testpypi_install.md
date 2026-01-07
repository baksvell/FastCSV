# Тестирование установки из TestPyPI

## Проблема

При попытке установить из TestPyPI pip показывает:
```
Requirement already satisfied: fastcsv in ... (0.1.0)
```

Это происходит потому, что пакет уже установлен локально (из разработки).

## Решение: Тестирование в чистом окружении

### Вариант 1: Создать новое виртуальное окружение

```bash
# Создайте новое окружение
python -m venv test_env

# Активируйте его
# Windows
test_env\Scripts\activate

# Linux/macOS
source test_env/bin/activate

# Установите из TestPyPI
pip install --index-url https://test.pypi.org/simple/ --extra-index-url https://pypi.org/simple/ fastcsv

# Проверьте версию
python -c "import fastcsv; print(f'FastCSV version: {fastcsv.__version__}')"

# Протестируйте функциональность
python -c "
import fastcsv
import io

# Тест reader
data = 'name,age\nJohn,30\nJane,25'
f = io.StringIO(data)
reader = fastcsv.reader(f)
for row in reader:
    print(row)
"
```

### Вариант 2: Удалить локальную установку

```bash
# В текущем окружении
pip uninstall fastcsv -y

# Установить из TestPyPI
pip install --index-url https://test.pypi.org/simple/ --extra-index-url https://pypi.org/simple/ fastcsv

# Проверьте версию
python -c "import fastcsv; print(f'FastCSV version: {fastcsv.__version__}')"
```

## Что проверить

1. ✅ Версия должна быть `0.2.0`
2. ✅ Все функции должны работать
3. ✅ Импорт должен проходить без ошибок
4. ✅ Тесты должны проходить (если есть)

## После успешного тестирования

Если все работает, можно публиковать на основной PyPI!

