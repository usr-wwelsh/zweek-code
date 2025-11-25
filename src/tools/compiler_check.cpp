#include "tools/compiler_check.hpp"
#include <cstdlib>
#include <fstream>


namespace zweek {
namespace tools {

CompilerCheck::CompilerCheck() {
  // Constructor
}

CompilerCheck::~CompilerCheck() {
  // Destructor
}

bool CompilerCheck::IsValidCpp(const std::string &code) {
  // Write code to temporary file
  std::string temp_file = "zweek_temp_check.cpp";
  std::ofstream out(temp_file);
  if (!out) {
    errors_ = "Failed to create temp file";
    return false;
  }

  out << code;
  out.close();

  // Run syntax check
  bool result = RunSyntaxCheck(temp_file);

  // Clean up
  std::remove(temp_file.c_str());
  std::remove("zweek_errors.txt");

  return result;
}

bool CompilerCheck::CheckFile(const std::string &filepath) {
  return RunSyntaxCheck(filepath);
}

bool CompilerCheck::RunSyntaxCheck(const std::string &filepath) {
  // Run MSVC syntax-only check
  // /Zs = syntax check only (no code generation)
  // /EHsc = enable C++ exceptions
  // Redirect stderr to file
  std::string command =
      "cl.exe /Zs /EHsc /std:c++17 \"" + filepath + "\" 2> zweek_errors.txt";

  int result = std::system(command.c_str());

  // Read errors if any
  std::ifstream error_file("zweek_errors.txt");
  if (error_file) {
    std::string line;
    errors_.clear();
    while (std::getline(error_file, line)) {
      errors_ += line + "\n";
    }
  }

  // Return true if no errors
  return (result == 0);
}

} // namespace tools
} // namespace zweek
