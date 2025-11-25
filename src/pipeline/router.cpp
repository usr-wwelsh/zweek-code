#include "pipeline/router.hpp"
#include "pipeline/grammars.hpp"
#include <algorithm>

namespace zweek {
namespace pipeline {

Router::Router() {
  // Constructor
}

Router::~Router() { UnloadModel(); }

Intent Router::ClassifyIntent(const std::string &user_input) {
  // Load model if not loaded (resident)
  if (!model_loaded_) {
    LoadModel("models/smollm-135m-router.gguf");
  }

  // Use GBNF grammar for guaranteed valid output
  std::string prompt = "Classify this request as CODE, CHAT, or TOOL:\n" +
                       user_input + "\nClassification:";

  std::string result =
      model_loader_.Infer(prompt, grammars::ROUTER_GRAMMAR, 10);

  // Parse result
  std::string lower = result;
  std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

  if (lower.find("code") != std::string::npos) {
    return Intent::CodeGeneration;
  }
  if (lower.find("chat") != std::string::npos) {
    return Intent::Chat;
  }
  if (lower.find("tool") != std::string::npos) {
    return Intent::Tool;
  }

  // Default to chat if parsing fails
  return Intent::Chat;
}

WorkflowType Router::GetWorkflow(Intent intent) {
  switch (intent) {
  case Intent::CodeGeneration:
    return WorkflowType::CodePipeline;
  case Intent::Chat:
    return WorkflowType::ChatMode;
  case Intent::Tool:
    return WorkflowType::ToolMode;
  default:
    return WorkflowType::ChatMode;
  }
}

bool Router::LoadModel(const std::string &model_path) {
  // Load as resident - never unloads
  model_loaded_ = model_loader_.LoadResident(model_path, 256);
  return model_loaded_;
}

void Router::UnloadModel() {
  // Resident models don't unload, but call it anyway
  model_loader_.Unload();
  model_loaded_ = false;
}

} // namespace pipeline
} // namespace zweek
