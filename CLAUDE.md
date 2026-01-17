# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Zweek Code is a **terminal-based AI coding assistant** that runs entirely offline on local machines using specialized small models (135M-1.1B parameters). The project philosophy is: "Use deterministic tools when possible, AI when necessary."

**Current Status:** Phase 1 complete (TUI + Chat + Router + Commands + History). Phase 2 in progress (Code Pipeline integration).

## Build Commands

### Initial Setup
```bash
# Configure build (first time or after CMakeLists.txt changes)
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release

# Build executable
cmake --build build

# Run application
.\build\zweek.exe    # Windows
./build/zweek        # Linux/macOS
```

### Development Workflow
```bash
# Rebuild after code changes
cmake --build build

# Debug build with symbols
cmake -S . -B build-debug -DCMAKE_BUILD_TYPE=Debug
cmake --build build-debug

# Run tests
cmake --build build
.\build\zweek_tests  # Windows
./build/zweek_tests  # Linux/macOS
```

### Clean Rebuild
```bash
# Remove build directory and rebuild from scratch
Remove-Item -Recurse -Force build  # Windows PowerShell
rm -rf build                        # Linux/macOS
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

## High-Level Architecture

### System Overview

Zweek Code uses a **3-model optimized architecture**:

1. **Router** (SmolLM-135M) - Resident in memory, classifies user intent as CODE/CHAT/TOOL using GBNF grammar
2. **Code Drafter** (StarCoder-Tiny) - On-demand code generation (Phase 2)
3. **Chat** (Qwen3-0.6B-Q8_0) - On-demand Q&A about code

### Component Relationships

```
main.cpp (Entry Point)
  ├── TUI (FTXUI) - Terminal interface, user input, conversation display
  ├── Orchestrator - Central hub coordinating all components
  │   ├── Router - Intent classification (SmolLM-135M, resident)
  │   ├── ChatMode - Q&A handler (Qwen3, on-demand)
  │   ├── CodePipeline - Code generation (Phase 2)
  │   ├── ToolMode - File operations (Phase 2)
  │   ├── CommandHandler - Slash commands (/help, /history, etc.)
  │   └── HistoryManager - Session persistence
  └── Threading Model
      ├── Main Thread - TUI event loop
      ├── Pipeline Thread - Model inference (per request)
      └── Spinner Thread - UI animation
```

### Key Data Flow

**User Request Processing:**
1. TUI captures input → OnSubmit callback
2. Orchestrator checks if command (`/help`, `/cd`, etc.)
3. If not command: Router classifies intent using SmolLM-135M + GBNF grammar
4. Route to workflow: CODE → CodePipeline | CHAT → ChatMode | TOOL → ToolMode
5. Stream responses back to TUI via callbacks
6. HistoryManager persists conversation to disk

**ChatMode Specifics:**
- Loads Qwen3 on-demand (unloads after response)
- Uses ChatML format with thinking sections
- Detects `</think>` marker to split thinking from answer
- Streams tokens to TUI for real-time display
- Context window: 2048 tokens (includes last 10 messages)

## Source Structure

```
include/             # Header files defining component interfaces
├── chat/           # ChatMode interface
├── coder/          # RLM Harness (RecursiveAgent, AgentToolSet)
├── commands/       # Command system (/help, /history, etc.)
├── history/        # Session management and persistence
├── models/         # Model loading wrapper (llama.cpp)
├── pipeline/       # Orchestrator, Router, GBNF grammars
├── tools/          # File operations, compiler validation
└── ui/             # TUI interface, branding

src/                # Implementation files (mirrors include/)
├── main.cpp        # Entry point, threading, callbacks
├── chat/chat_mode.cpp
├── coder/agent_toolset.cpp   # Constrained tool executor
├── coder/recursive_agent.cpp # Agent loop implementation
├── commands/command_handler.cpp
├── history/history_manager.cpp
├── models/model_loader.cpp
├── pipeline/orchestrator.cpp
├── pipeline/router.cpp
├── tools/tool_executor.cpp
└── ui/tui.cpp

tests/              # Unit tests
├── test_tool_executor.cpp
└── test_agent_toolset.cpp    # AgentToolSet tests

models/             # User-provided GGUF model files (not in repo)
├── smollm-135m-router.gguf
├── Qwen3-0.6B-Q8_0.gguf
└── starcoder-tiny.gguf
```

## Key Implementation Details

### GBNF Grammars (include/pipeline/grammars.hpp)

The router uses GBNF (GGML BNF) grammar to **constrain LLM output** and prevent hallucination:

```cpp
// Router grammar ensures output is exactly CODE, CHAT, or TOOL
root ::= ws intent ws
intent ::= "CODE" | "CHAT" | "TOOL"
```

When adding new grammars, follow this pattern to guarantee valid outputs.

### Threading and Callbacks

Main components communicate via **callbacks** set up in `main.cpp`:

```cpp
orchestrator.SetProgressCallback([&tui](stage, progress) { ... });
orchestrator.SetResponseCallback([&tui](response) { ... });
orchestrator.SetStreamCallback([&tui](token) { ... });
```

**Critical:** Inference runs in background threads to avoid blocking the TUI. Use `std::atomic<bool>` for interruption signals.

### Model Loading Strategy

- **Resident models** (Router): Loaded at startup, never unloaded (~150MB)
- **On-demand models** (Chat, Code): Load when needed, unload after use
- Uses `llama.cpp` via `ModelLoader` wrapper (include/models/model_loader.hpp)

### Session Management

`HistoryManager` maintains:
- **Chat messages** (role + content + timestamp)
- **Operations** (code edits, queries, etc.)
- **File snapshots** (full file content at operation time)

Stored as JSON in `~/.zweek/history/` directory.

### Working Directory

All file operations are relative to the current working directory, managed by `ToolExecutor`. Users can change it via `/cd <path>` command.

## Common Development Tasks

### Adding a New Slash Command

1. Add command to `CommandHandler::HandleCommand()` in src/commands/command_handler.cpp
2. Implement the logic (may call `ToolExecutor` or `HistoryManager`)
3. Update help text in the same file

### Adding a New GBNF Grammar

1. Define grammar string in `include/pipeline/grammars.hpp`
2. Use it in model inference: `model_loader_.Infer(prompt, YOUR_GRAMMAR, ...)`

### Extending TUI State

1. Add new field to `TUIState` struct in include/ui/tui.hpp
2. Update rendering logic in src/ui/tui.cpp
3. Add getter/setter methods if needed for thread safety

### Adding Tests

1. Create test file in `tests/` directory
2. Add to CMakeLists.txt:
   ```cmake
   add_executable(your_test tests/your_test.cpp src/component.cpp)
   target_link_libraries(your_test PRIVATE nlohmann_json::nlohmann_json)
   add_test(NAME YourTest COMMAND your_test)
   ```
3. Use simple assertions: `assert(condition)`

## Dependencies

Managed via CMake FetchContent (auto-downloaded during first build):

- **FTXUI v5.0.0** - Terminal UI framework
- **nlohmann/json v3.11.3** - JSON serialization
- **llama.cpp (master)** - GGUF model inference with GBNF support

**Build options in CMakeLists.txt:**
```cmake
set(LLAMA_BUILD_TESTS OFF)
set(LLAMA_BUILD_EXAMPLES OFF)
set(LLAMA_BUILD_SERVER OFF)
set(BUILD_SHARED_LIBS OFF)
```

## Memory and Performance

**Target Performance:**
- Idle RAM: ~350MB (Router + overhead)
- Peak inference: ~500MB (Router + one active model)
- Response time: <15 seconds for most operations
- Router classification: <100ms

**Design Constraints:**
- Models quantized to Q8_0 format for smaller size
- Context window: 2048 tokens (balance between context and speed)
- Thinking section limited to 1000 tokens to prevent "stuck thinking"

## Phase Roadmap

**Phase 1 (COMPLETE):**
- ✅ TUI with FTXUI
- ✅ Router with GBNF
- ✅ ChatMode with Qwen3
- ✅ Command system
- ✅ Persistent history

**Phase 2 (IN PROGRESS):**
- ✅ RLM Harness (RecursiveAgent) - "Blindfold Coder" architecture
- ✅ AgentToolSet with constrained file operations
- ✅ GBNF grammar for THOUGHT/CMD format
- ✅ Multi-step tool-use loop working with Qwen3-0.6B
- ⏳ Write/Insert/Delete file operations (currently read-only)
- ⏳ Improved model instruction-following
- ⏳ Diff generation and review

**Phase 3 (PLANNED):**
- Multi-file analysis
- Git integration
- Performance profiling
- Larger model support (1B+) for better reasoning

## RLM Harness Architecture (Phase 2)

The "Blindfold Coder" approach: model never sees full files, must query environment.

### Components

```
include/coder/
├── agent_toolset.hpp    # Constrained tool interface (50-line read limit)
└── recursive_agent.hpp  # Agent loop + state management

src/coder/
├── agent_toolset.cpp    # LIST, READ_LINES, GREP, FINISH implementations
└── recursive_agent.cpp  # Step loop, prompt building, output parsing
```

### Agent Loop

```
User Request → Router (CODE intent) → RecursiveAgent
    ↓
    ┌─────────────────────────────────────┐
    │  1. Build prompt (task + last result) │
    │  2. Infer with GBNF grammar          │
    │  3. Parse THOUGHT + CMD              │
    │  4. Execute tool via AgentToolSet    │
    │  5. Loop until FINISH or max_steps   │
    └─────────────────────────────────────┘
    ↓
Final summary returned to user
```

### GBNF Grammar Constraint

Forces model output to structured format:
```
THOUGHT: <reasoning>
CMD: <LIST|READ_LINES|GREP|FINISH> <args>
```

### Key Design Decisions

| Decision | Rationale |
|----------|-----------|
| 50-line read limit | Forces model to be surgical |
| Fresh context per inference | Prevents KV cache overflow |
| Few-shot example in prompt | Helps 0.6B model follow pattern |
| Read-only by default | Prevents rogue file modifications |
| Result truncation (1000 chars) | Keeps prompt within context window |

### Known Limitations (Small Model)

- Occasional hallucination in summaries
- May repeat commands unnecessarily
- Struggles with multi-step reasoning ("read file to understand it")
- Works best with simple, direct tasks
