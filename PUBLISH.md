# Инструкции по публикации FastCSV в PyPI

## Подготовка

1. **Убедитесь, что все тесты проходят:**
   ```bash
   python -m pytest tests/ -v
   ```

2. **Проверьте версию в файлах:**
   - `pyproject.toml` - версия должна быть актуальной
   - `fastcsv/__init__.py` - `__version__` должен совпадать с версией в `pyproject.toml`

3. **Обновите CHANGELOG (если есть) или README.md** с описанием изменений

## Сборка пакета

1. **Установите необходимые инструменты:**
   ```bash
   pip install build twine
   ```

2. **Очистите старые сборки:**
   ```bash
   rm -rf dist/ build/ *.egg-info
   ```

3. **Соберите пакет:**
   ```bash
   python -m build
   ```

   Это создаст файлы в директории `dist/`:
   - `fastcsv-0.2.0.tar.gz` (source distribution)
   - `fastcsv-0.2.0-*.whl` (wheel)

## Проверка пакета

1. **Проверьте пакет с помощью twine:**
   ```bash
   twine check dist/*
   ```

2. **Проверьте содержимое пакета:**
   ```bash
   # Проверка source distribution
   tar -tzf dist/fastcsv-0.2.0.tar.gz
   
   # Проверка wheel
   unzip -l dist/fastcsv-0.2.0-*.whl
   ```

## Тестовая публикация (TestPyPI)

**ВАЖНО:** Сначала опубликуйте на TestPyPI для проверки!

1. **Создайте аккаунт на TestPyPI:**
   - Перейдите на https://test.pypi.org/
   - Зарегистрируйтесь или войдите

2. **Создайте API токен:**
   - Перейдите в Account settings → API tokens
   - Создайте новый токен с scope "Entire account" или "Project: fastcsv"

3. **Опубликуйте на TestPyPI:**
   ```bash
   twine upload --repository testpypi dist/*
   ```
   
   Введите ваши учетные данные TestPyPI при запросе.

4. **Проверьте установку из TestPyPI:**
   ```bash
   pip install --index-url https://test.pypi.org/simple/ fastcsv
   ```

5. **Протестируйте установленный пакет:**
   ```bash
   python -c "import fastcsv; print(fastcsv.__version__)"
   ```

## Публикация на PyPI

**ВАЖНО:** Публикуйте на PyPI только после успешной проверки на TestPyPI!

1. **Создайте аккаунт на PyPI:**
   - Перейдите на https://pypi.org/
   - Зарегистрируйтесь или войдите

2. **Создайте API токен:**
   - Перейдите в Account settings → API tokens
   - Создайте новый токен с scope "Entire account" или "Project: fastcsv"

3. **Опубликуйте на PyPI:**
   ```bash
   twine upload dist/*
   ```
   
   Введите ваши учетные данные PyPI при запросе.

4. **Проверьте установку из PyPI:**
   ```bash
   pip install fastcsv
   ```

5. **Протестируйте установленный пакет:**
   ```bash
   python -c "import fastcsv; print(fastcsv.__version__)"
   ```

## Обновление версии

При выпуске новой версии:

1. **Обновите версию в `pyproject.toml`:**
   ```toml
   version = "0.2.1"  # или другая версия
   ```

2. **Обновите версию в `fastcsv/__init__.py`:**
   ```python
   __version__ = "0.2.1"
   ```

3. **Обновите CHANGELOG в README.md**

4. **Соберите и опубликуйте заново**

## Использование API токенов (рекомендуется)

Вместо ввода пароля каждый раз, используйте API токены:

1. **Создайте файл `~/.pypirc`:**
   ```ini
   [distutils]
   index-servers =
       pypi
       testpypi

   [pypi]
   username = __token__
   password = pypi-ваш-токен-здесь

   [testpypi]
   username = __token__
   password = pypi-ваш-тестовый-токен-здесь
   ```

2. **Или используйте переменные окружения:**
   ```bash
   export TWINE_USERNAME=__token__
   export TWINE_PASSWORD=pypi-ваш-токен-здесь
   ```

3. **Или передайте токен напрямую:**
   ```bash
   twine upload --username __token__ --password pypi-ваш-токен-здесь dist/*
   ```

## Troubleshooting

### Ошибка: "File already exists"
Если версия уже существует на PyPI, обновите версию в `pyproject.toml` и `__init__.py`.

### Ошибка: "Invalid distribution"
Убедитесь, что все необходимые файлы включены в пакет. Проверьте `MANIFEST.in` если он есть.

### Ошибка компиляции C++
Убедитесь, что установлены все зависимости:
- CMake 3.15+
- C++ компилятор с поддержкой C++17
- pybind11 2.10+

## Полезные ссылки

- [PyPI](https://pypi.org/)
- [TestPyPI](https://test.pypi.org/)
- [Python Packaging User Guide](https://packaging.python.org/)
- [Twine Documentation](https://twine.readthedocs.io/)


