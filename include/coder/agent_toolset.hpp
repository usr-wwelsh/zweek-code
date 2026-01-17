#pragma once

#include <string>
#include <vector>
#include <filesystem>
#include <optional>
#include <regex>

namespace zweek {
namespace coder {

// Result of a tool execution
struct ToolResult {
    bool success = false;
    std::string output;      // The data returned (file lines, search results, etc.)
    std::string error;       // Error message if !success
    int lines_returned = 0;  // For READ_LINES: actual lines returned
    bool truncated = false;  // True if output was capped
    bool finished = false;   // True if FINISH command was executed
};

// Constrained tool executor for the Recursive Agent
// Implements the "prosthetic" interface - model never sees full files
class AgentToolSet {
public:
    // Hard limits (force model to be precise)
    static constexpr int MAX_READ_LINES = 50;
    static constexpr int MAX_GREP_RESULTS = 20;
    static constexpr int MAX_LIST_ENTRIES = 100;
    static constexpr int MAX_WRITE_LINES = 200;
    static constexpr int MAX_PATH_LENGTH = 256;

    explicit AgentToolSet(const std::string& working_dir = ".");

    // READ_LINES <path> <start>-<end>
    // Returns lines [start, end] inclusive (1-indexed)
    // Enforces MAX_READ_LINES limit
    ToolResult ReadLines(const std::string& path, int start_line, int end_line);

    // GREP <pattern> <path_or_glob>
    // Returns matching lines with line numbers
    // Capped at MAX_GREP_RESULTS matches
    ToolResult Grep(const std::string& pattern, const std::string& path);

    // LIST <path>
    // Returns directory listing with file types
    ToolResult ListDir(const std::string& path);

    // FILE_INFO <path>
    // Returns: exists, line_count, size_bytes (no content!)
    ToolResult FileInfo(const std::string& path);

    // WRITE <path> <start_line> <end_line>
    // Replaces lines [start, end] with new content
    ToolResult WriteLines(const std::string& path, int start_line, int end_line,
                          const std::string& new_content);

    // INSERT <path> <after_line>
    // Inserts new content after specified line (0 = beginning)
    ToolResult InsertLines(const std::string& path, int after_line,
                           const std::string& new_content);

    // DELETE_LINES <path> <start>-<end>
    ToolResult DeleteLines(const std::string& path, int start_line, int end_line);

    // CREATE <path>
    // Creates a new empty file
    ToolResult CreateFile(const std::string& path);

    // FINISH <summary>
    // Signals task completion
    ToolResult Finish(const std::string& summary);

    // Parse and execute a command string from the model
    ToolResult Execute(const std::string& command);

    // Setters
    void SetWorkingDirectory(const std::string& path);
    std::string GetWorkingDirectory() const { return working_dir_; }

private:
    std::string working_dir_;

    // Path resolution and safety
    std::filesystem::path ResolvePath(const std::string& relative);
    bool IsPathSafe(const std::filesystem::path& resolved);

    // File reading helper
    std::vector<std::string> ReadFileLines(const std::filesystem::path& path);
    bool WriteFileLines(const std::filesystem::path& path,
                        const std::vector<std::string>& lines);

    // Command parsing helpers
    std::optional<std::pair<int, int>> ParseLineRange(const std::string& range);
    std::string ExtractQuotedOrWord(const std::string& input, size_t& pos);
};

}  // namespace coder
}  // namespace zweek
