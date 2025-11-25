#pragma once

#include "models/model_loader.hpp"
#include <string>


namespace zweek {
namespace pipeline {

// Intent classification for router
enum class Intent {
  CodeGeneration, // User wants to generate/modify code
  Chat,           // User wants to ask questions
  Tool,           // User wants to search/explore code
  Unknown
};

// Workflow types
enum class WorkflowType {
  CodePipeline, // Full 5-model pipeline
  ChatMode,     // TinyLlama-Chat for Q&A
  ToolMode      // Deterministic tools
};

// Router classifies user intent using SmolLM-135M
class Router {
public:
  Router();
  ~Router();

  // Classify user intent using real AI model
  Intent ClassifyIntent(const std::string &user_input);

  // Get workflow for intent
  WorkflowType GetWorkflow(Intent intent);

  // Load the router model (SmolLM-135M) as resident
  bool LoadModel(const std::string &model_path);

  // Unload to free memory
  void UnloadModel();

private:
  bool model_loaded_ = false;
  models::ModelLoader model_loader_;
};

} // namespace pipeline
} // namespace zweek
