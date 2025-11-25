#pragma once

#include <functional>
#include <string>


namespace zweek {
namespace commands {

// Command handler result
struct CommandResult {
  bool handled = false;
  std::string response;
};

// Command system
class CommandHandler {
public:
  CommandHandler();

  // Check if input is a command and handle it
  CommandResult HandleCommand(const std::string &input);

private:
  std::string GetHelpText();
};

} // namespace commands
} // namespace zweek
