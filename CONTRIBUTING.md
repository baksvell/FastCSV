# Contributing to FastCSV

Thank you for your interest in contributing to FastCSV! This document provides guidelines and instructions for contributing.

## Getting Started

1. **Fork the repository** on GitHub
2. **Clone your fork** locally:
   ```bash
   git clone https://github.com/yourusername/FastCSV.git
   cd FastCSV
   ```
3. **Create a branch** for your changes:
   ```bash
   git checkout -b feature/your-feature-name
   ```

## Development Setup

1. **Install development dependencies:**
   ```bash
   pip install -e ".[dev]"
   ```

2. **Run tests** to ensure everything works:
   ```bash
   pytest tests/ -v
   ```

3. **Run benchmarks** to check performance:
   ```bash
   python benchmark_performance.py
   ```

## Making Changes

### Code Style

- Follow PEP 8 for Python code
- Use meaningful variable and function names
- Add comments for complex logic
- Keep functions focused and small

### C++ Code

- Follow C++17 standards
- Use meaningful names
- Add comments for SIMD optimizations
- Ensure compatibility with different compilers

### Testing

- Add tests for new features
- Ensure all tests pass: `pytest tests/ -v`
- Test edge cases
- Test on different platforms if possible

### Documentation

- Update README.md if adding new features
- Add docstrings to new functions/classes
- Update examples if API changes

## Submitting Changes

1. **Commit your changes:**
   ```bash
   git add .
   git commit -m "Description of your changes"
   ```

2. **Push to your fork:**
   ```bash
   git push origin feature/your-feature-name
   ```

3. **Create a Pull Request** on GitHub:
   - Provide a clear description of changes
   - Reference any related issues
   - Include test results if applicable

## Pull Request Guidelines

- **Title**: Clear and descriptive
- **Description**: Explain what and why
- **Tests**: All tests must pass
- **Documentation**: Update if needed
- **Examples**: Add if introducing new features

## Areas for Contribution

### High Priority

- Performance optimizations
- Bug fixes
- Documentation improvements
- Test coverage
- Pre-built wheels for more platforms

### Medium Priority

- Additional features
- Better error messages
- More examples
- Performance benchmarks

### Low Priority

- Code style improvements
- Refactoring
- Documentation formatting

## Reporting Bugs

When reporting bugs, please include:

1. **Description**: What happened?
2. **Expected behavior**: What should happen?
3. **Steps to reproduce**: How to reproduce?
4. **Environment**:
   - OS and version
   - Python version
   - FastCSV version
   - CMake version
   - Compiler version
5. **Error messages**: Full traceback if available

## Requesting Features

When requesting features, please include:

1. **Use case**: Why is this feature needed?
2. **Proposed solution**: How should it work?
3. **Alternatives**: Other solutions considered?
4. **Examples**: Code examples if applicable

## Code of Conduct

- Be respectful and inclusive
- Welcome newcomers
- Focus on constructive feedback
- Help others learn

## Questions?

- Open an issue on GitHub
- Check existing issues and discussions
- Review the documentation

Thank you for contributing to FastCSV! 🚀
