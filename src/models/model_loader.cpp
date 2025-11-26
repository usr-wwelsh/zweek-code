#include "models/model_loader.hpp"
#include <cstring>
#include <iostream>
#include <llama.h>

namespace zweek {
namespace models {

ModelLoader::ModelLoader() {
// Suppress llama.cpp logs completely (redirect stderr)
#ifdef _WIN32
  freopen("NUL", "w", stderr); // Windows
#else
  freopen("/dev/null", "w", stderr); // Unix
#endif

  llama_backend_init();
  llama_log_set(nullptr, nullptr);
}

ModelLoader::~ModelLoader() {
  Unload();
  llama_backend_free();
}

bool ModelLoader::LoadResident(const std::string &model_path, int n_ctx) {
  is_resident_ = true;
  return Load(model_path, n_ctx);
}

bool ModelLoader::Load(const std::string &model_path, int n_ctx) {
  // Unload existing model first
  if (model_ != nullptr) {
    Unload();
  }

  n_ctx_ = n_ctx;

  // Load model
  llama_model_params model_params = llama_model_default_params();
  model_ = llama_model_load_from_file(model_path.c_str(), model_params);

  if (!model_) {
    std::cerr << "Failed to load model: " << model_path << std::endl;
    return false;
  }

  // Create context
  llama_context_params ctx_params = llama_context_default_params();
  ctx_params.n_ctx = n_ctx;
  ctx_params.n_batch = 512;
  ctx_params.n_threads = 4; // Use 4 threads for old hardware

  ctx_ = llama_new_context_with_model(model_, ctx_params);

  if (!ctx_) {
    std::cerr << "Failed to create context" << std::endl;
    llama_free_model(model_);
    model_ = nullptr;
    return false;
  }

  // Create sampler
  auto sparams = llama_sampler_chain_default_params();
  sampler_ = llama_sampler_chain_init(sparams);

  llama_sampler_chain_add(sampler_, llama_sampler_init_top_k(40));
  llama_sampler_chain_add(sampler_, llama_sampler_init_top_p(0.95f, 1));
  llama_sampler_chain_add(sampler_, llama_sampler_init_temp(0.7f));
  llama_sampler_chain_add(sampler_,
                          llama_sampler_init_dist(LLAMA_DEFAULT_SEED));

  // Model loaded successfully (silent - don't spam TUI)
  return true;
}

void ModelLoader::Unload() {
  // Don't unload if resident
  if (is_resident_) {
    return;
  }

  if (sampler_) {
    llama_sampler_free(sampler_);
    sampler_ = nullptr;
  }

  if (ctx_) {
    llama_free(ctx_);
    ctx_ = nullptr;
  }
}
std::string ModelLoader::Infer(const std::string &prompt,
                               const std::string &grammar, int max_tokens,
                               std::function<void(const std::string &)> stream_callback) {
  if (!model_ || !ctx_) {
    return "[Error: Model not loaded]";
  }

  return RunInference(prompt, grammar, max_tokens, stream_callback);
}
std::string ModelLoader::RunInference(const std::string &prompt,
                                      const std::string &grammar,
                                      int max_tokens,
                                      std::function<void(const std::string &)> stream_callback) {
  // Tokenize
  std::vector<llama_token> tokens;
  tokens.resize(prompt.size() + 16);
  const llama_vocab *vocab = llama_model_get_vocab(model_);
  int n_tokens = llama_tokenize(vocab, prompt.c_str(), prompt.size(),
                                tokens.data(), tokens.size(), true, false);
  if (n_tokens < 0)
    return "[Error: Tokenization failed]";
  tokens.resize(n_tokens);

  // Evaluate
  llama_batch batch = llama_batch_get_one(tokens.data(), n_tokens);
  if (llama_decode(ctx_, batch) != 0)
    return "[Error: Decode failed]";

  // Generate tokens with streaming display
  std::string result;
  int line_length = 0;
  const int MAX_LINE_LENGTH = 80;

  for (int i = 0; i < max_tokens; ++i) {
    llama_token tok = llama_sampler_sample(sampler_, ctx_, -1);
    if (llama_token_is_eog(vocab, tok))
      break;

    char buf[256];
    int n = llama_token_to_piece(vocab, tok, buf, sizeof(buf), 0, false);
    if (n > 0) {
      std::string token_str(buf, n);

      // Word wrap: insert newline if line gets too long
      if (line_length + token_str.length() > MAX_LINE_LENGTH) {
        bool wrapped = false;
        // If token starts with space, replace it with newline
        if (!token_str.empty() && token_str[0] == ' ') {
          token_str[0] = '\n';
          line_length = 0;
          wrapped = true;
        }
        // Otherwise if it's a long word or we can't find a space, force wrap
        else if (line_length > 0) {
          if (stream_callback) stream_callback("\n");
          result += "\n";
          line_length = 0;
          wrapped = true;
        }
      }

      result += token_str;
      line_length += token_str.length();

      // Reset line counter on newlines
      if (token_str.find('\n') != std::string::npos) {
        line_length = 0;
      }

      // Stream to console for immediate feedback (optional)
      // std::cout << token_str << std::flush;
      
      if (stream_callback) {
        stream_callback(token_str);
      }
    }

    batch = llama_batch_get_one(&tok, 1);
    if (llama_decode(ctx_, batch) != 0)
      break;
  }
  return result;
}
} // namespace models
} // namespace zweek
