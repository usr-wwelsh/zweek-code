# Zweek Code

**AI Coding Assistant That Runs on YOUR Machine**  
No cloud. No telemetry. Just you and your code.

![Zweek Code Demo](demo.png)

---

## What Is This?

A terminal-based AI coding assistant powered by specialized local models. Unlike cloud assistants, everything runs offline on consumer hardware using optimized small models (135M-1.1B parameters).

**Philosophy:** Use deterministic tools when possible, AI when necessary.

## Features

✅ **100% Offline** – No internet, no tracking, no data leaves your machine  
✅ **Fast** – Router stays resident (~150MB RAM), responses in seconds  
✅ **Private** – Your code is yours  
✅ **Lightweight** – Runs on older hardware (4GB RAM minimum)  
✅ **Smart Routing** – SmolLM classifies intent (CODE/CHAT/TOOL) instantly

## Architecture

**Optimized 3-Model System:**

1. **Router** (SmolLM-135M) - Stays loaded, classifies intent with GBNF grammar
2. **Code Drafter** (StarCoder-Tiny) - Generates code when needed
3. **Chat** (TinyLlama-1.1B) - Answers questions about your code

**Key Optimizations:**
- Resident models stay in memory (~350MB idle)
- GBNF grammars eliminate hallucination
- Compiler check (`cl.exe`) validates code instantly (no AI)
- Peak RAM: ~500MB during inference

## Quick Start

### Requirements

- Windows 10/11, Linux, or macOS
- 4GB RAM minimum (8GB recommended)
- CMake 3.20+
- C++17 compiler

### Build

```bash
git clone https://github.com/wedsmoker/zweek-code.git
cd zweek-code

# Download models (place in models/ directory)
# - smollm-135m-router.gguf
# - tinyllama-chat.gguf  
# - starcoder-tiny.gguf

cmake -S . -B build -G Ninja
cmake --build build
.\build\zweek.exe  # Windows
./build/zweek      # Linux/macOS
```

See [QUICKSTART.md](QUICKSTART.md) for detailed instructions.

## Usage

```
> /help
Show commands and tips

> what is this function doing?
Chat mode answers questions

> add error handling here
Code mode generates fixes
```

**Keyboard:**
- `m` - Switch between Plan and Auto mode
- `y`/`n` - Accept/reject changes  
- `Ctrl+C` - Exit

## Models

Place these in `models/`:

| Model | Size | Purpose | RAM |
|-------|------|---------|-----|
| smollm-135m-router.gguf | ~150MB | Intent classification | Resident |
| starcoder-tiny.gguf | ~200MB | Code generation | On-demand |
| tinyllama-chat.gguf | ~1.2GB | Q&A | On-demand |

Download from HuggingFace (GGUF Q8 quantized versions).

## Performance

**Target:** <15 seconds for most operations  
**Idle RAM:** ~350MB (Router + Code Drafter resident)  
**Peak RAM:** ~500MB during chat inference

## Status

**v1.0.0-alpha** - Phase 2 Complete

✅ TUI with FTXUI  
✅ Router with GBNF  
✅ Chat mode with TinyLlama  
✅ Compiler-based validation  
✅ Command system  

## Tech Stack

- [llama.cpp](https://github.com/ggerganov/llama.cpp) - GGUF model inference
- [FTXUI](https://github.com/ArthurSonzogni/FTXUI) - Terminal UI
- [nlohmann/json](https://github.com/nlohmann/json) - JSON handling

## License

MIT - See [LICENSE](LICENSE)

---

**No cloud. No compromises. Your code, your machine.**
