#!/bin/bash

# ESP-IDF Build System Verification Script
# Run this script after setting up ESP-IDF environment

echo "ESP32 SIP Door Station - Build System Verification"
echo "=================================================="

# Check ESP-IDF environment
if [ -z "$IDF_PATH" ]; then
    echo "ERROR: ESP-IDF environment not set. Please run 'get_idf' or source export.sh"
    exit 1
fi

echo "ESP-IDF Path: $IDF_PATH"

# Check idf.py availability
if ! command -v idf.py &> /dev/null; then
    echo "ERROR: idf.py not found in PATH"
    exit 1
fi

echo "idf.py version:"
idf.py --version

# Verify project structure
echo ""
echo "Verifying project structure..."
required_dirs=("main" "test" "web_root")
for dir in "${required_dirs[@]}"; do
    if [ -d "$dir" ]; then
        echo "✓ $dir/ directory exists"
    else
        echo "✗ $dir/ directory missing"
        exit 1
    fi
done

required_files=("CMakeLists.txt" "sdkconfig.defaults" "main/CMakeLists.txt" "main/app_main.c" "test/CMakeLists.txt" "test/test_main.c")
for file in "${required_files[@]}"; do
    if [ -f "$file" ]; then
        echo "✓ $file exists"
    else
        echo "✗ $file missing"
        exit 1
    fi
done

# Test build configuration
echo ""
echo "Testing build configuration..."
idf.py reconfigure

if [ $? -eq 0 ]; then
    echo "✓ Build configuration successful"
else
    echo "✗ Build configuration failed"
    exit 1
fi

# Test main application build
echo ""
echo "Testing main application build..."
idf.py build

if [ $? -eq 0 ]; then
    echo "✓ Main application build successful"
else
    echo "✗ Main application build failed"
    exit 1
fi

# Test Unity test framework build
echo ""
echo "Testing Unity test framework build..."
idf.py -T test build

if [ $? -eq 0 ]; then
    echo "✓ Unity test framework build successful"
else
    echo "✗ Unity test framework build failed"
    exit 1
fi

echo ""
echo "All verification tests passed! ✓"
echo "Project structure and build system are properly configured."