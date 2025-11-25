#pragma once

#include "chat/chat_mode.hpp"
#include "commands/command_handler.hpp"
#include "pipeline/router.hpp"
#include <functional>
#include <string>


namespace zweek {
namespace pipeline {

// Main orchestrator coordinates workflows
class Orchestrator {
public:
  Orchestrator();
  ~Orchestrator();

  // Main entry point - processes user request
  void ProcessRequest(const std::string &user_request);

  // Set callbacks for UI updates
  void SetProgressCallback(std::function<void(const std::string &)> callback);
  void SetResponseCallback(std::function<void(const std::string &)> callback);

private:
  // Workflow handlers
  void RunCodePipeline(const std::string &request);
  void RunChatMode(const std::string &request);
  void RunToolMode(const std::string &request);

  Router router_;
  chat::ChatMode chat_mode_;
  commands::CommandHandler command_handler_;

  // Callbacks
  std::function<void(const std::string &)> progress_callback_;
  std::function<void(const std::string &)> response_callback_;
};

} // namespace pipeline
} // namespace zweek
