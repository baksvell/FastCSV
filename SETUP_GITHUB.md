# Настройка GitHub репозитория

Этот файл содержит инструкции по настройке GitHub репозитория для FastCSV.

## Инициализация Git репозитория

Если репозиторий еще не инициализирован:

```bash
# Инициализация Git
git init

# Добавление удаленного репозитория
git remote add origin git@github.com:baksvell/FastCSV.git

# Или через HTTPS
git remote add origin https://github.com/baksvell/FastCSV.git
```

## Первый коммит

```bash
# Добавить все файлы
git add .

# Создать первый коммит
git commit -m "Initial commit: FastCSV v0.2.0"

# Отправить в репозиторий
git branch -M main
git push -u origin main
```

## Настройка GitHub Actions

### 1. Создать секреты для PyPI

1. Перейдите в Settings → Secrets and variables → Actions
2. Добавьте новый секрет:
   - Name: `PYPI_API_TOKEN`
   - Value: ваш API токен из PyPI

### 2. Проверить workflows

Workflows уже настроены в `.github/workflows/`:
- `build-wheels.yml` - автоматическая сборка wheels при релизе
- `tests.yml` - автоматические тесты при push/PR

### 3. Тестирование workflows

Workflows запустятся автоматически при:
- Push в main/develop ветки (tests)
- Создании Pull Request (tests)
- Создании Release (build wheels)

## Настройка GitHub Pages (опционально)

Если хотите создать документацию:

1. Settings → Pages
2. Source: GitHub Actions
3. Создать workflow для генерации документации

## Настройка веток

Рекомендуемая структура:
- `main` - стабильная версия
- `develop` - разработка
- `feature/*` - новые функции
- `fix/*` - исправления багов

## Защита веток

Рекомендуется защитить `main` ветку:

1. Settings → Branches
2. Add rule для `main`
3. Включить:
   - Require pull request reviews
   - Require status checks to pass
   - Require branches to be up to date

## Labels для Issues

Рекомендуемые labels:
- `bug` - баги
- `enhancement` - улучшения
- `feature` - новые функции
- `documentation` - документация
- `performance` - производительность
- `help wanted` - нужна помощь
- `good first issue` - для новичков

## Release процесс

1. Обновить версию в `pyproject.toml` и `fastcsv/__init__.py`
2. Обновить CHANGELOG в README.md
3. Создать commit: `git commit -m "Bump version to X.Y.Z"`
4. Создать tag: `git tag -a vX.Y.Z -m "Release vX.Y.Z"`
5. Push: `git push origin main --tags`
6. Создать Release на GitHub (это запустит build-wheels workflow)

## Проверка перед публикацией

- [ ] Все тесты проходят
- [ ] Версия обновлена в обоих файлах
- [ ] CHANGELOG обновлен
- [ ] README.md актуален
- [ ] GitHub Actions настроены
- [ ] PyPI токен добавлен в секреты


