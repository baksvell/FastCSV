# FastCSV

High-performance CSV parsing library for Python with SIMD optimizations (AVX2/SSE4.2).

## Features

- **🚀 High Performance**: Up to 7x faster than Python's standard `csv` module for large files
- **🔌 Drop-in Replacement**: Full compatibility with Python's `csv` module API
- **⚡ SIMD Optimizations**: Uses AVX2/SSE4.2 instructions for maximum performance
- **📦 Batch Processing**: Efficient handling of large CSV files with memory-mapped I/O
- **🎯 Dialect Support**: Full support for CSV dialects (register_dialect, get_dialect, list_dialects)
- **🔍 Sniffer**: Automatic format detection
- **📚 Standard Dialects**: Built-in support for excel, excel-tab, unix dialects

## Installation

### From PyPI (Recommended)

```bash
pip install pyfastcsv
```

### From Source

See [INSTALL.md](INSTALL.md) for detailed installation instructions, including platform-specific requirements.

**Quick start:**
```bash
git clone https://github.com/baksvell/FastCSV.git
cd FastCSV
pip install -e .
```

## Quick Start

### Basic Usage

```python
import fastcsv

# Read CSV file
with open('data.csv', 'r') as f:
    reader = fastcsv.reader(f)
    for row in reader:
        print(row)

# Write CSV file
with open('output.csv', 'w', newline='') as f:
    writer = fastcsv.writer(f)
    writer.writerow(['Name', 'Age', 'City'])
    writer.writerow(['John', '30', 'New York'])
```

### Dictionary Reader/Writer

```python
import fastcsv

# Read as dictionary
with open('data.csv', 'r') as f:
    reader = fastcsv.DictReader(f)
    for row in reader:
        print(row['Name'], row['Age'])

# Write from dictionary
with open('output.csv', 'w', newline='') as f:
    fieldnames = ['Name', 'Age', 'City']
    writer = fastcsv.DictWriter(f, fieldnames=fieldnames)
    writer.writeheader()
    writer.writerow({'Name': 'John', 'Age': '30', 'City': 'New York'})
```

### Memory-Mapped Reader (for large files)

```python
import fastcsv

# Efficient reading of large files
reader = fastcsv.mmap_reader('large_file.csv')
for row in reader:
    print(row)
```

### Custom Dialects

```python
import fastcsv

# Register custom dialect
fastcsv.register_dialect('semicolon', delimiter=';', quotechar='"')

# Use custom dialect
with open('data.csv', 'r') as f:
    reader = fastcsv.reader(f, dialect='semicolon')
    for row in reader:
        print(row)
```

### Automatic Format Detection

```python
import fastcsv

# Detect CSV format
with open('data.csv', 'rb') as f:
    sample = f.read(1024)
    dialect = fastcsv.Sniffer().sniff(sample.decode('utf-8', errors='ignore'))
    
    f.seek(0)
    reader = fastcsv.reader(f, dialect=dialect)
    for row in reader:
        print(row)
```

## Performance

FastCSV is optimized for performance, especially with large files:

- **Small files (100 rows)**: ~1.5x faster
- **Medium files (1000 rows)**: ~1.5x faster
- **Large files (10000 rows)**: Up to 7x faster

Performance may vary depending on your hardware and CSV file structure.

## Requirements

- Python 3.10+
- C++ compiler with C++17 support (GCC, Clang, or MSVC)
- CMake 3.15+
- pybind11 2.10+

> **Note**: For detailed installation instructions and troubleshooting, see [INSTALL.md](INSTALL.md)

## API Compatibility

FastCSV provides full compatibility with Python's standard `csv` module:

- `fastcsv.reader()` - equivalent to `csv.reader()`
- `fastcsv.writer()` - equivalent to `csv.writer()`
- `fastcsv.DictReader()` - equivalent to `csv.DictReader()`
- `fastcsv.DictWriter()` - equivalent to `csv.DictWriter()`
- `fastcsv.register_dialect()` - equivalent to `csv.register_dialect()`
- `fastcsv.get_dialect()` - equivalent to `csv.get_dialect()`
- `fastcsv.list_dialects()` - equivalent to `csv.list_dialects()`
- `fastcsv.Sniffer()` - equivalent to `csv.Sniffer()`

## Additional Features

### Memory-Mapped Reader

For very large files, use the memory-mapped reader:

```python
import fastcsv

reader = fastcsv.mmap_reader('large_file.csv')
for row in reader:
    process(row)
```

This uses memory-mapped I/O for efficient handling of files that don't fit in memory.

## More Examples

For comprehensive examples and use cases, see [EXAMPLES.md](EXAMPLES.md).

## Troubleshooting

### Installation Issues

**Problem: "CMake not found"**
- Solution: Install CMake 3.15+ from https://cmake.org/download/

**Problem: "C++ compiler not found" (Windows)**
- Solution: Install Visual Studio Build Tools from https://visualstudio.microsoft.com/downloads/

**Problem: "pybind11 not found"**
- Solution: `pip install pybind11`

**Problem: Build fails with SIMD errors**
- Solution: Your CPU might not support AVX2/SSE4.2. The code should fall back gracefully, but if not, check your CPU capabilities.

### Runtime Issues

**Problem: "ImportError: cannot import name '_native'"**
- Solution: The native module wasn't built. Run `pip install -e .` to rebuild.

**Problem: Performance is not as expected**
- Solution: Ensure your CPU supports AVX2/SSE4.2. Check with: `python -c "import fastcsv; print(fastcsv.__version__)"` after `pip install pyfastcsv`

## License

MIT License

## Contributing

Contributions are welcome! Please see [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines.

- 🐛 **Report bugs**: [GitHub Issues](https://github.com/baksvell/FastCSV/issues)
- 💡 **Request features**: [GitHub Issues](https://github.com/baksvell/FastCSV/issues)
- 📝 **Submit PRs**: [GitHub Pull Requests](https://github.com/baksvell/FastCSV/pulls)

## Changelog

### 0.2.0
- Fixed bug with quoted fields in multi-line buffers
- Performance optimizations
- Improved error handling

### 0.1.0
- Initial release
- Basic CSV parsing functionality
- SIMD optimizations
- Dialect support
- Sniffer implementation
