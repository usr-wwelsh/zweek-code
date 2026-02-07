#include "commands/command_handler.hpp"
#include "history/history_manager.hpp"
#include "chat/chat_mode.hpp"
#include "tools/tool_executor.hpp"
#include <algorithm>
#include <filesystem>

namespace zweek {
namespace commands {

CommandHandler::CommandHandler() {
  // Constructor
}

CommandResult CommandHandler::HandleCommand(const std::string &input) {
  CommandResult result;

  // Check if input starts with /
  if (input.empty() || input[0] != '/') {
    return result; // Not a command
  }

  // Parse command
  std::string cmd = input.substr(1);
  std::string args;
  size_t space_pos = cmd.find(' ');
  if (space_pos != std::string::npos) {
    args = cmd.substr(space_pos + 1);
    cmd = cmd.substr(0, space_pos);
  }

  // Handle /help
  if (cmd == "help") {
    result.handled = true;
    result.response = GetHelpText();
    return result;
  }

  // Handle /history [limit]
  if (cmd == "history") {
    result.handled = true;
    if (!history_manager_) {
      result.response = "Error: History manager not available.";
      return result;
    }
    
    int limit = 10; // Default
    if (!args.empty()) {
      try {
        limit = std::stoi(args);
      } catch (...) {
        // Ignore parsing error
      }
    }
    
    auto history = history_manager_->GetChatHistory(limit);
    if (history.empty()) {
      result.response = "No chat history available.";
    } else {
      std::string output = "Chat History (last " + std::to_string(history.size()) + " messages):\n";
      for (const auto& msg : history) {
        output += "[" + msg.role + "]: " + msg.content + "\n";
      }
      result.response = output;
    }
    return result;
  }

  // Handle /sessions
  if (cmd == "sessions") {
    result.handled = true;
    if (!history_manager_) {
      result.response = "Error: History manager not available.";
      return result;
    }
    
    // Update cache
    cached_sessions_ = history_manager_->GetAvailableSessions();
    std::string current = history_manager_->GetCurrentSessionId();
    
    std::string output = "Available Sessions:\n";
    if (cached_sessions_.empty()) {
      output += "(No saved sessions found)\n";
    } else {
      for (size_t i = 0; i < cached_sessions_.size(); ++i) {
        std::string marker = (cached_sessions_[i] == current) ? "* " : "  ";
        output += marker + "[" + std::to_string(i + 1) + "] " + cached_sessions_[i] + "\n";
      }
    }
    result.response = output;
    return result;
  }

  // Handle /load <index_or_id>
  if (cmd == "load") {
    result.handled = true;
    if (!history_manager_ || !chat_mode_) {
      result.response = "Error: History manager or Chat mode not available.";
      return result;
    }
    
    if (args.empty()) {
      result.response = "Usage: /load <index> (run /sessions first to see indices)";
      return result;
    }
    
    std::string session_id = args;
    
    // Check if input is a number (index)
    bool is_number = !args.empty() && std::all_of(args.begin(), args.end(), ::isdigit);
    
    if (is_number) {
      int index = std::stoi(args);
      
      // If cache is empty, try to populate it
      if (cached_sessions_.empty()) {
        cached_sessions_ = history_manager_->GetAvailableSessions();
      }
      
      if (index > 0 && index <= static_cast<int>(cached_sessions_.size())) {
        session_id = cached_sessions_[index - 1];
      } else {
        result.response = "Error: Invalid session index. Run /sessions to see available sessions.";
        return result;
      }
    }
    
    std::string dir = history_manager_->GetSessionsDirectory();
    std::string path = dir + "/" + session_id + ".json";
    
    #ifdef _WIN32
    path = dir + "\\" + session_id + ".json";
    #endif
    
    if (history_manager_->LoadFromFile(path)) {
      chat_mode_->LoadSessionHistory();
      
      // Construct full history output for TUI
      // Start with [CLEAR] token to clear existing TUI history
      std::string output = "[CLEAR]\nSession loaded: " + session_id + "\n";
      
      auto history = chat_mode_->GetHistory();
      for (const auto& msg : history) {
        if (msg.role == "user") {
          output += "> " + msg.content + "\n";
        } else {
          output += msg.content + "\n";
        }
      }
      result.response = output;
    } else {
      result.response = "Error: Failed to load session " + session_id;
    }
    return result;
  }

  // Handle /clear-history
  if (cmd == "clear-history") {
    result.handled = true;
    if (!history_manager_) {
      result.response = "Error: History manager not available.";
      return result;
    }
    
    history_manager_->ClearChatHistory();
    if (chat_mode_) chat_mode_->ClearHistory();
    result.response = "[CLEAR]\nChat history cleared for this session.";
    return result;
  }

  // Handle /cd <path>
  if (cmd == "cd") {
    result.handled = true;
    if (!tool_executor_) {
      result.response = "Error: Tool executor not available.";
      return result;
    }

    if (args.empty()) {
      result.response = "Usage: /cd <path>";
      return result;
    }

    std::string path = args;
    
    // Resolve path relative to ToolExecutor's working directory
    std::filesystem::path target_path(path);
    if (!target_path.is_absolute()) {
      // Resolve relative to ToolExecutor's current working directory
      std::filesystem::path current_wd(tool_executor_->GetWorkingDirectory());
      target_path = current_wd / target_path;
    }
    
    // Normalize the path
    std::error_code ec;
    target_path = std::filesystem::canonical(target_path, ec);
    if (ec) {
      // canonical failed, try without normalization
      target_path = std::filesystem::absolute(target_path);
    }
    
    // Check if it exists and is a directory
    if (std::filesystem::exists(target_path, ec) && std::filesystem::is_directory(target_path, ec)) {
      tool_executor_->SetWorkingDirectory(target_path.string());
      
      // Notify UI
      if (directory_change_callback_) {
        directory_change_callback_(target_path.string());
      }
      result.response = "Changed directory to: " + target_path.string();
    } else {
      result.response = "Error: Directory not found: " + path;
    }
    return result;
  }

  // Handle /ls [path]
  if (cmd == "ls") {
    result.handled = true;
    if (!tool_executor_) {
      result.response = "Error: Tool executor not available.";
      return result;
    }

    std::string path = args.empty() ? "." : args;
    auto files = tool_executor_->ListDir(path);
    
    if (files.empty()) {
        result.response = "No files found in " + path;
    } else {
        std::string output = "Files in " + path + ":\n";
        for (const auto& file : files) {
            output += file + "\n";
        }
        result.response = output;
    }
    return result;
  }

  return result;
}

std::vector<std::string> CommandHandler::GetAvailableCommands() const {
  return {
    "help",
    "history",
    "sessions",
    "load",
    "clear-history",
    "cd",
    "ls"
  };
}

std::string CommandHandler::GetHelpText() {
  return R"(
Zweek Code - AI that runs on YOUR machine, not the cloud.
Seven specialized models working together to keep your code private and your workflow fast.

Available Commands:
  /help - Show this message
  /history [n] - Show last n chat messages
  /sessions - List available sessions
  /load <id> - Load a previous session
  /clear-history - Clear current session history
  /cd <path> - Change working directory
  /ls [path] - List files in directory (current if no path given)

Tips:
  • Type code requests: "add error handling" or "refactor this function"
  • Ask questions: "what does this do?" or "explain the auth flow"
  • Search code: "find all TODOs" or "show me database queries"
  • Press 'm' to switch between Plan and Auto mode
  • Press 'y' to accept changes, 'n' to reject

No telemetry. No cloud. Just you and your code.
)";
}

} // namespace commands
} // namespace zweek
