#include "coder/agent_toolset.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>

namespace zweek {
namespace coder {

AgentToolSet::AgentToolSet(const std::string& working_dir)
    : working_dir_(working_dir) {
    if (working_dir_.empty()) {
        working_dir_ = std::filesystem::current_path().string();
    }
}

void AgentToolSet::SetWorkingDirectory(const std::string& path) {
    auto resolved = std::filesystem::absolute(path);
    if (std::filesystem::exists(resolved) && std::filesystem::is_directory(resolved)) {
        working_dir_ = resolved.string();
    }
}

std::filesystem::path AgentToolSet::ResolvePath(const std::string& relative) {
    std::filesystem::path base(working_dir_);
    std::filesystem::path rel(relative);

    // If absolute, use as-is but check safety
    if (rel.is_absolute()) {
        return std::filesystem::weakly_canonical(rel);
    }

    // Resolve relative to working directory
    return std::filesystem::weakly_canonical(base / rel);
}

bool AgentToolSet::IsPathSafe(const std::filesystem::path& resolved) {
    // Ensure path is within or equal to working directory
    std::filesystem::path base = std::filesystem::weakly_canonical(working_dir_);
    std::filesystem::path target = std::filesystem::weakly_canonical(resolved);

    // Check if target starts with base
    auto base_str = base.string();
    auto target_str = target.string();

    return target_str.find(base_str) == 0;
}

std::vector<std::string> AgentToolSet::ReadFileLines(const std::filesystem::path& path) {
    std::vector<std::string> lines;
    std::ifstream file(path);

    if (!file.is_open()) {
        return lines;
    }

    std::string line;
    while (std::getline(file, line)) {
        lines.push_back(line);
    }

    return lines;
}

bool AgentToolSet::WriteFileLines(const std::filesystem::path& path,
                                   const std::vector<std::string>& lines) {
    // Ensure parent directory exists
    auto parent = path.parent_path();
    if (!parent.empty() && !std::filesystem::exists(parent)) {
        std::filesystem::create_directories(parent);
    }

    std::ofstream file(path);
    if (!file.is_open()) {
        return false;
    }

    for (size_t i = 0; i < lines.size(); ++i) {
        file << lines[i];
        if (i < lines.size() - 1) {
            file << "\n";
        }
    }

    // Add final newline if file is not empty
    if (!lines.empty()) {
        file << "\n";
    }

    return true;
}

std::optional<std::pair<int, int>> AgentToolSet::ParseLineRange(const std::string& range) {
    size_t dash = range.find('-');
    if (dash == std::string::npos) {
        return std::nullopt;
    }

    try {
        int start = std::stoi(range.substr(0, dash));
        int end = std::stoi(range.substr(dash + 1));
        return std::make_pair(start, end);
    } catch (...) {
        return std::nullopt;
    }
}

std::string AgentToolSet::ExtractQuotedOrWord(const std::string& input, size_t& pos) {
    // Skip whitespace
    while (pos < input.size() && std::isspace(input[pos])) {
        ++pos;
    }

    if (pos >= input.size()) {
        return "";
    }

    // Check for quoted string
    if (input[pos] == '"') {
        ++pos;
        size_t start = pos;
        while (pos < input.size() && input[pos] != '"') {
            ++pos;
        }
        std::string result = input.substr(start, pos - start);
        if (pos < input.size()) ++pos;  // Skip closing quote
        return result;
    }

    // Extract word
    size_t start = pos;
    while (pos < input.size() && !std::isspace(input[pos])) {
        ++pos;
    }

    return input.substr(start, pos - start);
}

ToolResult AgentToolSet::ReadLines(const std::string& path, int start_line, int end_line) {
    ToolResult result;

    // Validate line range
    if (start_line < 1 || end_line < start_line) {
        result.error = "Invalid line range. Use 1-indexed positive integers.";
        return result;
    }

    int requested = end_line - start_line + 1;
    if (requested > MAX_READ_LINES) {
        result.error = "Too many lines requested (" + std::to_string(requested) +
                       "). Maximum is " + std::to_string(MAX_READ_LINES) +
                       ". Narrow your request.";
        return result;
    }

    // Resolve and validate path
    auto resolved = ResolvePath(path);
    if (!IsPathSafe(resolved)) {
        result.error = "Path outside working directory.";
        return result;
    }

    if (!std::filesystem::exists(resolved)) {
        result.error = "File not found: " + path;
        return result;
    }

    // Read file
    auto lines = ReadFileLines(resolved);
    if (lines.empty() && std::filesystem::file_size(resolved) > 0) {
        result.error = "Failed to read file.";
        return result;
    }

    // Extract requested range
    std::stringstream ss;
    int actual_start = std::min(start_line, (int)lines.size() + 1);
    int actual_end = std::min(end_line, (int)lines.size());

    for (int i = actual_start; i <= actual_end; ++i) {
        ss << i << ": " << lines[i - 1] << "\n";
        result.lines_returned++;
    }

    if (actual_end < end_line) {
        ss << "[EOF at line " << lines.size() << "]\n";
    }

    result.success = true;
    result.output = ss.str();

    return result;
}

ToolResult AgentToolSet::Grep(const std::string& pattern, const std::string& path) {
    ToolResult result;

    auto resolved = ResolvePath(path);
    if (!IsPathSafe(resolved)) {
        result.error = "Path outside working directory.";
        return result;
    }

    std::vector<std::filesystem::path> files_to_search;

    // Check if it's a directory or file
    if (std::filesystem::is_directory(resolved)) {
        // Search all files in directory (non-recursive for safety)
        for (const auto& entry : std::filesystem::directory_iterator(resolved)) {
            if (entry.is_regular_file()) {
                files_to_search.push_back(entry.path());
            }
        }
    } else if (std::filesystem::exists(resolved)) {
        files_to_search.push_back(resolved);
    } else {
        result.error = "Path not found: " + path;
        return result;
    }

    // Compile regex
    std::regex regex;
    try {
        regex = std::regex(pattern, std::regex::ECMAScript | std::regex::icase);
    } catch (const std::regex_error& e) {
        result.error = "Invalid regex pattern: " + std::string(e.what());
        return result;
    }

    std::stringstream ss;
    int match_count = 0;

    for (const auto& file : files_to_search) {
        auto lines = ReadFileLines(file);
        auto rel_path = std::filesystem::relative(file, working_dir_).string();

        for (size_t i = 0; i < lines.size() && match_count < MAX_GREP_RESULTS; ++i) {
            if (std::regex_search(lines[i], regex)) {
                ss << rel_path << ":" << (i + 1) << ": " << lines[i] << "\n";
                match_count++;
            }
        }

        if (match_count >= MAX_GREP_RESULTS) {
            result.truncated = true;
            break;
        }
    }

    if (match_count == 0) {
        ss << "No matches found for pattern: " << pattern << "\n";
    } else if (result.truncated) {
        ss << "[Results truncated at " << MAX_GREP_RESULTS << " matches]\n";
    }

    result.success = true;
    result.output = ss.str();
    result.lines_returned = match_count;

    return result;
}

ToolResult AgentToolSet::ListDir(const std::string& path) {
    ToolResult result;

    auto resolved = ResolvePath(path);
    if (!IsPathSafe(resolved)) {
        result.error = "Path outside working directory.";
        return result;
    }

    if (!std::filesystem::exists(resolved)) {
        result.error = "Directory not found: " + path;
        return result;
    }

    if (!std::filesystem::is_directory(resolved)) {
        result.error = "Not a directory: " + path;
        return result;
    }

    std::vector<std::string> entries;
    for (const auto& entry : std::filesystem::directory_iterator(resolved)) {
        std::string name = entry.path().filename().string();
        if (entry.is_directory()) {
            name += "/";
        }
        entries.push_back(name);
    }

    // Sort alphabetically
    std::sort(entries.begin(), entries.end());

    std::stringstream ss;
    int count = 0;
    for (const auto& entry : entries) {
        if (count >= MAX_LIST_ENTRIES) {
            result.truncated = true;
            ss << "[... " << (entries.size() - count) << " more entries]\n";
            break;
        }
        ss << entry << "\n";
        count++;
    }

    if (entries.empty()) {
        ss << "[Empty directory]\n";
    }

    result.success = true;
    result.output = ss.str();
    result.lines_returned = count;

    return result;
}

ToolResult AgentToolSet::FileInfo(const std::string& path) {
    ToolResult result;

    auto resolved = ResolvePath(path);
    if (!IsPathSafe(resolved)) {
        result.error = "Path outside working directory.";
        return result;
    }

    std::stringstream ss;

    if (!std::filesystem::exists(resolved)) {
        ss << "exists: false\n";
        ss << "path: " << path << "\n";
        result.success = true;
        result.output = ss.str();
        return result;
    }

    ss << "exists: true\n";
    ss << "path: " << path << "\n";

    if (std::filesystem::is_directory(resolved)) {
        ss << "type: directory\n";
        int count = 0;
        for (const auto& _ : std::filesystem::directory_iterator(resolved)) {
            (void)_;
            count++;
        }
        ss << "entries: " << count << "\n";
    } else {
        ss << "type: file\n";
        ss << "size_bytes: " << std::filesystem::file_size(resolved) << "\n";

        // Count lines
        auto lines = ReadFileLines(resolved);
        ss << "line_count: " << lines.size() << "\n";
    }

    result.success = true;
    result.output = ss.str();

    return result;
}

ToolResult AgentToolSet::WriteLines(const std::string& path, int start_line, int end_line,
                                     const std::string& new_content) {
    ToolResult result;

    // Validate range
    if (start_line < 1 || end_line < start_line) {
        result.error = "Invalid line range.";
        return result;
    }

    auto resolved = ResolvePath(path);
    if (!IsPathSafe(resolved)) {
        result.error = "Path outside working directory.";
        return result;
    }

    if (!std::filesystem::exists(resolved)) {
        result.error = "File not found. Use CREATE first for new files.";
        return result;
    }

    // Read existing content
    auto lines = ReadFileLines(resolved);

    // Parse new content into lines
    std::vector<std::string> new_lines;
    std::istringstream iss(new_content);
    std::string line;
    while (std::getline(iss, line)) {
        new_lines.push_back(line);
    }

    if (new_lines.size() > MAX_WRITE_LINES) {
        result.error = "Too many lines to write (" + std::to_string(new_lines.size()) +
                       "). Maximum is " + std::to_string(MAX_WRITE_LINES) + ".";
        return result;
    }

    // Adjust indices (1-indexed to 0-indexed)
    int idx_start = start_line - 1;
    int idx_end = std::min(end_line - 1, (int)lines.size() - 1);

    // Build new file content
    std::vector<std::string> result_lines;

    // Lines before replacement
    for (int i = 0; i < idx_start && i < (int)lines.size(); ++i) {
        result_lines.push_back(lines[i]);
    }

    // Pad with empty lines if start is beyond current file
    while ((int)result_lines.size() < idx_start) {
        result_lines.push_back("");
    }

    // New content
    for (const auto& nl : new_lines) {
        result_lines.push_back(nl);
    }

    // Lines after replacement
    for (int i = idx_end + 1; i < (int)lines.size(); ++i) {
        result_lines.push_back(lines[i]);
    }

    if (!WriteFileLines(resolved, result_lines)) {
        result.error = "Failed to write file.";
        return result;
    }

    result.success = true;
    result.output = "Replaced lines " + std::to_string(start_line) + "-" +
                    std::to_string(end_line) + " with " +
                    std::to_string(new_lines.size()) + " new lines.\n";
    result.output += "File now has " + std::to_string(result_lines.size()) + " lines.\n";

    return result;
}

ToolResult AgentToolSet::InsertLines(const std::string& path, int after_line,
                                      const std::string& new_content) {
    ToolResult result;

    if (after_line < 0) {
        result.error = "Invalid line number. Use 0 to insert at beginning.";
        return result;
    }

    auto resolved = ResolvePath(path);
    if (!IsPathSafe(resolved)) {
        result.error = "Path outside working directory.";
        return result;
    }

    if (!std::filesystem::exists(resolved)) {
        result.error = "File not found. Use CREATE first for new files.";
        return result;
    }

    auto lines = ReadFileLines(resolved);

    // Parse new content
    std::vector<std::string> new_lines;
    std::istringstream iss(new_content);
    std::string line;
    while (std::getline(iss, line)) {
        new_lines.push_back(line);
    }

    if (new_lines.size() > MAX_WRITE_LINES) {
        result.error = "Too many lines to insert.";
        return result;
    }

    // Insert at position
    int insert_pos = std::min(after_line, (int)lines.size());
    lines.insert(lines.begin() + insert_pos, new_lines.begin(), new_lines.end());

    if (!WriteFileLines(resolved, lines)) {
        result.error = "Failed to write file.";
        return result;
    }

    result.success = true;
    result.output = "Inserted " + std::to_string(new_lines.size()) +
                    " lines after line " + std::to_string(after_line) + ".\n";
    result.output += "File now has " + std::to_string(lines.size()) + " lines.\n";

    return result;
}

ToolResult AgentToolSet::DeleteLines(const std::string& path, int start_line, int end_line) {
    ToolResult result;

    if (start_line < 1 || end_line < start_line) {
        result.error = "Invalid line range.";
        return result;
    }

    auto resolved = ResolvePath(path);
    if (!IsPathSafe(resolved)) {
        result.error = "Path outside working directory.";
        return result;
    }

    if (!std::filesystem::exists(resolved)) {
        result.error = "File not found.";
        return result;
    }

    auto lines = ReadFileLines(resolved);

    int idx_start = start_line - 1;
    int idx_end = std::min(end_line - 1, (int)lines.size() - 1);

    if (idx_start >= (int)lines.size()) {
        result.error = "Start line beyond end of file.";
        return result;
    }

    int deleted = idx_end - idx_start + 1;
    lines.erase(lines.begin() + idx_start, lines.begin() + idx_end + 1);

    if (!WriteFileLines(resolved, lines)) {
        result.error = "Failed to write file.";
        return result;
    }

    result.success = true;
    result.output = "Deleted " + std::to_string(deleted) + " lines.\n";
    result.output += "File now has " + std::to_string(lines.size()) + " lines.\n";

    return result;
}

ToolResult AgentToolSet::CreateFile(const std::string& path) {
    ToolResult result;

    auto resolved = ResolvePath(path);
    if (!IsPathSafe(resolved)) {
        result.error = "Path outside working directory.";
        return result;
    }

    if (std::filesystem::exists(resolved)) {
        result.error = "File already exists. Use WRITE to modify.";
        return result;
    }

    // Create parent directories
    auto parent = resolved.parent_path();
    if (!parent.empty() && !std::filesystem::exists(parent)) {
        std::filesystem::create_directories(parent);
    }

    // Create empty file
    std::ofstream file(resolved);
    if (!file.is_open()) {
        result.error = "Failed to create file.";
        return result;
    }
    file.close();

    result.success = true;
    result.output = "Created empty file: " + path + "\n";

    return result;
}

ToolResult AgentToolSet::Finish(const std::string& summary) {
    ToolResult result;
    result.success = true;
    result.finished = true;
    result.output = summary;
    return result;
}

ToolResult AgentToolSet::Execute(const std::string& command) {
    ToolResult result;

    // Trim whitespace
    std::string cmd = command;
    size_t start = cmd.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) {
        result.error = "Empty command.";
        return result;
    }
    cmd = cmd.substr(start);

    // Parse command type
    size_t space_pos = cmd.find(' ');
    std::string cmd_type = (space_pos != std::string::npos)
                               ? cmd.substr(0, space_pos)
                               : cmd;

    // Convert to uppercase for matching
    std::transform(cmd_type.begin(), cmd_type.end(), cmd_type.begin(), ::toupper);

    std::string args = (space_pos != std::string::npos)
                           ? cmd.substr(space_pos + 1)
                           : "";

    // Dispatch to appropriate handler
    if (cmd_type == "READ_LINES") {
        size_t pos = 0;
        std::string path = ExtractQuotedOrWord(args, pos);
        std::string range = ExtractQuotedOrWord(args, pos);

        auto line_range = ParseLineRange(range);
        if (!line_range) {
            result.error = "Invalid line range format. Use: READ_LINES <path> <start>-<end>";
            return result;
        }

        return ReadLines(path, line_range->first, line_range->second);
    }
    else if (cmd_type == "GREP") {
        size_t pos = 0;
        std::string pattern = ExtractQuotedOrWord(args, pos);
        std::string path = ExtractQuotedOrWord(args, pos);

        if (path.empty()) path = ".";

        return Grep(pattern, path);
    }
    else if (cmd_type == "LIST") {
        std::string path = args.empty() ? "." : args;
        // Trim any trailing whitespace/newlines
        size_t end = path.find_last_not_of(" \t\n\r");
        if (end != std::string::npos) {
            path = path.substr(0, end + 1);
        }
        return ListDir(path);
    }
    else if (cmd_type == "FILE_INFO") {
        std::string path = args;
        size_t end = path.find_last_not_of(" \t\n\r");
        if (end != std::string::npos) {
            path = path.substr(0, end + 1);
        }
        return FileInfo(path);
    }
    else if (cmd_type == "CREATE") {
        std::string path = args;
        size_t end = path.find_last_not_of(" \t\n\r");
        if (end != std::string::npos) {
            path = path.substr(0, end + 1);
        }
        return CreateFile(path);
    }
    else if (cmd_type == "DELETE_LINES") {
        size_t pos = 0;
        std::string path = ExtractQuotedOrWord(args, pos);
        std::string range = ExtractQuotedOrWord(args, pos);

        auto line_range = ParseLineRange(range);
        if (!line_range) {
            result.error = "Invalid format. Use: DELETE_LINES <path> <start>-<end>";
            return result;
        }

        return DeleteLines(path, line_range->first, line_range->second);
    }
    else if (cmd_type == "WRITE") {
        // WRITE <path> <start>-<end>
        // Content follows until END_WRITE
        size_t pos = 0;
        std::string path = ExtractQuotedOrWord(args, pos);
        std::string range = ExtractQuotedOrWord(args, pos);

        auto line_range = ParseLineRange(range);
        if (!line_range) {
            result.error = "Invalid format. Use: WRITE <path> <start>-<end>\\n<content>\\nEND_WRITE";
            return result;
        }

        // Extract content (everything after newline until END_WRITE)
        size_t newline = args.find('\n', pos);
        if (newline == std::string::npos) {
            result.error = "Missing content block. Content should follow on next line.";
            return result;
        }

        std::string content_block = args.substr(newline + 1);
        size_t end_marker = content_block.find("END_WRITE");
        if (end_marker != std::string::npos) {
            content_block = content_block.substr(0, end_marker);
        }

        // Remove trailing newline if present
        if (!content_block.empty() && content_block.back() == '\n') {
            content_block.pop_back();
        }

        return WriteLines(path, line_range->first, line_range->second, content_block);
    }
    else if (cmd_type == "INSERT") {
        // INSERT <path> <after_line>
        size_t pos = 0;
        std::string path = ExtractQuotedOrWord(args, pos);
        std::string line_str = ExtractQuotedOrWord(args, pos);

        int after_line;
        try {
            after_line = std::stoi(line_str);
        } catch (...) {
            result.error = "Invalid line number.";
            return result;
        }

        // Extract content
        size_t newline = args.find('\n', pos);
        if (newline == std::string::npos) {
            result.error = "Missing content block.";
            return result;
        }

        std::string content_block = args.substr(newline + 1);
        size_t end_marker = content_block.find("END_INSERT");
        if (end_marker != std::string::npos) {
            content_block = content_block.substr(0, end_marker);
        }

        if (!content_block.empty() && content_block.back() == '\n') {
            content_block.pop_back();
        }

        return InsertLines(path, after_line, content_block);
    }
    else if (cmd_type == "FINISH") {
        return Finish(args);
    }
    else {
        result.error = "Unknown command: " + cmd_type + "\n"
                       "Available: READ_LINES, GREP, LIST, FILE_INFO, WRITE, INSERT, DELETE_LINES, CREATE, FINISH";
        return result;
    }
}

}  // namespace coder
}  // namespace zweek
