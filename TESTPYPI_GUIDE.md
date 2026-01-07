# Руководство по публикации на TestPyPI

Это пошаговое руководство поможет вам протестировать публикацию FastCSV на TestPyPI перед публикацией на основной PyPI.

## Шаг 1: Подготовка

### 1.1 Установите необходимые инструменты

```bash
pip install build twine
```

### 1.2 Проверьте, что все тесты проходят

```bash
python -m pytest tests/ -v
```

### 1.3 Убедитесь, что версии совпадают

Проверьте, что версия в `pyproject.toml` совпадает с версией в `fastcsv/__init__.py`:
- `pyproject.toml`: `version = "0.2.0"`
- `fastcsv/__init__.py`: `__version__ = "0.2.0"`

## Шаг 2: Создание аккаунта на TestPyPI

1. Перейдите на https://test.pypi.org/
2. Нажмите "Register" (если у вас еще нет аккаунта)
3. Заполните форму регистрации
4. Подтвердите email (проверьте почту)

## Шаг 3: Создание API токена

1. Войдите в аккаунт на TestPyPI
2. Перейдите в Account settings → API tokens: https://test.pypi.org/manage/account/token/
3. Нажмите "Add API token"
4. Заполните:
   - Token name: `FastCSV TestPyPI`
   - Scope: выберите "Entire account" или "Project: fastcsv"
5. Нажмите "Add token"
6. **ВАЖНО:** Скопируйте токен сразу! Он показывается только один раз.
   - Токен начинается с `pypi-`
   - Сохраните его в безопасном месте

## Шаг 4: Публикация на TestPyPI

### Вариант A: Использование скрипта (рекомендуется)

```bash
python publish_testpypi.py
```

Скрипт автоматически:
- Проверит наличие необходимых инструментов
- Очистит старые сборки
- Соберет пакет
- Проверит пакет
- Загрузит на TestPyPI

При запросе учетных данных:
- Username: `__token__`
- Password: вставьте ваш API токен (начинается с `pypi-`)

### Вариант B: Ручная публикация

#### 4.1 Очистите старые сборки

```bash
# Windows
rmdir /s /q dist build
del /s /q *.egg-info

# Linux/macOS
rm -rf dist/ build/ *.egg-info
```

#### 4.2 Соберите пакет

```bash
python -m build
```

Это создаст файлы в `dist/`:
- `fastcsv-0.2.0.tar.gz` (source distribution)
- `fastcsv-0.2.0-*.whl` (wheel для вашей платформы)

#### 4.3 Проверьте пакет

```bash
python -m twine check dist/*
```

Должно вывести: `Checking dist/...: PASSED`

#### 4.4 Загрузите на TestPyPI

```bash
python -m twine upload --repository testpypi dist/*
```

При запросе:
- Username: `__token__`
- Password: ваш API токен (начинается с `pypi-`)

## Шаг 5: Проверка на TestPyPI

1. Откройте https://test.pypi.org/project/pyfastcsv/
2. Убедитесь, что пакет отображается корректно
3. Проверьте, что все файлы загружены

## Шаг 6: Тестовая установка

### 6.1 Создайте виртуальное окружение (рекомендуется)

```bash
# Создайте новое виртуальное окружение
python -m venv test_env

# Активируйте его
# Windows
test_env\Scripts\activate

# Linux/macOS
source test_env/bin/activate
```

### 6.2 Установите из TestPyPI

```bash
pip install --index-url https://test.pypi.org/simple/ --extra-index-url https://pypi.org/simple/ pyfastcsv
```

**Примечание:** `--extra-index-url https://pypi.org/simple/` нужен для установки зависимостей (если они есть), так как TestPyPI не содержит все пакеты.

### 6.3 Протестируйте установленный пакет

```bash
python -c "import fastcsv; print(f'FastCSV version: {fastcsv.__version__}')"
```

Должно вывести: `FastCSV version: 0.2.0`

### 6.4 Запустите тесты

```bash
# Если у вас есть тесты в пакете
python -m pytest tests/ -v

# Или протестируйте вручную
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

## Шаг 7: Проверка метаданных

Убедитесь, что на странице TestPyPI отображается:
- ✅ Правильное описание
- ✅ Правильные авторы
- ✅ Правильные ссылки (Homepage, Repository, Issues)
- ✅ Правильные classifiers
- ✅ README отображается корректно

## Шаг 8: Если все работает

Если тестовая установка прошла успешно:

1. ✅ Все работает - можно публиковать на основной PyPI
2. ❌ Есть проблемы - исправьте и повторите процесс

## Troubleshooting

### Ошибка: "File already exists"

Версия `0.2.0` уже существует на TestPyPI. Решения:
1. Используйте другую версию (например, `0.2.0.post1`)
2. Или удалите старую версию (если возможно)

### Ошибка: "Invalid distribution"

Проверьте:
- Все файлы включены в `MANIFEST.in`
- `pyproject.toml` корректен
- Нет синтаксических ошибок

### Ошибка при установке: "No matching distribution"

Убедитесь, что используете правильный URL:
```bash
pip install --index-url https://test.pypi.org/simple/ --extra-index-url https://pypi.org/simple/ pyfastcsv
```

### Ошибка компиляции при установке

Это нормально для TestPyPI - wheel может быть не собран для вашей платформы. 
Проверьте, что source distribution (`tar.gz`) установился и скомпилировался.

## Следующие шаги

После успешной проверки на TestPyPI:

1. Следуйте инструкциям в `PUBLISH.md` для публикации на основной PyPI
2. Или используйте GitHub Actions для автоматической публикации при создании Release

## Полезные ссылки

- [TestPyPI](https://test.pypi.org/)
- [TestPyPI Account Settings](https://test.pypi.org/manage/account/)
- [Python Packaging Guide](https://packaging.python.org/)
- [Twine Documentation](https://twine.readthedocs.io/)

