#pragma once

#include "models/model_loader.hpp"
#include <string>
#include <vector>


namespace zweek {
namespace chat {

// Chat message structure
struct Message {
  std::string role; // "user" or "assistant"
  std::string content;
};

// Chat mode handler
class ChatMode {
public:
  ChatMode();
  ~ChatMode();

  // Load chat model (TinyLlama-Chat)
  bool LoadModel(const std::string &model_path);

  // Unload to free memory
  void UnloadModel();

  // Chat with context
  std::string Chat(const std::string &user_message,
                   const std::vector<std::string> &context_files);

  // Get conversation history
  const std::vector<Message> &GetHistory() const { return history_; }

  // Clear conversation
  void ClearHistory();

private:
  bool model_loaded_ = false;
  std::vector<Message> history_;
  models::ModelLoader model_loader_;
};

} // namespace chat
} // namespace zweek
