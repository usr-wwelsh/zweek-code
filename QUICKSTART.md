# Quick Start Guide for Beginners (Windows)

**Never used C++ before? No problem!** This guide walks you through everything step-by-step.

---

## What You Need to Install

### 1. C++ Compiler

You need a C++ compiler to build the project. Choose **ONE** option:

#### Option A: MSVC Build Tools (Recommended - Lightweight)

If you already use VS Code, you don't need the full Visual Studio IDE. Just get the compiler:

**Quick Install:**
```powershell
winget install Microsoft.VisualStudio.2022.BuildTools
```

When the installer opens, select **"Desktop development with C++"** and click Install.

**Manual Download:**
- Go to: https://visualstudio.microsoft.com/downloads/
- Scroll to "Tools for Visual Studio" → Download **"Build Tools for Visual Studio 2022"**
- Run it and select "Desktop development with C++"

**Size:** ~2-3GB (much smaller than full Visual Studio)

#### Option B: Full Visual Studio 2022 (If you want the IDE)

Only choose this if you want the full development environment:
1. Download **"Visual Studio 2022 Community"** (free)
2. Select **"Desktop development with C++"**
3. Install (takes 15-30 minutes, ~10GB)

**Why?** The C++ compiler (MSVC) turns our code into a runnable program.

### 2. CMake (Build System)

CMake is a tool that helps organize and build C++ projects.

**Quick Install (Recommended):**
```powershell
winget install Kitware.CMake
winget install Ninja-build.Ninja
```

**Manual Install:**
1. CMake: https://cmake.org/download/ - Get the Windows x64 Installer (.msi)
   - ⚠️ Check the box "Add CMake to system PATH"
2. Ninja: https://github.com/ninja-build/ninja/releases
   - Download `ninja-win.zip`
   - Extract `ninja.exe` to `C:\Windows\System32\` (or add to PATH)

**Why?** 
- CMake reads our `CMakeLists.txt` file and sets up the build
- Ninja makes builds faster than Visual Studio's default builder

### 3. Git (Optional but Recommended)

**Already installed?** Try: `git --version` in PowerShell

If not installed:
```powershell
winget install Git.Git
```

---

## Building Zweek Code

### Step 1: Open PowerShell

1. Press `Windows + X`
2. Select **"Windows PowerShell"** or **"Terminal"**

### Step 2: Navigate to the Project

```powershell
cd C:\Users\User1\Documents\agent-coder\zweek-code
```

### Step 3: Configure the Build

This step tells CMake to generate build files:

```powershell
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
```

**What this does:**
- `-S .` = Source is in current directory
- `-B build` = Put build files in a folder called "build"
- `-G Ninja` = Use Ninja for fast builds
- `-DCMAKE_BUILD_TYPE=Release` = Optimized build (faster program)

**First time?** CMake will download FTXUI and JSON libraries automatically. This takes 2-3 minutes.

You should see output like:
```
-- The CXX compiler identification is MSVC 19.xx.xxxxx.x
-- Detecting CXX compiler ABI info
-- Configuring done
-- Generating done
-- Build files written to: C:/Users/User1/Documents/agent-coder/zweek-code/build
```

**Alternative:** If Ninja doesn't work, use Visual Studio generator:
```powershell
cmake -S . -B build -G "Visual Studio 17 2022"
```

### Step 4: Build the Project

```powershell
cmake --build build
```

**What this does:**
- Compiles all the C++ code
- Links it together into an executable
- Puts the result in `build\zweek.exe` (Ninja) or `build\Release\zweek.exe` (Visual Studio)

This takes 1-2 minutes. You'll see lots of output as files compile:
```
[1/8] Building CXX object CMakeFiles/zweek.dir/src/main.cpp.obj
[2/8] Building CXX object CMakeFiles/zweek.dir/src/ui/tui.cpp.obj
...
[8/8] Linking CXX executable zweek.exe
```

### Step 5: Run Zweek Code!

**If using Ninja:**
```powershell
.\build\zweek.exe
```

**If using Visual Studio generator:**
```powershell
.\build\Release\zweek.exe
```

🎉 You should see the Zweek Code TUI launch!

---

## Quick Reference: All Commands in One Place

After initial setup, you only need these commands:

```powershell
# Navigate to project
cd C:\Users\User1\Documents\agent-coder\zweek-code

# If you made code changes, rebuild:
cmake --build build

# Run the program:
.\build\zweek.exe
```

---

## Troubleshooting

### ❌ "cmake: command not found"

**Problem:** CMake isn't installed or not in PATH

**Solution:**
1. Restart PowerShell (CMake might just have been installed)
2. If still broken, reinstall CMake and check "Add to PATH"
3. Or run: `winget install Kitware.CMake` then restart PowerShell

### ❌ "No CMAKE_CXX_COMPILER could be found"

**Problem:** Visual Studio C++ tools aren't installed

**Solution:**
1. Open Visual Studio Installer
2. Click "Modify" on Visual Studio 2022
3. Make sure "Desktop development with C++" is checked
4. Click Modify to install

### ❌ Build fails with errors

**Problem:** Code might have issues or conflicting dependencies

**Solution:**
1. Delete the `build` folder: `Remove-Item -Recurse -Force build`
2. Try configuring again: `cmake -S . -B build -G "Visual Studio 17 2022"`
3. Build: `cmake --build build --config Release`

### ❌ Program crashes or doesn't start

**Problem:** Missing DLL files or runtime issues

**Solution:**
1. Make sure you built with `--config Release`
2. Try running from the correct directory
3. Check you have Visual C++ Redistributables installed (usually comes with Visual Studio)

---

## Understanding the Files

**What is all this code?**

- `src/main.cpp` - Entry point, starts the program
- `src/ui/tui.cpp` - All the visual interface code
- `include/ui/tui.hpp` - Defines what the TUI can do
- `CMakeLists.txt` - Build instructions for CMake

**Can I edit the code?**

Yes! Try this:
1. Open `src/ui/branding.hpp` in any text editor
2. Change the `VERSION` string to something like `"v1.0.0-myedition"`
3. Rebuild: `cmake --build build --config Release`
4. Run: `.\build\Release\zweek.exe`
5. You'll see your custom version number!

---

## What the Demo Does

The current build is a **demo** that shows what Zweek Code will do:

1. Type a request (e.g., "Add error handling to login")
2. Press Submit (or Enter)
3. Watch the progress bar move through 5 stages:
   - 🔍 **Planning** - Figuring out what to do
   - 🔧 **Tool Execution** - Reading files, gathering info
   - 💻 **Code Drafting** - Generating code
   - 🎨 **Style Enforcing** - Making it look nice
   - 📊 **Complexity Auditing** - Checking quality
   - 🔐 **Gatekeeper Review** - Final approval

4. See sample code appear
5. Try the buttons (they print to console for now)

**Note:** This is a simulation! The real AI models will be added in Phase 2.

---

## Next Steps

Once you're comfortable with building and running:

1. **Explore the code** - Open files in Visual Studio or VS Code
2. **Try making small changes** - Edit version strings, colors, emojis
3. **Read the full docs** - Check out `docs/BUILD.md` for advanced options
4. **Wait for Phase 2** - Real AI model integration coming soon!

---

## Alternative: Visual Studio GUI Method

If you prefer clicking buttons instead of command line:

1. Open Visual Studio 2022
2. Click **"Open a local folder"**
3. Select `C:\Users\User1\Documents\agent-coder\zweek-code`
4. Visual Studio will detect CMake and configure automatically
5. Press `Ctrl + Shift + B` to build
6. Press `F5` to run

The executable will still be in `build\Release\zweek.exe`.

---

## Getting Help

- **Build errors?** Check the error message and troubleshooting section above
- **Want to understand C++?** This project uses modern C++17, check out LearnCpp.com
- **FTXUI questions?** See https://github.com/ArthurSonzogni/FTXUI

**You don't need to be a C++ expert to use Zweek Code!** The build system handles most of the complexity.
