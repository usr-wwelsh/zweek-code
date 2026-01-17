#include "coder/recursive_agent.hpp"
#include <algorithm>
#include <iostream>

namespace zweek {
namespace coder {

// System prompt that defines the agent's behavior
const char* RecursiveAgent::GetSystemPrompt() {
    return "You are a code assistant. Use tools to answer questions about code.\n"
           "Commands: LIST <path> | READ_LINES <path> <start>-<end> | GREP <pattern> <path> | FINISH <answer>\n\n"
           "Example:\n"
           "TASK: List files in src/\n"
           "THOUGHT: I will list the src directory.\n"
           "CMD: LIST src/\n"
           "RESULT: main.cpp chat/ utils.cpp\n"
           "THOUGHT: I found the files. I will tell the user.\n"
           "CMD: FINISH The files in src/ are: main.cpp, chat/, utils.cpp\n\n"
           "RULES:\n"
           "1. Only use commands listed above\n"
           "2. FINISH must include the actual answer with details\n"
           "3. Do NOT create or modify files unless explicitly asked";
}

// GBNF grammar for constrained generation
const char* RecursiveAgent::GetAgentGrammar() {
    return
        "root ::= thought command\n"
        "thought ::= \"THOUGHT: \" thought-text \"\\n\"\n"
        "thought-text ::= [^\\n]+\n"
        "command ::= \"CMD: \" cmd-body\n"
        "cmd-body ::= read-cmd | grep-cmd | list-cmd | finish-cmd\n"
        "read-cmd ::= \"READ_LINES \" path \" \" line-range \"\\n\"\n"
        "grep-cmd ::= \"GREP \" pattern \" \" path \"\\n\"\n"
        "list-cmd ::= \"LIST \" path \"\\n\"\n"
        "finish-cmd ::= \"FINISH \" [^\\n]+ \"\\n\"\n"
        "line-range ::= number \"-\" number\n"
        "number ::= [0-9]+\n"
        "path ::= [a-zA-Z0-9_./-]+\n"
        "pattern ::= [a-zA-Z0-9_.*?]+";
}

RecursiveAgent::RecursiveAgent(const AgentConfig& config)
    : config_(config)
    , toolset_(".") {
}

RecursiveAgent::~RecursiveAgent() {
    Unload();
}

bool RecursiveAgent::Init() {
    ReportProgress("Loading model: " + config_.model_path);

    if (!model_.Load(config_.model_path, config_.context_window)) {
        if (callbacks_.on_error) {
            callbacks_.on_error("Failed to load model: " + config_.model_path);
        }
        state_ = AgentState::Error;
        return false;
    }

    ReportProgress("Model loaded successfully");
    return true;
}

void RecursiveAgent::StartTask(const std::string& task_description,
                                const std::string& working_directory) {
    Reset();
    current_task_ = task_description;
    toolset_.SetWorkingDirectory(working_directory);
    state_ = AgentState::Ready;

    ReportProgress("Starting task in: " + working_directory);
}

void RecursiveAgent::Reset() {
    history_.clear();
    step_count_ = 0;
    current_task_.clear();
    final_summary_.clear();
    state_ = AgentState::Ready;
}

void RecursiveAgent::Unload() {
    model_.Unload();
}

std::string RecursiveAgent::Run(std::atomic<bool>* interrupt_flag) {
    if (state_ == AgentState::Error) {
        return "Error: Agent in error state";
    }

    if (current_task_.empty()) {
        return "Error: No task set. Call StartTask first.";
    }

    state_ = AgentState::Ready;

    while (state_ != AgentState::Finished &&
           state_ != AgentState::Error &&
           state_ != AgentState::Interrupted) {

        if (interrupt_flag && interrupt_flag->load()) {
            state_ = AgentState::Interrupted;
            ReportProgress("Task interrupted by user");
            return "Task interrupted.";
        }

        if (step_count_ >= config_.max_steps) {
            state_ = AgentState::Error;
            std::string msg = "Maximum steps (" + std::to_string(config_.max_steps) +
                              ") reached. Task may be incomplete.";
            if (callbacks_.on_error) {
                callbacks_.on_error(msg);
            }
            return msg;
        }

        if (!Step(interrupt_flag)) {
            break;
        }
    }

    if (state_ == AgentState::Finished) {
        if (callbacks_.on_finish) {
            callbacks_.on_finish(final_summary_);
        }
        return final_summary_;
    }

    return "Task ended with state: " + std::to_string(static_cast<int>(state_));
}

bool RecursiveAgent::Step(std::atomic<bool>* interrupt_flag) {
    if (state_ == AgentState::Finished ||
        state_ == AgentState::Error ||
        state_ == AgentState::Interrupted) {
        return false;
    }

    step_count_++;
    ReportProgress("Step " + std::to_string(step_count_) + "/" +
                   std::to_string(config_.max_steps));

    // Build prompt with history
    state_ = AgentState::Thinking;
    std::string prompt = BuildPrompt();

    // Run inference with grammar constraint
    std::string model_output = model_.Infer(
        prompt,
        GetAgentGrammar(),
        config_.max_tokens_per_step,
        [this](const std::string& token) {
            if (callbacks_.on_stream) {
                callbacks_.on_stream(token);
            }
        },
        interrupt_flag
    );

    if (interrupt_flag && interrupt_flag->load()) {
        state_ = AgentState::Interrupted;
        return false;
    }

    // Parse output
    std::string thought, command;
    if (!ParseModelOutput(model_output, thought, command)) {
        // Try to recover - model might have produced partial output
        if (callbacks_.on_error) {
            callbacks_.on_error("Failed to parse model output: " + model_output);
        }
        state_ = AgentState::Error;
        return false;
    }

    // Report thought
    if (callbacks_.on_thought) {
        callbacks_.on_thought(thought);
    }

    // Report command
    if (callbacks_.on_command) {
        callbacks_.on_command(command);
    }

    // Execute command
    state_ = AgentState::Executing;
    ToolResult result = toolset_.Execute(command);

    // Report result
    if (callbacks_.on_tool_result) {
        callbacks_.on_tool_result(result);
    }

    // Record step in history
    AgentStep step;
    step.thought = thought;
    step.command = command;
    step.result = result;

    // Observation is the result of the previous step (or initial state for first step)
    if (history_.empty()) {
        step.observation = "Working directory: " + toolset_.GetWorkingDirectory() +
                           "\nTask: " + current_task_;
    } else {
        const auto& prev = history_.back();
        step.observation = prev.result.success ? prev.result.output
                                                : "ERROR: " + prev.result.error;
    }

    history_.push_back(step);

    // Check if finished
    if (result.finished) {
        state_ = AgentState::Finished;
        final_summary_ = result.output;
        return false;  // No more steps needed
    }

    state_ = AgentState::Ready;
    return true;  // Continue to next step
}

// Helper to truncate long strings
static std::string Truncate(const std::string& s, size_t max_len) {
    if (s.length() <= max_len) return s;
    return s.substr(0, max_len) + "...[truncated]";
}

std::string RecursiveAgent::BuildPrompt() const {
    std::stringstream ss;

    // Compact system prompt
    ss << GetSystemPrompt() << "\n\n";

    // Task
    ss << "TASK: " << current_task_ << "\n";
    ss << "DIR: " << toolset_.GetWorkingDirectory() << "\n\n";

    if (history_.empty()) {
        // First step - no history yet
        ss << "Begin by exploring. What is your first action?\n\n";
        ss << "THOUGHT:";
    } else {
        // Show what just happened
        const auto& last = history_.back();
        ss << "YOUR LAST ACTION:\n";
        ss << "CMD: " << last.command << "\n";
        ss << "RESULT:\n";
        if (last.result.success) {
            ss << Truncate(last.result.output, 1000) << "\n";
        } else {
            ss << "ERROR: " << last.result.error << "\n";
        }
        ss << "\nBased on this result, what is your NEXT action? (Use FINISH if done)\n\n";
        ss << "THOUGHT:";
    }

    return ss.str();
}

bool RecursiveAgent::ParseModelOutput(const std::string& output,
                                       std::string& thought,
                                       std::string& command) {
    // Find THOUGHT: prefix
    size_t thought_pos = output.find("THOUGHT:");
    if (thought_pos == std::string::npos) {
        // Try without the space
        thought_pos = output.find("THOUGHT:");
    }

    if (thought_pos == std::string::npos) {
        return false;
    }

    // Find CMD: prefix
    size_t cmd_pos = output.find("CMD:");
    if (cmd_pos == std::string::npos) {
        cmd_pos = output.find("CMD:");
    }

    if (cmd_pos == std::string::npos || cmd_pos <= thought_pos) {
        return false;
    }

    // Extract thought (between THOUGHT: and CMD:)
    size_t thought_start = thought_pos + 8;  // Length of "THOUGHT:"
    while (thought_start < cmd_pos && (output[thought_start] == ' ' || output[thought_start] == '\t')) {
        thought_start++;
    }

    thought = output.substr(thought_start, cmd_pos - thought_start);

    // Trim trailing whitespace/newlines from thought
    size_t end = thought.find_last_not_of(" \t\n\r");
    if (end != std::string::npos) {
        thought = thought.substr(0, end + 1);
    }

    // Extract command (everything after CMD:)
    size_t cmd_start = cmd_pos + 4;  // Length of "CMD:"
    while (cmd_start < output.size() && (output[cmd_start] == ' ' || output[cmd_start] == '\t')) {
        cmd_start++;
    }

    command = output.substr(cmd_start);

    // Trim trailing whitespace but preserve internal newlines (for WRITE/INSERT blocks)
    end = command.find_last_not_of(" \t\r");
    if (end != std::string::npos) {
        command = command.substr(0, end + 1);
    }

    return !thought.empty() && !command.empty();
}

void RecursiveAgent::ReportProgress(const std::string& message) {
    if (callbacks_.on_progress) {
        callbacks_.on_progress(message);
    }
}

}  // namespace coder
}  // namespace zweek
