#include "chat/chat_mode.hpp"

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

void ChatMode::ClearHistory() { history_.clear(); }

std::string ChatMode::Chat(const std::string &user_message,
                           const std::vector<std::string> &context_files) {
  if (!model_loaded_) {
    LoadModel("models/tinyllama-chat.gguf");
  }

  if (!model_loaded_) {
    return "Error: Chat model not loaded";
  }

  std::string prompt =
      "You are a helpful coding assistant. Answer: " + user_message;
  std::string response = model_loader_.Infer(prompt, "", 512);

  history_.push_back({"user", user_message});
  history_.push_back({"assistant", response});

  return response;
}

} // namespace chat
} // namespace zweek
