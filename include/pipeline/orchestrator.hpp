#pragma once

#include "chat/chat_mode.hpp"
#include "coder/recursive_agent.hpp"
#include "commands/command_handler.hpp"
#include "history/history_manager.hpp"
#include "pipeline/router.hpp"
#include "tools/tool_executor.hpp"
#include <functional>
#include <string>
#include <atomic>
#include <memory>


namespace zweek {
namespace pipeline {

// Main orchestrator coordinates workflows
class Orchestrator {
public:
  Orchestrator();
  ~Orchestrator();

  // Main entry point - processes user request
  void ProcessRequest(const std::string &user_request);

  // Set working directory
  void SetWorkingDirectory(const std::string &path);

  // Set callbacks for UI updates
  void SetProgressCallback(std::function<void(const std::string &)> callback);
  void SetResponseCallback(std::function<void(const std::string &)> callback);
  void SetStreamCallback(std::function<void(const std::string &)> callback);
  void SetDirectoryUpdateCallback(std::function<void(const std::string &)> callback);

  // Agent-specific callbacks for RLM harness
  void SetAgentThoughtCallback(std::function<void(const std::string &)> callback);
  void SetAgentCommandCallback(std::function<void(const std::string &)> callback);
  void SetAgentResultCallback(std::function<void(const std::string &, bool)> callback);
  
  // Set interrupt flag for cancellation
  void SetInterruptFlag(std::atomic<bool>* flag) { interrupt_flag_ = flag; }
  
  // Get history manager for external use
  history::HistoryManager* GetHistoryManager() { return &history_manager_; }
  
  // Get command handler for external use
  commands::CommandHandler* GetCommandHandler() { return &command_handler_; }

private:
  // Workflow handlers
  void RunCodePipeline(const std::string &request);
  void RunChatMode(const std::string &request);
  void RunToolMode(const std::string &request);

  Router router_;
  chat::ChatMode chat_mode_;
  commands::CommandHandler command_handler_;
  history::HistoryManager history_manager_;
  tools::ToolExecutor tool_executor_;

  // Recursive agent for code tasks (lazy-initialized)
  std::unique_ptr<coder::RecursiveAgent> agent_;
  coder::AgentConfig agent_config_;

  // Callbacks
  std::function<void(const std::string &)> progress_callback_;
  std::function<void(const std::string &)> response_callback_;
  std::function<void(const std::string &)> stream_callback_;
  std::function<void(const std::string &)> directory_update_callback_;

  // Agent-specific callbacks
  std::function<void(const std::string &)> agent_thought_callback_;
  std::function<void(const std::string &)> agent_command_callback_;
  std::function<void(const std::string &, bool)> agent_result_callback_;

  // Interrupt flag
  std::atomic<bool>* interrupt_flag_ = nullptr;
};

} // namespace pipeline
} // namespace zweek
