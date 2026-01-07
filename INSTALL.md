# Installation Guide for FastCSV

This guide provides detailed instructions for installing FastCSV on different platforms.

## Prerequisites

Before installing FastCSV, ensure you have:

- **Python 3.10 or higher**
- **C++ compiler** with C++17 support
- **CMake 3.15 or higher**
- **pybind11 2.10 or higher**

## Quick Installation

If you have all prerequisites installed:

```bash
pip install pyfastcsv
```

## Platform-Specific Instructions

### Windows

#### Option 1: Using Visual Studio Build Tools (Recommended)

1. **Install Visual Studio Build Tools:**
   - Download from: https://visualstudio.microsoft.com/downloads/
   - Select "Desktop development with C++" workload
   - Or install "Build Tools for Visual Studio"

2. **Install CMake:**
   - Download from: https://cmake.org/download/
   - Or use: `choco install cmake` (if you have Chocolatey)

3. **Install FastCSV:**
   ```bash
   pip install pyfastcsv
   ```

#### Option 2: Using MinGW-w64

1. **Install MinGW-w64:**
   - Download from: https://www.mingw-w64.org/downloads/
   - Or use MSYS2: `pacman -S mingw-w64-x86_64-gcc cmake`

2. **Add to PATH:**
   - Add MinGW bin directory to your PATH

3. **Install FastCSV:**
   ```bash
   pip install pyfastcsv
   ```

### Linux

#### Ubuntu/Debian

```bash
# Install build tools
sudo apt-get update
sudo apt-get install -y build-essential cmake python3-dev

# Install FastCSV
pip install pyfastcsv
```

#### Fedora/RHEL/CentOS

```bash
# Install build tools
sudo dnf install gcc-c++ cmake python3-devel

# Install FastCSV
pip install pyfastcsv
```

#### Arch Linux

```bash
# Install build tools
sudo pacman -S base-devel cmake python

# Install FastCSV
pip install pyfastcsv
```

### macOS

#### Using Homebrew (Recommended)

```bash
# Install build tools
brew install cmake

# Install FastCSV
pip install pyfastcsv
```

#### Using Xcode Command Line Tools

```bash
# Install Xcode Command Line Tools
xcode-select --install

# Install CMake (if not using Homebrew)
# Download from https://cmake.org/download/

# Install FastCSV
pip install pyfastcsv
```

## Building from Source

If you want to build from source or contribute:

```bash
# Clone the repository
git clone https://github.com/baksvell/FastCSV.git
cd FastCSV

# Install build dependencies
pip install build wheel cmake pybind11 setuptools

# Build the package
python -m build

# Install the built package
pip install dist/pyfastcsv-*.whl
```

Or install in development mode:

```bash
pip install -e .
```

## Verification

After installation, verify that FastCSV works:

```bash
python -c "import fastcsv; print(f'FastCSV version: {fastcsv.__version__}')"
```

You should see:
```
FastCSV version: 0.2.0
```

## Troubleshooting

### Common Issues

#### "CMake not found"

**Solution:**
- Install CMake from https://cmake.org/download/
- Or use package manager: `apt-get install cmake`, `brew install cmake`, etc.
- Verify: `cmake --version`

#### "C++ compiler not found"

**Windows:**
- Install Visual Studio Build Tools
- Or install MinGW-w64
- Verify: `gcc --version` or `cl` (Visual Studio)

**Linux:**
- Install build-essential: `sudo apt-get install build-essential`
- Verify: `gcc --version`

**macOS:**
- Install Xcode Command Line Tools: `xcode-select --install`
- Verify: `gcc --version`

#### "pybind11 not found"

**Solution:**
```bash
pip install pybind11
```

#### "Cannot import _native"

**Solution:**
- The native module wasn't built correctly
- Try rebuilding: `pip install --force-reinstall --no-cache-dir fastcsv`
- Or build from source: `pip install -e .`

#### Build fails with SIMD-related errors

**Solution:**
- Your CPU might not support AVX2/SSE4.2
- The code should fall back gracefully
- If errors persist, check your CPU capabilities
- You can disable SIMD optimizations (requires code modification)

#### "Permission denied" during installation

**Solution:**
- Use `pip install --user fastcsv` to install for current user only
- Or use virtual environment: `python -m venv venv && source venv/bin/activate`

## Using Virtual Environments (Recommended)

It's recommended to use virtual environments:

```bash
# Create virtual environment
python -m venv venv

# Activate (Windows)
venv\Scripts\activate

# Activate (Linux/macOS)
source venv/bin/activate

# Install FastCSV
pip install pyfastcsv
```

## Docker Installation

If you prefer using Docker:

```dockerfile
FROM python:3.11-slim

# Install build dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    && rm -rf /var/lib/apt/lists/*

# Install FastCSV
RUN pip install pyfastcsv
```

## Getting Help

If you encounter issues:

1. Check the [Troubleshooting section](#troubleshooting)
2. Search [GitHub Issues](https://github.com/baksvell/FastCSV/issues)
3. Create a new issue with:
   - Your OS and version
   - Python version: `python --version`
   - CMake version: `cmake --version`
   - Compiler version: `gcc --version` or `cl`
   - Full error message


