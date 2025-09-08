# ESP32 SIP Door Station - Setup Guide

## ESP-IDF Environment Setup

### Prerequisites
- ESP-IDF v5.4.2 installed at: `C:\Users\Christian\esp\v5.4.2\esp-idf`

### First-Time Setup (Required)

If you see the error "ESP-IDF Python virtual environment not found", you need to run the installation script first:

```cmd
C:\Users\Christian\esp\v5.4.2\esp-idf\install.bat
```

This will:
- Download and install the required Python packages
- Set up the Python virtual environment
- Install ESP-IDF tools and dependencies

**Note:** This step only needs to be done once after installing ESP-IDF.

### Step-by-Step Setup Process

#### Step 1: Install ESP-IDF Tools (First Time Only)

Run the installation helper:
```cmd
install_esp_idf.bat
```

Or manually run:
```cmd
C:\Users\Christian\esp\v5.4.2\esp-idf\install.bat
```

This will download and install all required tools and Python packages.

#### Step 2: Activate ESP-IDF Environment

After installation, you have several options to activate the ESP-IDF environment:

#### Option 1: ESP-IDF Command Prompt (Recommended)
1. Open Start Menu
2. Search for "ESP-IDF 5.4.2 CMD"
3. Click to open the ESP-IDF Command Prompt
4. Navigate to your project directory
5. Run the verification script: `verify_build.bat`

#### Option 2: Manual Activation
1. Open regular Command Prompt
2. Run the export script:
   ```cmd
   C:\Users\Christian\esp\v5.4.2\esp-idf\export.bat
   ```
3. Navigate to your project directory
4. Run the verification script: `verify_build.bat`

#### Option 3: PowerShell (Alternative)
1. Open PowerShell
2. Run:
   ```powershell
   & "C:\Users\Christian\esp\v5.4.2\esp-idf\export.ps1"
   ```
3. Navigate to your project directory
4. Run the verification script: `verify_build.bat`

### Verifying Setup

After activating the ESP-IDF environment, run:
```cmd
verify_build.bat
```

This will:
- ✅ Check ESP-IDF environment variables
- ✅ Verify idf.py is working
- ✅ Validate project structure
- ✅ Test build configuration
- ✅ Build main application
- ✅ Build Unity test framework

### Common Issues

#### "No module named 'esp_idf_monitor'"
This error means the ESP-IDF environment is not properly activated. Follow the activation steps above.

#### "idf.py not found"
The ESP-IDF tools are not in your PATH. Make sure to run the export script first.

#### Build Errors
If you encounter build errors, try:
1. Clean the build: `idf.py fullclean`
2. Reconfigure: `idf.py reconfigure`
3. Build again: `idf.py build`

### Next Steps

Once the verification passes, you can:
1. Build the project: `idf.py build`
2. Flash to device: `idf.py -p COM_PORT flash`
3. Monitor output: `idf.py -p COM_PORT monitor`
4. Run tests: `idf.py -T test build flash monitor`

Replace `COM_PORT` with your actual ESP32 device port (e.g., COM3, COM4, etc.).