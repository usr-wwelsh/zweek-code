#pragma once

#include "coder/agent_toolset.hpp"
#include "models/model_loader.hpp"
#include <string>
#include <vector>
#include <functional>
#include <atomic>
#include <sstream>

namespace zweek {
namespace coder {

// A single step in the agent's history
struct AgentStep {
    std::string observation;   // Tool output from previous step (or initial task)
    std::string thought;       // Model's reasoning
    std::string command;       // The CMD: output from model
    ToolResult result;         // Result of executing the command
};

// Agent state
enum class AgentState {
    Ready,          // Waiting for task
    Thinking,       // Running inference
    Executing,      // Running tool
    Finished,       // Task complete (FINISH called)
    Error,          // Unrecoverable error
    Interrupted     // User interrupted
};

// Configuration for the agent
struct AgentConfig {
    std::string model_path = "models/Qwen3-0.6B-Q8_0.gguf";
    int max_steps = 25;             // Safety limit on iterations
    int max_tokens_per_step = 512;  // Token limit per inference
    int context_window = 2048;      // Model context size
    int history_window = 8;         // Max steps to keep in prompt
};

// Callbacks for UI integration
struct AgentCallbacks {
    std::function<void(const std::string&)> on_thought;     // Model is thinking
    std::function<void(const std::string&)> on_command;     // Model issued command
    std::function<void(const ToolResult&)> on_tool_result;  // Tool executed
    std::function<void(const std::string&)> on_progress;    // Status updates
    std::function<void(const std::string&)> on_finish;      // Task complete
    std::function<void(const std::string&)> on_error;       // Error occurred
    std::function<void(const std::string&)> on_stream;      // Token streaming
};

// The Recursive Language Model Agent
// Implements: Observation -> Inference -> Action -> Tool -> Observation loop
class RecursiveAgent {
public:
    explicit RecursiveAgent(const AgentConfig& config);
    ~RecursiveAgent();

    // Initialize (loads model)
    bool Init();

    // Start a new task
    void StartTask(const std::string& task_description,
                   const std::string& working_directory);

    // Run the full agent loop until FINISH or max_steps
    // Returns final summary or error
    std::string Run(std::atomic<bool>* interrupt_flag = nullptr);

    // Run a single step (for debugging/stepping through)
    // Returns false if agent is Finished or Error
    bool Step(std::atomic<bool>* interrupt_flag = nullptr);

    // Get current state
    AgentState GetState() const { return state_; }

    // Get step history
    const std::vector<AgentStep>& GetHistory() const { return history_; }

    // Get current step count
    int GetStepCount() const { return step_count_; }

    // Set callbacks
    void SetCallbacks(const AgentCallbacks& callbacks) { callbacks_ = callbacks; }

    // Reset for new task (keeps model loaded)
    void Reset();

    // Unload model to free memory
    void Unload();

    // Check if model is loaded
    bool IsModelLoaded() const { return model_.IsLoaded(); }

private:
    AgentConfig config_;
    AgentCallbacks callbacks_;
    AgentState state_ = AgentState::Ready;

    models::ModelLoader model_;
    AgentToolSet toolset_;

    std::string current_task_;
    std::vector<AgentStep> history_;
    int step_count_ = 0;
    std::string final_summary_;

    // Build the prompt from history
    std::string BuildPrompt() const;

    // Parse model output into thought + command
    bool ParseModelOutput(const std::string& output,
                          std::string& thought,
                          std::string& command);

    // Extract content block for WRITE/INSERT commands
    std::string ExtractContentBlock(const std::string& command,
                                    const std::string& end_marker);

    // Report progress
    void ReportProgress(const std::string& message);

    // System prompt - defines the agent's capabilities
    static const char* GetSystemPrompt();

    // Get the GBNF grammar for constrained generation
    static const char* GetAgentGrammar();
};

}  // namespace coder
}  // namespace zweek
