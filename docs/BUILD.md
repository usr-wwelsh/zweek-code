# Building Zweek Code

## Prerequisites

### 1. Install CMake

**Windows:**
- Download from: https://cmake.org/download/
- Or use winget: `winget install Kitware.CMake`
- Or use Chocolatey: `choco install cmake`

**Linux:**
```bash
sudo apt install cmake  # Ubuntu/Debian
sudo dnf install cmake  # Fedora
```

**macOS:**
```bash
brew install cmake
```

### 2. Install a C++ Compiler

**Windows:**
- Install Visual Studio 2019 or later with "Desktop development with C++" workload
- Or install MinGW-w64 for GCC

**Linux:**
```bash
sudo apt install build-essential  # Ubuntu/Debian
```

**macOS:**
```bash
xcode-select --install
```

## Build Steps

### Option 1: Visual Studio (Windows)

```powershell
# Generate Visual Studio solution
cmake -S . -B build -G "Visual Studio 17 2022"

# Build
cmake --build build --config Release

# Run
.\build\Release\zweek.exe
```

### Option 2: Ninja (Cross-platform, faster)

```bash
# Install Ninja first
# Windows: winget install Ninja-build.Ninja
# Linux: sudo apt install ninja-build
# macOS: brew install ninja

# Generate Ninja build files
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build build

# Run
./build/zweek  # Linux/macOS
.\build\zweek.exe  # Windows
```

### Option 3: Unix Makefiles (Linux/macOS)

```bash
# Generate Makefiles
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build build -j$(nproc)

# Run
./build/zweek
```

## Troubleshooting

### "cmake: command not found"

Make sure CMake is installed and added to your PATH. Restart your terminal after installation.

### "Could not find FTXUI"

CMake will automatically download FTXUI during the build process. Make sure you have an internet connection during the first build.

### Build is slow

- Use Ninja generator for faster builds: `-G Ninja`
- Use parallel builds: `cmake --build build -j8` (or number of CPU cores)
- On Linux/Mac: Use `-DCMAKE_BUILD_TYPE=Release` for optimized builds

## Development Build

For faster iteration during development:

```bash
# Debug build with symbols
cmake -S . -B build-debug -DCMAKE_BUILD_TYPE=Debug

# Build
cmake --build build-debug

# Run with debugger
gdb ./build-debug/zweek  # Linux
lldb ./build-debug/zweek  # macOS
```

## Current Status

**Phase 1: TUI Foundation** ✅ Complete
- ASCII art Zweek Code branding
- Complete UI layout with FTXUI
- Progress tracking for all 5 pipeline stages
- Code preview panel
- Quality metrics display
- Interactive action buttons
- Demo pipeline simulation

**Next Phase: Model Integration** 🚧 In Progress
- llama.cpp integration for GGUF models
- ONNX Runtime integration
- Model download scripts
- Sequential model loading

## Testing the TUI (Current Demo)

The current build includes a demo pipeline that simulates the 5-stage workflow:

1. Launch the application
2. Enter a request (e.g., "Add error handling to login")
3. Press Submit
4. Watch the progress indicator move through stages:
   - 🔍 Planning
   - 🔧 Tool Execution
   - 💻 Code Drafting
   - 🎨 Style Enforcing
   - 📊 Complexity Auditing
   - 🔐 Gatekeeper Review
5. Review the sample generated code
6. Try the action buttons (Accept/Modify/Reject/View Diff)

**Note:** The demo uses simulated delays and sample code. Real model integration coming soon!
