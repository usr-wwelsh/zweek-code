#include "coder/agent_toolset.hpp"
#include <cassert>
#include <iostream>
#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;

// Test helper: create a temp directory
fs::path create_test_dir() {
    fs::path test_dir = fs::temp_directory_path() / "zweek_agent_test";
    fs::create_directories(test_dir);
    return test_dir;
}

// Test helper: write a file
void write_test_file(const fs::path& path, const std::string& content) {
    std::ofstream file(path);
    file << content;
}

// Test helper: read a file
std::string read_test_file(const fs::path& path) {
    std::ifstream file(path);
    return std::string((std::istreambuf_iterator<char>(file)),
                       std::istreambuf_iterator<char>());
}

void test_read_lines() {
    std::cout << "Testing READ_LINES..." << std::endl;

    auto test_dir = create_test_dir();
    auto test_file = test_dir / "test.txt";

    // Create a file with 10 lines
    std::string content;
    for (int i = 1; i <= 10; ++i) {
        content += "Line " + std::to_string(i) + "\n";
    }
    write_test_file(test_file, content);

    zweek::coder::AgentToolSet toolset(test_dir.string());

    // Test reading middle lines
    auto result = toolset.ReadLines("test.txt", 3, 5);
    assert(result.success);
    assert(result.lines_returned == 3);
    assert(result.output.find("3: Line 3") != std::string::npos);
    assert(result.output.find("5: Line 5") != std::string::npos);

    // Test reading beyond EOF
    result = toolset.ReadLines("test.txt", 8, 15);
    assert(result.success);
    assert(result.lines_returned == 3);  // Lines 8, 9, 10
    assert(result.output.find("[EOF") != std::string::npos);

    // Test exceeding MAX_READ_LINES
    result = toolset.ReadLines("test.txt", 1, 100);
    assert(!result.success);
    assert(result.error.find("Too many lines") != std::string::npos);

    // Test file not found
    result = toolset.ReadLines("nonexistent.txt", 1, 5);
    assert(!result.success);
    assert(result.error.find("not found") != std::string::npos);

    fs::remove_all(test_dir);
    std::cout << "  PASSED" << std::endl;
}

void test_file_info() {
    std::cout << "Testing FILE_INFO..." << std::endl;

    auto test_dir = create_test_dir();
    auto test_file = test_dir / "info_test.txt";

    write_test_file(test_file, "Line 1\nLine 2\nLine 3\n");

    zweek::coder::AgentToolSet toolset(test_dir.string());

    // Test existing file
    auto result = toolset.FileInfo("info_test.txt");
    assert(result.success);
    assert(result.output.find("exists: true") != std::string::npos);
    assert(result.output.find("type: file") != std::string::npos);
    assert(result.output.find("line_count: 3") != std::string::npos);

    // Test non-existent file
    result = toolset.FileInfo("nonexistent.txt");
    assert(result.success);  // FileInfo always succeeds
    assert(result.output.find("exists: false") != std::string::npos);

    // Test directory
    fs::create_directory(test_dir / "subdir");
    result = toolset.FileInfo("subdir");
    assert(result.success);
    assert(result.output.find("type: directory") != std::string::npos);

    fs::remove_all(test_dir);
    std::cout << "  PASSED" << std::endl;
}

void test_list_dir() {
    std::cout << "Testing LIST..." << std::endl;

    auto test_dir = create_test_dir();
    write_test_file(test_dir / "file1.txt", "content");
    write_test_file(test_dir / "file2.cpp", "content");
    fs::create_directory(test_dir / "subdir");

    zweek::coder::AgentToolSet toolset(test_dir.string());

    auto result = toolset.ListDir(".");
    assert(result.success);
    assert(result.output.find("file1.txt") != std::string::npos);
    assert(result.output.find("file2.cpp") != std::string::npos);
    assert(result.output.find("subdir/") != std::string::npos);  // Dirs have trailing /

    fs::remove_all(test_dir);
    std::cout << "  PASSED" << std::endl;
}

void test_grep() {
    std::cout << "Testing GREP..." << std::endl;

    auto test_dir = create_test_dir();
    write_test_file(test_dir / "code.cpp", "int main() {\n    return 0;\n}\n");
    write_test_file(test_dir / "header.hpp", "int helper();\nint main();\n");

    zweek::coder::AgentToolSet toolset(test_dir.string());

    // Search for "main" in specific file
    auto result = toolset.Grep("main", "code.cpp");
    assert(result.success);
    assert(result.output.find("code.cpp:1:") != std::string::npos);
    assert(result.lines_returned == 1);

    // Search in directory
    result = toolset.Grep("main", ".");
    assert(result.success);
    assert(result.lines_returned >= 2);  // Found in both files

    // No matches
    result = toolset.Grep("foobar", "code.cpp");
    assert(result.success);
    assert(result.output.find("No matches") != std::string::npos);

    fs::remove_all(test_dir);
    std::cout << "  PASSED" << std::endl;
}

void test_write_lines() {
    std::cout << "Testing WRITE..." << std::endl;

    auto test_dir = create_test_dir();
    auto test_file = test_dir / "write_test.txt";
    write_test_file(test_file, "Line 1\nLine 2\nLine 3\nLine 4\nLine 5\n");

    zweek::coder::AgentToolSet toolset(test_dir.string());

    // Replace lines 2-3 with new content
    auto result = toolset.WriteLines("write_test.txt", 2, 3, "New Line A\nNew Line B\nNew Line C");
    assert(result.success);

    // Verify content
    std::string content = read_test_file(test_file);
    assert(content.find("Line 1") != std::string::npos);
    assert(content.find("New Line A") != std::string::npos);
    assert(content.find("New Line B") != std::string::npos);
    assert(content.find("New Line C") != std::string::npos);
    assert(content.find("Line 4") != std::string::npos);
    assert(content.find("Line 2") == std::string::npos);  // Original Line 2 should be gone

    fs::remove_all(test_dir);
    std::cout << "  PASSED" << std::endl;
}

void test_insert_lines() {
    std::cout << "Testing INSERT..." << std::endl;

    auto test_dir = create_test_dir();
    auto test_file = test_dir / "insert_test.txt";
    write_test_file(test_file, "Line 1\nLine 2\nLine 3\n");

    zweek::coder::AgentToolSet toolset(test_dir.string());

    // Insert after line 1
    auto result = toolset.InsertLines("insert_test.txt", 1, "Inserted A\nInserted B");
    assert(result.success);

    // Verify
    auto read_result = toolset.ReadLines("insert_test.txt", 1, 10);
    assert(read_result.success);
    assert(read_result.output.find("1: Line 1") != std::string::npos);
    assert(read_result.output.find("2: Inserted A") != std::string::npos);
    assert(read_result.output.find("3: Inserted B") != std::string::npos);
    assert(read_result.output.find("4: Line 2") != std::string::npos);

    fs::remove_all(test_dir);
    std::cout << "  PASSED" << std::endl;
}

void test_delete_lines() {
    std::cout << "Testing DELETE_LINES..." << std::endl;

    auto test_dir = create_test_dir();
    auto test_file = test_dir / "delete_test.txt";
    write_test_file(test_file, "Line 1\nLine 2\nLine 3\nLine 4\nLine 5\n");

    zweek::coder::AgentToolSet toolset(test_dir.string());

    // Delete lines 2-4
    auto result = toolset.DeleteLines("delete_test.txt", 2, 4);
    assert(result.success);

    // Verify
    auto read_result = toolset.ReadLines("delete_test.txt", 1, 10);
    assert(read_result.success);
    assert(read_result.lines_returned == 2);  // Only Line 1 and Line 5 remain
    assert(read_result.output.find("Line 1") != std::string::npos);
    assert(read_result.output.find("Line 5") != std::string::npos);
    assert(read_result.output.find("Line 2") == std::string::npos);

    fs::remove_all(test_dir);
    std::cout << "  PASSED" << std::endl;
}

void test_create_file() {
    std::cout << "Testing CREATE..." << std::endl;

    auto test_dir = create_test_dir();

    zweek::coder::AgentToolSet toolset(test_dir.string());

    // Create new file
    auto result = toolset.CreateFile("new_file.txt");
    assert(result.success);
    assert(fs::exists(test_dir / "new_file.txt"));

    // Try to create existing file
    result = toolset.CreateFile("new_file.txt");
    assert(!result.success);
    assert(result.error.find("already exists") != std::string::npos);

    // Create file in subdirectory (auto-creates dir)
    result = toolset.CreateFile("subdir/nested.txt");
    assert(result.success);
    assert(fs::exists(test_dir / "subdir" / "nested.txt"));

    fs::remove_all(test_dir);
    std::cout << "  PASSED" << std::endl;
}

void test_execute_command() {
    std::cout << "Testing Execute (command parsing)..." << std::endl;

    auto test_dir = create_test_dir();
    write_test_file(test_dir / "test.txt", "Hello\nWorld\n");

    zweek::coder::AgentToolSet toolset(test_dir.string());

    // Test READ_LINES command parsing
    auto result = toolset.Execute("READ_LINES test.txt 1-2");
    assert(result.success);
    assert(result.output.find("Hello") != std::string::npos);

    // Test LIST command
    result = toolset.Execute("LIST .");
    assert(result.success);
    assert(result.output.find("test.txt") != std::string::npos);

    // Test FILE_INFO command
    result = toolset.Execute("FILE_INFO test.txt");
    assert(result.success);
    assert(result.output.find("exists: true") != std::string::npos);

    // Test FINISH command
    result = toolset.Execute("FINISH Task completed successfully");
    assert(result.success);
    assert(result.finished);
    assert(result.output.find("Task completed") != std::string::npos);

    // Test unknown command
    result = toolset.Execute("UNKNOWN_CMD foo");
    assert(!result.success);
    assert(result.error.find("Unknown command") != std::string::npos);

    fs::remove_all(test_dir);
    std::cout << "  PASSED" << std::endl;
}

void test_path_safety() {
    std::cout << "Testing path safety (no directory traversal)..." << std::endl;

    auto test_dir = create_test_dir();
    write_test_file(test_dir / "safe.txt", "safe content");

    zweek::coder::AgentToolSet toolset(test_dir.string());

    // Try to read outside working directory
    auto result = toolset.ReadLines("../../../etc/passwd", 1, 5);
    assert(!result.success);
    assert(result.error.find("outside working directory") != std::string::npos);

    // Try absolute path outside working directory
    result = toolset.ReadLines("/etc/passwd", 1, 5);
    assert(!result.success);

    fs::remove_all(test_dir);
    std::cout << "  PASSED" << std::endl;
}

int main() {
    std::cout << "\n=== AgentToolSet Tests ===" << std::endl;
    std::cout << "Building the future of local AI!\n" << std::endl;

    try {
        test_read_lines();
        test_file_info();
        test_list_dir();
        test_grep();
        test_write_lines();
        test_insert_lines();
        test_delete_lines();
        test_create_file();
        test_execute_command();
        test_path_safety();

        std::cout << "\n=== All tests PASSED ===" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "\nTest FAILED with exception: " << e.what() << std::endl;
        return 1;
    }
}
