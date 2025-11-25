#pragma once

#include <string>

namespace zweek {
namespace tools {

// Compiler-based code validation (replaces Gatekeeper model)
class CompilerCheck {
public:
  CompilerCheck();
  ~CompilerCheck();

  // Check if code compiles (syntax only)
  bool IsValidCpp(const std::string &code);

  // Get compiler errors
  std::string GetErrors() const { return errors_; }

  // Check specific file
  bool CheckFile(const std::string &filepath);

private:
  std::string errors_;

  // Run MSVC syntax-only check
  bool RunSyntaxCheck(const std::string &filepath);
};

} // namespace tools
} // namespace zweek
