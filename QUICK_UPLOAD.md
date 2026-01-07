# Быстрая загрузка на TestPyPI

Пакет уже собран! Файлы находятся в папке `dist/`.

## Загрузка на TestPyPI

Выполните команду:

```bash
python -m twine upload --repository testpypi dist/*
```

При запросе учетных данных:
- **Username**: `__token__`
- **Password**: ваш API токен от TestPyPI (начинается с `pypi-`)

## Если у вас нет токена

1. Перейдите на https://test.pypi.org/manage/account/token/
2. Нажмите "Add API token"
3. Заполните:
   - Token name: `FastCSV TestPyPI`
   - Scope: выберите "Entire account" или "Project: pyfastcsv"
4. Скопируйте токен (начинается с `pypi-`)

## После загрузки

Проверьте пакет на https://test.pypi.org/project/pyfastcsv/

## Тестовая установка

```bash
pip install --index-url https://test.pypi.org/simple/ --extra-index-url https://pypi.org/simple/ pyfastcsv
```

