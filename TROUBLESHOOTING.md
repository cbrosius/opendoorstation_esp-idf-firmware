# ESP32 SIP Door Station - Troubleshooting Guide

## Common Build Issues and Solutions

### 1. NVS Encryption eFuse Error

**Error:**
```
error: #error "NVS Encryption (HMAC): Configured eFuse block (CONFIG_NVS_SEC_HMAC_EFUSE_KEY_ID) out of range!"
```

**Solutions (try in order):**

1. **Complete build cleanup** (recommended first step):
   ```cmd
   clean_build.bat
   ```

2. **Switch to minimal configuration** (if cleanup doesn't work):
   ```cmd
   use_minimal_config.bat
   ```

3. **Manual cleanup** (if scripts don't work):
   ```cmd
   rmdir /s /q build
   del sdkconfig
   del sdkconfig.old
   idf.py reconfigure
   idf.py build
   ```

**Root Cause:** Cached configuration files may still contain old NVS encryption settings even after updating sdkconfig.defaults.

### 2. ESP-IDF Environment Issues

**Error:**
```
No module named 'esp_idf_monitor'
```

**Solution:**
1. Run the ESP-IDF installation: `install_esp_idf.bat`
2. Activate environment: `setup_env.bat`
3. Or manually: `C:\Users\Christian\esp\v5.4.2\esp-idf\export.bat`

### 3. Build Configuration Errors

**Error:**
```
Build configuration failed
```

**Solutions:**
1. Clean build artifacts: `idf.py fullclean`
2. Reconfigure: `idf.py reconfigure`
3. Check configuration: `idf.py menuconfig`
4. Verify sdkconfig.defaults settings

### 4. Component Build Failures

**Error:**
```
ninja: build stopped: subcommand failed
```

**Solutions:**
1. Check specific error messages in build logs
2. Clean and rebuild: `idf.py fullclean && idf.py build`
3. Check component dependencies in CMakeLists.txt files
4. Verify ESP32-S3 target configuration

### 5. Python Virtual Environment Issues

**Error:**
```
ESP-IDF Python virtual environment not found
```

**Solutions:**
1. Run: `C:\Users\Christian\esp\v5.4.2\esp-idf\install.bat`
2. If that fails, delete `C:\Users\Christian\.espressif` and reinstall
3. Check Python installation and version compatibility

## Build Process Steps

### Clean Build Process
```cmd
idf.py fullclean
idf.py reconfigure
idf.py build
```

### Configuration Management
```cmd
idf.py menuconfig          # Interactive configuration
idf.py save-defconfig      # Save current config as defconfig
```

### Testing
```cmd
idf.py -T test build       # Build test target
idf.py -T test flash monitor  # Flash and run tests
```

## Configuration Files

### sdkconfig.defaults
Contains default project configuration:
- ESP32-S3 target settings
- Unity test framework
- Logging configuration
- Wi-Fi and HTTP server settings
- NVS configuration (encryption disabled)

### CMakeLists.txt Files
- **Root CMakeLists.txt**: Project-level configuration
- **main/CMakeLists.txt**: Main component configuration
- **test/CMakeLists.txt**: Test component with Unity integration

## Getting Help

### Build Logs
Check detailed build logs in:
- `build/log/idf_py_stdout_output_*`
- `build/log/idf_py_stderr_output_*`

### ESP-IDF Documentation
- [ESP-IDF Programming Guide](https://docs.espressif.com/projects/esp-idf/en/v5.4.2/)
- [ESP32-S3 Technical Reference](https://www.espressif.com/sites/default/files/documentation/esp32-s3_technical_reference_manual_en.pdf)

### Project Structure
Run `verify_build.bat` to check project setup and build system configuration.