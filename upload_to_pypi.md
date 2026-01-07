# Инструкция по загрузке на PyPI

Если при загрузке всех файлов сразу возникает ошибка соединения, попробуйте загрузить файлы по одному:

## Вариант 1: Загрузить все файлы сразу

```bash
python -m twine upload dist/*
```

## Вариант 2: Загрузить файлы по одному (если есть проблемы с соединением)

### Шаг 1: Загрузите source distribution (tar.gz)

```bash
python -m twine upload dist\fastcsv-0.2.0.tar.gz
```

### Шаг 2: Загрузите wheel

```bash
python -m twine upload dist\fastcsv-0.2.0-cp312-cp312-win_amd64.whl
```

## При запросе учетных данных:

- **Username**: `__token__`
- **Password**: ваш API токен от PyPI (начинается с `pypi-`)

## После успешной загрузки:

1. Проверьте пакет на https://pypi.org/project/fastcsv/
2. Протестируйте установку: `pip install fastcsv`
3. Проверьте версию: `python -c "import fastcsv; print(fastcsv.__version__)"`

