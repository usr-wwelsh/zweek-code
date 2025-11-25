#include "commands/command_handler.hpp"

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

  // Handle /help
  if (cmd == "help") {
    result.handled = true;
    result.response = GetHelpText();
  }

  return result;
}

std::string CommandHandler::GetHelpText() {
  return R"(
Zweek Code - AI that runs on YOUR machine, not the cloud.
Seven specialized models working together to keep your code private and your workflow fast.

Available Commands:
  /help - Show this message

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
