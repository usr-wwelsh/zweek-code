#include "ui/tui.hpp"
#include "ui/branding.hpp"
#include <ftxui/component/component_options.hpp>
#include <ftxui/dom/elements.hpp>
#include <sstream>

using namespace ftxui;

namespace zweek {
namespace ui {

TUI::TUI() : screen_(ScreenInteractive::Fullscreen()) {
  state_.status_message = "Ready";
  state_.conversation_history.push_back(
      "Welcome to Zweek Code - Local AI Coding Assistant");
  state_.conversation_history.push_back("Type your request and press Enter...");
  state_.conversation_history.push_back("");
}

TUI::~TUI() = default;

void TUI::Run() {
  auto layout = CreateLayout();
  screen_.Loop(layout);
}

void TUI::UpdateStage(PipelineStage stage, float progress) {
  state_.current_stage = stage;
  state_.progress = progress;
  state_.status_message = StageToString(stage);
  state_.conversation_history.push_back("[" + StageToString(stage) + "]");
  screen_.PostEvent(Event::Custom);
}

void TUI::SetCodePreview(const std::string &code) {
  state_.code_preview = code;
  state_.conversation_history.push_back("");
  state_.conversation_history.push_back("Generated code:");
  state_.conversation_history.push_back(code);
  screen_.PostEvent(Event::Custom);
}

void TUI::SetQualityReport(const std::string &report) {
  state_.quality_report = report;
  state_.conversation_history.push_back("Quality: " + report);
  screen_.PostEvent(Event::Custom);
}

void TUI::SetError(const std::string &error) {
  state_.current_stage = PipelineStage::Error;
  state_.status_message = error;
  state_.conversation_history.push_back("Error: " + error);
  screen_.PostEvent(Event::Custom);
}

void TUI::AddToHistory(const std::string &message) {
  // Split multi-line messages into separate entries
  std::istringstream stream(message);
  std::string line;
  while (std::getline(stream, line)) {
    if (!line.empty()) {
      state_.conversation_history.push_back(line);
    }
  }
  screen_.PostEvent(Event::Custom);
}

void TUI::SetMode(Mode mode) {
  state_.current_mode = mode;
  state_.conversation_history.push_back("Switched to " + ModeToString(mode) +
                                        " mode");
  if (on_mode_switch_) {
    on_mode_switch_(mode);
  }
  screen_.PostEvent(Event::Custom);
}

void TUI::SetOnSubmit(std::function<void(const std::string &)> callback) {
  on_submit_ = callback;
}

void TUI::SetOnAccept(std::function<void()> callback) { on_accept_ = callback; }

void TUI::SetOnReject(std::function<void()> callback) { on_reject_ = callback; }

void TUI::SetOnModify(std::function<void()> callback) { on_modify_ = callback; }

void TUI::SetOnModeSwitch(std::function<void(Mode)> callback) {
  on_mode_switch_ = callback;
}

Component TUI::CreateLayout() {
  auto terminal_view = CreateTerminalView();
  auto mode_selector = CreateModeSelector();
  auto input_line = CreateInputLine();

  auto main_container =
      Container::Vertical({terminal_view, mode_selector, input_line});

  return Renderer(main_container, [=] {
    return vbox({terminal_view->Render() | flex, separator(),
                 mode_selector->Render(), input_line->Render()});
  });
}

Component TUI::CreateTerminalView() {
  return Renderer([this] {
    // Build conversation history display
    Elements history_elements;

    // Show logo at top - split into lines
    std::string logo = ZWEEK_LOGO;
    std::istringstream logo_stream(logo);
    std::string line;
    while (std::getline(logo_stream, line)) {
      if (!line.empty()) {
        history_elements.push_back(text(line) | color(Color::Cyan));
      }
    }

    history_elements.push_back(text(VERSION + " | " + TAGLINE) |
                               color(Color::GrayLight) | dim);
    history_elements.push_back(text(""));

    // Show conversation history (last 100 messages for full responses)
    int start_idx = std::max(0, (int)state_.conversation_history.size() - 100);
    for (int i = start_idx; i < state_.conversation_history.size(); i++) {
      const auto &msg = state_.conversation_history[i];

      // Color code based on message type
      if (msg.find("Error:") == 0) {
        history_elements.push_back(text(msg) | color(Color::Red));
      } else if (msg.find("[") == 0 && msg.find("]") != std::string::npos) {
        // Status messages
        history_elements.push_back(text(msg) | color(Color::Yellow) | dim);
      } else if (msg.find("Generated code:") == 0) {
        history_elements.push_back(text(msg) | color(Color::Green) | bold);
      } else if (msg.find("Quality:") == 0) {
        history_elements.push_back(text(msg) | color(Color::Cyan));
      } else if (msg.find("Switched to") == 0) {
        history_elements.push_back(text(msg) | color(Color::Magenta));
      } else if (msg.find(">") == 0) {
        // User input
        history_elements.push_back(text(msg) | color(Color::White) | bold);
      } else {
        history_elements.push_back(text(msg));
      }
    }

    // Show current status if processing
    if (state_.current_stage != PipelineStage::Idle &&
        state_.current_stage != PipelineStage::Complete) {
      history_elements.push_back(text(""));
      history_elements.push_back(hbox(
          {text("Working... "), gauge(state_.progress) | flex,
           text(" " + std::to_string((int)(state_.progress * 100)) + "%")}));
    }

    return vbox(history_elements) | frame;
  });
}

Component TUI::CreateModeSelector() {
  return Renderer([this] {
    std::string mode_text = "Mode: ";
    if (state_.current_mode == Mode::Plan) {
      mode_text += "[Plan] Auto  (press 'm' to switch)";
    } else {
      mode_text += "Plan [Auto]  (press 'm' to switch)";
    }

    std::string help_text = "Ctrl+C: Exit";
    if (state_.current_stage == PipelineStage::Complete) {
      help_text = "y: Accept | n: Reject | Ctrl+C: Exit";
    }

    return hbox({text(mode_text) | color(Color::Cyan), separator(),
                 text(help_text) | dim});
  });
}

Component TUI::CreateInputLine() {
  InputOption input_opt;
  input_opt.multiline = false;
  input_opt.placeholder = "Type your request...";
  input_opt.on_enter = [this] {
    if (!state_.user_input.empty()) {
      state_.conversation_history.push_back("> " + state_.user_input);
      if (on_submit_) {
        on_submit_(state_.user_input);
      }
      state_.user_input.clear();
    }
  };

  auto input_box = Input(&state_.user_input, input_opt);

  // Simple keyboard shortcuts (press 'm' to switch modes, 'y' to accept, 'n' to
  // reject)
  auto input_with_hotkeys = CatchEvent(input_box, [this](Event event) {
    // Press 'm' to switch mode
    if (event.is_character() && event.character() == "m") {
      // Only switch if input is empty (so typing 'm' in requests still works)
      if (state_.user_input.empty()) {
        SetMode(state_.current_mode == Mode::Plan ? Mode::Auto : Mode::Plan);
        return true;
      }
    }
    // Press 'y' for accept when complete
    if (event.is_character() && event.character() == "y" &&
        state_.current_stage == PipelineStage::Complete &&
        state_.user_input.empty()) {
      if (on_accept_)
        on_accept_();
      state_.conversation_history.push_back("✓ Changes accepted");
      return true;
    }
    // Press 'n' for reject when complete
    if (event.is_character() && event.character() == "n" &&
        state_.current_stage == PipelineStage::Complete &&
        state_.user_input.empty()) {
      if (on_reject_)
        on_reject_();
      state_.conversation_history.push_back("✗ Changes rejected");
      return true;
    }
    return false;
  });

  return Renderer(input_with_hotkeys, [=] {
    return hbox({text("❯ ") | color(Color::GreenLight) | bold,
                 input_with_hotkeys->Render() | flex});
  });
}

std::string TUI::StageToString(PipelineStage stage) {
  switch (stage) {
  case PipelineStage::Idle:
    return "Idle";
  case PipelineStage::Planning:
    return "Planning";
  case PipelineStage::ToolExecution:
    return "Executing tools";
  case PipelineStage::CodeDrafting:
    return "Generating code";
  case PipelineStage::StyleEnforcing:
    return "Applying style";
  case PipelineStage::ComplexityAuditing:
    return "Analyzing complexity";
  case PipelineStage::GatekeeperReview:
    return "Final review";
  case PipelineStage::Complete:
    return "Complete";
  case PipelineStage::Error:
    return "Error";
  default:
    return "Unknown";
  }
}

std::string TUI::ModeToString(Mode mode) {
  return mode == Mode::Plan ? "Plan" : "Auto";
}

} // namespace ui
} // namespace zweek
