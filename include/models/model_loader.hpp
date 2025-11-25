#pragma once

#include <string>
#include <vector>

// Forward declare llama.cpp types
struct llama_model;
struct llama_context;
struct llama_sampler;

namespace zweek {
namespace models {

// Model loader with GBNF and resident support
class ModelLoader {
public:
  ModelLoader();
  ~ModelLoader();

  // Load model and keep it resident (never unload automatically)
  bool LoadResident(const std::string &model_path, int n_ctx = 512);

  // Load model temporarily
  bool Load(const std::string &model_path, int n_ctx = 512);

  // Run inference with optional GBNF grammar
  std::string Infer(const std::string &prompt, const std::string &grammar = "",
                    int max_tokens = 256);

  // Unload model (only if not resident)
  void Unload();

  // Check if model is loaded
  bool IsLoaded() const { return model_ != nullptr; }

  // Check if model is resident
  bool IsResident() const { return is_resident_; }

private:
  llama_model *model_ = nullptr;
  llama_context *ctx_ = nullptr;
  llama_sampler *sampler_ = nullptr;
  bool is_resident_ = false;
  int n_ctx_ = 512;

  // Internal inference
  std::string RunInference(const std::string &prompt,
                           const std::string &grammar, int max_tokens);
};

} // namespace models
} // namespace zweek
