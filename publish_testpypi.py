#!/usr/bin/env python3
"""
Скрипт для публикации FastCSV на TestPyPI.

Использование:
    python publish_testpypi.py

Требования:
    - pip install build twine
    - Аккаунт на TestPyPI (https://test.pypi.org/)
    - API токен от TestPyPI
"""

import os
import shutil
import subprocess
import sys
from pathlib import Path

def run_command(cmd, check=True):
    """Выполнить команду и показать вывод."""
    print(f"\n{'='*60}")
    print(f"Выполняю: {' '.join(cmd)}")
    print('='*60)
    result = subprocess.run(cmd, check=check)
    if result.returncode != 0:
        print(f"\n❌ Ошибка при выполнении команды!")
        sys.exit(1)
    return result

def check_requirements():
    """Проверить наличие необходимых инструментов."""
    print("[*] Проверяю наличие необходимых инструментов...")
    
    required = ['build', 'twine']
    missing = []
    
    for package in required:
        try:
            result = subprocess.run(
                [sys.executable, '-m', 'pip', 'show', package],
                capture_output=True,
                text=True
            )
            if result.returncode != 0:
                missing.append(package)
        except Exception:
            missing.append(package)
    
    if missing:
        print(f"\n[!] Отсутствуют необходимые пакеты: {', '.join(missing)}")
        print(f"[*] Устанавливаю их...")
        try:
            subprocess.run(
                [sys.executable, '-m', 'pip', 'install'] + missing,
                check=True
            )
            print("[+] Пакеты успешно установлены")
        except subprocess.CalledProcessError:
            print(f"\n[!] Не удалось установить пакеты автоматически.")
            print(f"Установите их вручную командой: pip install {' '.join(missing)}")
            sys.exit(1)
    else:
        print("[+] Все необходимые инструменты установлены")

def clean_build():
    """Очистить старые сборки."""
    print("\n[*] Очищаю старые сборки...")
    
    dirs_to_remove = ['dist', 'build']
    files_to_remove = ['*.egg-info']
    
    for dir_name in dirs_to_remove:
        dir_path = Path(dir_name)
        if dir_path.exists():
            shutil.rmtree(dir_path)
            print(f"  Удалено: {dir_name}/")
    
    for pattern in files_to_remove:
        for path in Path('.').glob(pattern):
            if path.is_dir():
                shutil.rmtree(path)
                print(f"  Удалено: {path}/")

def build_package():
    """Собрать пакет."""
    print("\n[*] Собираю пакет...")
    run_command([sys.executable, '-m', 'build'])

def check_package():
    """Проверить пакет с помощью twine."""
    print("\n[*] Проверяю пакет...")
    run_command([sys.executable, '-m', 'twine', 'check', 'dist/*'])

def upload_to_testpypi():
    """Загрузить пакет на TestPyPI."""
    print("\n[*] Загружаю пакет на TestPyPI...")
    print("\n[!] ВАЖНО: Вам нужно будет ввести:")
    print("   - Username: __token__")
    print("   - Password: ваш API токен от TestPyPI (начинается с pypi-)")
    print("\n   Если у вас нет токена, создайте его на:")
    print("   https://test.pypi.org/manage/account/token/")
    
    input("\nНажмите Enter, чтобы продолжить...")
    
    run_command([
        sys.executable, '-m', 'twine', 'upload',
        '--repository', 'testpypi',
        'dist/*'
    ])

def main():
    """Основная функция."""
    print("="*60)
    print("FastCSV - Публикация на TestPyPI")
    print("="*60)
    
    # Проверка требований
    check_requirements()
    
    # Очистка старых сборок
    clean_build()
    
    # Сборка пакета
    build_package()
    
    # Проверка пакета
    check_package()
    
    # Загрузка на TestPyPI
    upload_to_testpypi()
    
    print("\n" + "="*60)
    print("[+] Публикация на TestPyPI завершена!")
    print("="*60)
    print("\n[*] Следующие шаги:")
    print("   1. Проверьте пакет на https://test.pypi.org/project/pyfastcsv/")
    print("   2. Протестируйте установку:")
    print("      pip install --index-url https://test.pypi.org/simple/ pyfastcsv")
    print("   3. Если все работает, можно публиковать на основной PyPI")

if __name__ == '__main__':
    main()

