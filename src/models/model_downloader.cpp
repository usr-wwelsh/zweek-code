#include "models/model_downloader.hpp"
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

namespace zweek {
namespace models {

ModelDownloader::ModelDownloader() {
  // Constructor
}

ModelDownloader::~ModelDownloader() {
  // Destructor
}

bool ModelDownloader::ModelExists(const std::string &model_path) {
  return fs::exists(model_path) && fs::is_regular_file(model_path);
}

bool ModelDownloader::EnsureModel(
    const ModelInfo &model_info,
    std::function<void(const std::string &)> progress_callback) {
  // Check if already exists
  if (ModelExists(model_info.local_path)) {
    if (progress_callback) {
      progress_callback("✓ " + model_info.name + " already downloaded");
    }
    return true;
  }

  // Create models directory if needed
  fs::path model_path(model_info.local_path);
  fs::create_directories(model_path.parent_path());

  if (progress_callback) {
    progress_callback("Downloading " + model_info.name + "...");
  }

  // Download the model
  return DownloadFile(model_info.url, model_info.local_path, progress_callback);
}

bool ModelDownloader::EnsureAllModels(
    std::function<void(const std::string &)> progress_callback) {
  auto models = GetAllModels();

  for (const auto &model : models) {
    if (!EnsureModel(model, progress_callback)) {
      return false;
    }
  }

  return true;
}

std::vector<ModelInfo> ModelDownloader::GetAllModels() {
  // Model URLs from Hugging Face (GGUF quantized versions)
  return {
      {"SmolLM-135M (Router)",
       "https://huggingface.co/TheBloke/SmolLM-135M-GGUF/resolve/main/"
       "smollm-135m.Q8_0.gguf",
       "models/smollm-135m-router.gguf", 150},
      {"TinyLlama-Chat",
       "https://huggingface.co/TheBloke/TinyLlama-1.1B-Chat-v1.0-GGUF/resolve/"
       "main/tinyllama-1.1b-chat-v1.0.Q8_0.gguf",
       "models/tinyllama-chat.gguf", 1200},
      {"StarCoder-Tiny (Code Drafter)",
       "https://huggingface.co/TheBloke/starcoder-GGUF/resolve/main/"
       "starcoder.Q8_0.gguf",
       "models/starcoder-tiny.gguf", 200} // Add more models as needed
  };
}

bool ModelDownloader::DownloadFile(
    const std::string &url, const std::string &output_path,
    std::function<void(const std::string &)> progress_callback) {
  // TODO: Implement actual HTTP download
  // For now, just print what we would do
  if (progress_callback) {
    progress_callback("Download stub: " + url + " -> " + output_path);
    progress_callback("(Full download implementation coming in Phase 2)");
  }

  std::cout << "Would download: " << url << std::endl;
  std::cout << "To: " << output_path << std::endl;

  // Return false for now since we're not actually downloading
  return false;
}

} // namespace models
} // namespace zweek
