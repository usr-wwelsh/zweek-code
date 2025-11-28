#include "pipeline/orchestrator.hpp"
#include "ui/tui.hpp"
#include <chrono>
#include <iostream>
#include <thread>

using namespace zweek::ui;
using namespace zweek::pipeline;

int main(int argc, char **argv) {
  TUI tui;
  Orchestrator orchestrator;

  // Connect orchestrator callbacks to TUI
  orchestrator.SetProgressCallback(
      [&](const std::string &status) { tui.AddToHistory(status); });

  orchestrator.SetResponseCallback([&](const std::string &response) {
    // Only add to history if response is not empty
    if (!response.empty()) {
      tui.AddToHistory(response);
    }
    tui.UpdateStage(PipelineStage::Complete, 1.0f);
  });

  orchestrator.SetStreamCallback([&](const std::string &chunk) {
    tui.AppendToLastMessage(chunk);
  });

  // Set up TUI callbacks
  tui.SetOnSubmit([&](const std::string &request) {
    std::cout << "Processing: " << request << std::endl;

    // Run orchestrator in background thread
    std::thread pipeline_thread([&, request]() {
      tui.UpdateStage(PipelineStage::Planning, 0.1f);
      orchestrator.ProcessRequest(request);
    });
    pipeline_thread.detach();
  });

  tui.SetOnAccept([]() { std::cout << "Changes accepted!" << std::endl; });

  tui.SetOnReject([]() { std::cout << "Changes rejected!" << std::endl; });

  tui.SetOnModify(
      []() { std::cout << "Requesting modifications..." << std::endl; });

  // Run the TUI
  tui.Run();

  return 0;
}
