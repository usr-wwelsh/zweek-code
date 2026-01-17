#include "pipeline/orchestrator.hpp"
#include "commands/command_handler.hpp"

namespace zweek {
namespace pipeline {

Orchestrator::Orchestrator() : command_handler_() {
  // Initialize history manager
  history_manager_.Init("");
  
  // Wire history manager to chat mode
  chat_mode_.SetHistoryManager(&history_manager_);
  
  // Wire history manager to command handler
  command_handler_.SetHistoryManager(&history_manager_);
  command_handler_.SetChatMode(&chat_mode_);
  
  // Wire tool executor to command handler
  command_handler_.SetToolExecutor(&tool_executor_);
  
  // Wire directory change callback
  command_handler_.SetDirectoryChangeCallback([this](const std::string& path) {
    if (directory_update_callback_) {
      directory_update_callback_(path);
    }
  });
}

Orchestrator::~Orchestrator() {
  // Destructor
}

void Orchestrator::SetWorkingDirectory(const std::string &path) {
  tool_executor_.SetWorkingDirectory(path);
  if (directory_update_callback_) {
      directory_update_callback_(path);
  }
}

void Orchestrator::ProcessRequest(const std::string &user_request) {
  // Check if it's a command first
  auto cmd_result = command_handler_.HandleCommand(user_request);
  if (cmd_result.handled) {
    if (response_callback_) {
      response_callback_(cmd_result.response);
    }
    return;
  }

  if (progress_callback_) {
    progress_callback_("Classifying intent...");
  }

  // Step 1: Classify intent
  Intent intent = router_.ClassifyIntent(user_request);
  WorkflowType workflow = router_.GetWorkflow(intent);

  // Step 2: Execute appropriate workflow
  switch (workflow) {
  case WorkflowType::CodePipeline:
    if (progress_callback_) {
      progress_callback_("Starting code generation pipeline...");
    }
    RunCodePipeline(user_request);
    break;

  case WorkflowType::ChatMode:
    if (progress_callback_) {
      progress_callback_("Entering chat mode...");
    }
    RunChatMode(user_request);
    break;

  case WorkflowType::ToolMode:
    if (progress_callback_) {
      progress_callback_("Running tools...");
    }
    RunToolMode(user_request);
    break;
  }
}

void Orchestrator::SetProgressCallback(
    std::function<void(const std::string &)> callback) {
  progress_callback_ = callback;
}

void Orchestrator::SetResponseCallback(
    std::function<void(const std::string &)> callback) {
  response_callback_ = callback;
}

void Orchestrator::SetStreamCallback(
    std::function<void(const std::string &)> callback) {
  stream_callback_ = callback;
}

void Orchestrator::SetDirectoryUpdateCallback(
    std::function<void(const std::string &)> callback) {
  directory_update_callback_ = callback;
}

void Orchestrator::SetAgentThoughtCallback(
    std::function<void(const std::string &)> callback) {
  agent_thought_callback_ = callback;
}

void Orchestrator::SetAgentCommandCallback(
    std::function<void(const std::string &)> callback) {
  agent_command_callback_ = callback;
}

void Orchestrator::SetAgentResultCallback(
    std::function<void(const std::string &, bool)> callback) {
  agent_result_callback_ = callback;
}

void Orchestrator::RunCodePipeline(const std::string &request) {
  // Lazy-initialize the recursive agent
  if (!agent_) {
    agent_config_.model_path = "models/Qwen3-0.6B-Q8_0.gguf";
    agent_config_.max_steps = 25;
    agent_config_.max_tokens_per_step = 512;
    agent_config_.context_window = 2048;
    agent_config_.history_window = 8;

    agent_ = std::make_unique<coder::RecursiveAgent>(agent_config_);

    if (progress_callback_) {
      progress_callback_("Initializing code agent...");
    }

    if (!agent_->Init()) {
      if (response_callback_) {
        response_callback_("ERROR: Failed to initialize code agent. Check model path.");
      }
      agent_.reset();
      return;
    }
  }

  // Wire up agent callbacks
  coder::AgentCallbacks callbacks;

  callbacks.on_progress = [this](const std::string& msg) {
    if (progress_callback_) {
      progress_callback_(msg);
    }
  };

  callbacks.on_thought = [this](const std::string& thought) {
    if (agent_thought_callback_) {
      agent_thought_callback_(thought);
    }
    if (progress_callback_) {
      progress_callback_("Thinking: " + thought.substr(0, 60) + (thought.length() > 60 ? "..." : ""));
    }
  };

  callbacks.on_command = [this](const std::string& cmd) {
    if (agent_command_callback_) {
      agent_command_callback_(cmd);
    }
    // Extract just the command type for progress display
    std::string cmd_preview = cmd.substr(0, cmd.find('\n'));
    if (cmd_preview.length() > 50) {
      cmd_preview = cmd_preview.substr(0, 50) + "...";
    }
    if (progress_callback_) {
      progress_callback_("Executing: " + cmd_preview);
    }
  };

  callbacks.on_tool_result = [this](const coder::ToolResult& result) {
    if (agent_result_callback_) {
      std::string output = result.success ? result.output : "ERROR: " + result.error;
      agent_result_callback_(output, result.success);
    }
  };

  callbacks.on_stream = [this](const std::string& token) {
    if (stream_callback_) {
      stream_callback_(token);
    }
  };

  callbacks.on_finish = [this](const std::string& summary) {
    if (progress_callback_) {
      progress_callback_("Task complete");
    }
  };

  callbacks.on_error = [this](const std::string& error) {
    if (progress_callback_) {
      progress_callback_("Agent error: " + error);
    }
  };

  agent_->SetCallbacks(callbacks);

  // Start and run the task
  agent_->StartTask(request, tool_executor_.GetWorkingDirectory());

  std::string result = agent_->Run(interrupt_flag_);

  // Store in history
  history_manager_.LogChatMessage("user", request);
  history_manager_.LogChatMessage("assistant", result);

  // Send final response
  if (response_callback_) {
    response_callback_(result);
  }

  // Reset agent for next task (keeps model loaded for faster subsequent requests)
  agent_->Reset();
}

void Orchestrator::RunChatMode(const std::string &request) {
  // Use ChatMode to respond
  std::vector<std::string> context; // TODO: Get relevant files
  
  std::string response = chat_mode_.Chat(request, context, [&](const std::string& chunk) {
    if (stream_callback_) {
      stream_callback_(chunk);
    }
  }, interrupt_flag_);

  // Mark as complete after streaming finishes
  if (response_callback_) {
    response_callback_(response);
  }
}

void Orchestrator::RunToolMode(const std::string &request) {
  // TODO: Implement deterministic tools (grep, git, etc.)
  if (response_callback_) {
    response_callback_("Tool mode coming in Phase 2!");
  }
}

} // namespace pipeline
} // namespace zweek
