#pragma once

#include <functional>
#include <string>
#include <vector>


namespace zweek {
namespace models {

// Model information
struct ModelInfo {
  std::string name;
  std::string url;
  std::string local_path;
  size_t expected_size_mb;
};

// Model downloader with auto-check
class ModelDownloader {
public:
  ModelDownloader();
  ~ModelDownloader();

  // Check if model exists locally
  bool ModelExists(const std::string &model_path);

  // Download model if not present
  bool EnsureModel(
      const ModelInfo &model_info,
      std::function<void(const std::string &)> progress_callback = nullptr);

  // Download all required models
  bool EnsureAllModels(
      std::function<void(const std::string &)> progress_callback = nullptr);

  // Get list of all models
  static std::vector<ModelInfo> GetAllModels();

private:
  bool DownloadFile(const std::string &url, const std::string &output_path,
                    std::function<void(const std::string &)> progress_callback);
};

} // namespace models
} // namespace zweek
