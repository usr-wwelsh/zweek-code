# Zweek Code - Build this in the# Models Directory

This directory contains the AI models used by Zweek Code.

## Auto-Download

Zweek Code will automatically download models on first run. If a model is already present, it won't re-download.

**Models will be downloaded to:**
```
models/
├── smollm-135m-router.gguf         (Router: ~150MB)
├── tinyllama-chat.gguf             (Chat: ~1.2GB)
├── starcoder-tiny.gguf             (Code: ~200MB)
├── codet5-small.onnx               (Style: ~80MB)
└── codebert-small.onnx             (Audit: ~70MB)
```

## Manual Download

If you prefer to download manually, get the models from Hugging Face:

1. **SmolLM-135M (Router)**: https://huggingface.co/HuggingFaceTB/SmolLM-135M
2. **TinyLlama-Chat**: https://huggingface.co/TinyLlama/TinyLlama-1.1B-Chat-v1.0
3. **StarCoder-Tiny**: https://huggingface.co/bigcode/tiny_starcoder_py

Look for GGUF quantized versions (Q8_0 or Q4_K_M for smaller size).

## Total Size

- **Full suite**: ~1.7GB
- **Code-only (no chat)**: ~500MB

Models load one at a time, so peak RAM usage stays low!
models/auditor/codebert-small-int8.onnx

# Gatekeeper: SmolLM-135M quantized (can reuse planner model)
models/gatekeeper/smollm-135m-q8_0.gguf
