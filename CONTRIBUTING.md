# Contributing to ESP32 SIP Door Station

Thank you for your interest in contributing to this project! This document provides guidelines for contributing.

## Development Setup

1. **Fork and clone the repository**
2. **Set up ESP-IDF environment** (see README.md)
3. **Verify your setup** with `verify_build.bat` or `./verify_build.sh`

## Code Style

### C/C++ Code
- Follow [ESP-IDF coding standards](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/contribute/style-guide.html)
- Use 4 spaces for indentation
- Maximum line length: 120 characters
- Use descriptive variable and function names

### Example:
```c
#include "esp_log.h"

static const char *TAG = "component_name";

esp_err_t my_function(int parameter)
{
    if (parameter < 0) {
        ESP_LOGE(TAG, "Invalid parameter: %d", parameter);
        return ESP_ERR_INVALID_ARG;
    }
    
    ESP_LOGI(TAG, "Processing parameter: %d", parameter);
    return ESP_OK;
}
```

### Documentation
- Document all public functions with Doxygen comments
- Update README.md for significant changes
- Add inline comments for complex logic

## Testing

### Unit Tests
- Add unit tests for new functionality
- Use Unity testing framework
- Place tests in `test/` directory
- Run tests: `idf.py -T test build flash monitor`

### Hardware Testing
- Test on actual ESP32-S3 hardware
- Verify Wi-Fi connectivity
- Test audio functionality
- Validate SIP integration

## Commit Guidelines

### Commit Messages
Use conventional commit format:
```
type(scope): description

[optional body]

[optional footer]
```

**Types:**
- `feat`: New feature
- `fix`: Bug fix
- `docs`: Documentation changes
- `style`: Code style changes
- `refactor`: Code refactoring
- `test`: Adding or updating tests
- `chore`: Maintenance tasks

**Examples:**
```
feat(sip): add SIP registration functionality

fix(wifi): resolve connection timeout issue

docs(readme): update installation instructions
```

### Branch Naming
- `feature/description` - New features
- `fix/description` - Bug fixes
- `docs/description` - Documentation updates
- `refactor/description` - Code refactoring

## Pull Request Process

1. **Create a feature branch** from `main`
2. **Make your changes** following the guidelines above
3. **Add/update tests** as needed
4. **Update documentation** if required
5. **Ensure all tests pass**
6. **Create a pull request** with:
   - Clear title and description
   - Reference any related issues
   - Screenshots/videos for UI changes
   - Test results

### PR Checklist
- [ ] Code follows project style guidelines
- [ ] Self-review completed
- [ ] Tests added/updated and passing
- [ ] Documentation updated
- [ ] No merge conflicts
- [ ] Builds successfully on target hardware

## Issue Reporting

### Bug Reports
Include:
- ESP-IDF version
- Hardware details (ESP32-S3 variant, audio codec, etc.)
- Steps to reproduce
- Expected vs actual behavior
- Relevant logs/error messages
- Configuration details

### Feature Requests
Include:
- Clear description of the feature
- Use case and benefits
- Proposed implementation approach
- Any relevant examples or references

## Code Review

All contributions require code review. Reviews focus on:
- Code correctness and efficiency
- Adherence to style guidelines
- Test coverage
- Documentation quality
- Security considerations

## Getting Help

- 📖 Check existing documentation
- 🔍 Search existing issues
- 💬 Start a discussion for questions
- 🐛 Create an issue for bugs

## Recognition

Contributors will be acknowledged in:
- README.md contributors section
- Release notes for significant contributions
- Project documentation

Thank you for contributing! 🎉