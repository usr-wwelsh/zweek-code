# Zweek Code

**Local-Only AI Coding Assistant**  
Fully offline • Privacy-first • Multi-model architecture

---

> **🚀 New to C++?** Check out [QUICKSTART.md](QUICKSTART.md) for step-by-step installation and build instructions!

---

## Overview

Zweek Code is a terminal-based AI coding assistant that runs entirely offline on consumer hardware. Instead of relying on a single large language model, it orchestrates five specialized models (each under 150M parameters) working in sequence to provide coding assistance similar to cloud-based solutions.

## Architecture

The system uses a **5-stage pipeline**:

1. **Planner** – Breaks down user requests into discrete, tool-based steps
2. **Code Drafter** – Generates raw code (C++/Python/JS) from the plan
3. **Style Enforcer** – Refines syntax, naming, and formatting per conventions
4. **Complexity Auditor** – Flags functions over 200 lines, high cyclomatic complexity, or inefficient patterns
5. **Final Gatekeeper** – Applies deterministic rules and approves output

All models are quantized for CPU inference (INT8/FP16), loadable one at a time to minimize memory.

## System Requirements

**Minimum:**
- CPU: 4-core x86-64 (2015-era or newer, e.g., Intel Skylake, AMD Zen)
- RAM: 4GB
- Storage: 1GB for models and application
- OS: Windows 10/11, Linux, macOS

**Recommended:**
- RAM: 8GB+
- SSD for faster model loading

## Building from Source

### Prerequisites

- CMake 3.15+
- C++17 compatible compiler (MSVC 2019+, GCC 9+, Clang 10+)
- Internet connection (for fetching dependencies during build)

### Build Instructions

```bash
# Clone the repository
git clone https://github.com/wedsmoker/zweek-code.git
cd zweek-code

# Create build directory
mkdir build && cd build

# Configure and build
cmake ..
cmake --build . --config Release

# Run
./zweek  # Linux/macOS
zweek.exe  # Windows
```

## Models

Zweek Code uses the following specialized models:

| Role | Model | Parameters | Memory (INT8) |
|------|-------|------------|---------------|
| Planner | SmolLM-135M | 135M | ~150MB |
| Code Drafter | tiny_starcoder_py | 164M | ~200MB |
| Style Enforcer | CodeT5-small | 60M | ~80MB |
| Complexity Auditor | CodeBERT-small | 60M | ~70MB |
| Gatekeeper | SmolLM-135M | 135M | ~150MB |

Models are loaded sequentially, so **peak RAM usage is only ~700MB**.

### Downloading Models

Models should be placed in the `models/` directory:

```
models/
├── planner/
│   └── smollm-135m-q8_0.gguf
├── drafter/
│   └── tiny_starcoder_py-q8_0.gguf
├── enforcer/
│   └── codet5-small-int8.onnx
├── auditor/
│   └── codebert-small-int8.onnx
└── gatekeeper/
    └── smollm-135m-q8_0.gguf
```

Download links and instructions are available in the [models documentation](docs/models.md).

## Usage

1. **Launch Zweek Code:**
   ```bash
   zweek
   ```

2. **Enter your request** in the input field (e.g., "Add error handling to the login function")

3. **Watch the pipeline** as it progresses through each stage:
   - 🔍 Planning
   - 🔧 Tool Execution
   - 💻 Code Drafting
   - 🎨 Style Enforcement
   - 📊 Complexity Auditing
   - 🔐 Gatekeeper Review

4. **Review the generated code** and quality metrics

5. **Accept, Modify, or Reject** the suggested changes

## Features

- ✅ **Fully Offline** – No internet required, all processing local
- ✅ **Privacy First** – Your code never leaves your machine
- ✅ **Lightweight** – Runs on decade-old consumer hardware
- ✅ **Multi-Language** – C++, Python, JavaScript support
- ✅ **Quality Checks** – Automated complexity analysis and style enforcement
- ✅ **Tool Integration** – File operations, shell execution, code search
- ✅ **Beautiful TUI** – Modern terminal interface with FTXUI

## Project Status

**Current Version:** v1.0.0-alpha

This is an early-stage project. The TUI foundation is complete, and we're currently implementing:

- [ ] Model loader integration (llama.cpp, ONNX Runtime)
- [ ] Pipeline orchestration
- [ ] Tool executor with sandboxing
- [ ] Model fine-tuning for each role

See the [implementation plan](docs/implementation-plan.md) for details.

## Contributing

Contributions are welcome! Please see [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines.

## License

MIT License - see [LICENSE](LICENSE) for details.

## Acknowledgments

- [FTXUI](https://github.com/ArthurSonzogni/FTXUI) - Terminal UI framework
- [llama.cpp](https://github.com/ggerganov/llama.cpp) - LLM inference
- [ONNX Runtime](https://onnxruntime.ai/) - Model inference
- All open-source model creators

---

**Built with ❤️ for local-first AI development**
