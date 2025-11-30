#include "chat/chat_mode.hpp"
#include "history/history_manager.hpp"
#include <fstream>

namespace zweek {
namespace chat {

ChatMode::ChatMode() {}
ChatMode::~ChatMode() { UnloadModel(); }

bool ChatMode::LoadModel(const std::string &model_path) {
  model_loaded_ = model_loader_.Load(model_path, 2048);
  return model_loaded_;
}

void ChatMode::UnloadModel() {
  model_loader_.Unload();
  model_loaded_ = false;
}

void ChatMode::ClearHistory() { 
  history_.clear(); 
  if (history_manager_) {
    history_manager_->ClearChatHistory();
  }
}

void ChatMode::LoadSessionHistory() {
  if (!history_manager_ || !history_manager_->IsInitialized()) return;
  
  // Get chat history from history manager
  auto chat_messages = history_manager_->GetChatHistory();
  
  // Convert to Message format and populate history_
  history_.clear();
  for (const auto& msg : chat_messages) {
    history_.push_back({msg.role, msg.content});
  }
}

std::string ChatMode::Chat(const std::string &user_message,
                           const std::vector<std::string> &context_files,
                           std::function<void(const std::string &)> stream_callback,
                           std::atomic<bool>* interrupt_flag) {
  if (!model_loaded_) {
    LoadModel("models/Qwen3-0.6B-Q8_0.gguf");
  }

  if (!model_loaded_) {
    return "Error: Chat model not loaded";
  }

  // Use ChatML format for Qwen3 with thinking trigger
  // Use ChatML format for Qwen3 with thinking trigger
  std::string prompt = 
    "<|im_start|>system\n"
    "You are a helpful coding assistant.<|im_end|>\n";

  // Add history (last 10 messages to fit context)
  int start_idx = std::max(0, (int)history_.size() - 10);
  for (int i = start_idx; i < history_.size(); ++i) {
    prompt += "<|im_start|>" + history_[i].role + "\n" + 
              history_[i].content + "<|im_end|>\n";
  }

  // Add current user message and trigger thinking
  prompt += "<|im_start|>user\n" + 
            user_message + "<|im_end|>\n" +
            "<|im_start|>assistant\n" +
            "<|im_start|>think\n";

  // Increased max tokens to 2048 to prevent cutoff
  // Wrap callback to detect stuck thinking
  int token_count = 0;
  bool thinking_ended = false;
  const int MAX_THINKING_TOKENS = 1000;
  bool limit_exceeded = false;

  auto wrapped_callback = [&](const std::string& chunk) {
    token_count++;
    
    // Check if thinking ended
    if (!thinking_ended && chunk.find("</think>") != std::string::npos) {
      thinking_ended = true;
    }

    // Check limit
    if (!thinking_ended && token_count > MAX_THINKING_TOKENS) {
      limit_exceeded = true;
      if (interrupt_flag) *interrupt_flag = true;
      stream_callback("\n</think>\n[Error: Thinking limit exceeded]");
      return;
    }

    stream_callback(chunk);
  };

  std::string response = model_loader_.Infer(prompt, "", 2048, wrapped_callback, interrupt_flag);

  // If limit exceeded, ensure we close the tag in the final response so TUI parses it
  if (limit_exceeded && response.find("</think>") == std::string::npos) {
      response += "\n</think>\n[Error: Thinking limit exceeded]";
  }

  // Check if we have an answer after </think>
  size_t think_end = response.rfind("</think>");
  
  // If </think> is missing, but we expected thinking (which we do), append it
  if (think_end == std::string::npos) {
      response += "\n</think>";
      think_end = response.length() - 8;
  }

  if (think_end != std::string::npos) {
    std::string after_think = response.substr(think_end + 8);
    // Check if it's just whitespace
    bool has_answer = false;
    for (char c : after_think) {
      if (!std::isspace(c)) {
        has_answer = true;
        break;
      }
    }
    
    // If no answer, re-prompt to continue generation
    if (!has_answer) {
      // Append the thinking to the prompt and ask for continuation
      std::string continuation_prompt = prompt + response;
      
      // We need to stream this too, but we don't want to duplicate the thinking part in the UI
      // The UI already has the thinking part. We just want the answer.
      // But ModelLoader::Infer streams everything it generates.
      // Since we are re-prompting with the thinking included in the prompt, 
      // the model will ONLY generate the answer (continuation).
      // So we can just pass the original stream_callback!
      
      std::string answer = model_loader_.Infer(continuation_prompt, "", 2048, stream_callback, interrupt_flag);
      response += answer;
    }
  }

  // DEBUG: Log raw response to file
  {
    std::ofstream debug_log("debug_response.txt", std::ios::app);
    debug_log << "--- New Response ---\n";
    debug_log << response << "\n";
    debug_log << "--------------------\n";
  }

  // Update in-memory history
  history_.push_back({"user", user_message});
  history_.push_back({"assistant", response});
  
  // Persist to history manager if available
  if (history_manager_ && history_manager_->IsInitialized()) {
    history_manager_->LogChatMessage("user", user_message);
    history_manager_->LogChatMessage("assistant", response);
  }

  return response;
}

} // namespace chat
} // namespace zweek
