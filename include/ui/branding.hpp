#pragma once

#include <string>

namespace zweek {
namespace ui {

// ASCII art branding for Zweek Code (pure ASCII, Windows-compatible)
const std::string ZWEEK_LOGO = R"(
 ______                 _      _____          _      
|___  /                | |    / ____|        | |     
   / /_      _____  ___| | __| |     ___   __| | ___ 
  / /\ \ /\ / / _ \/ _ \ |/ /| |    / _ \ / _` |/ _ \
 / /__\ V  V /  __/  __/   < | |___| (_) | (_| |  __/
/_____|\_/\_/ \___|\___|_|\_\ \_____\___/ \__,_|\___|
)";

const std::string ZWEEK_SMALL_LOGO = R"(
 Zweek Code
)";

const std::string VERSION = "v1.0.0-alpha";
const std::string TAGLINE = "Local AI - Fully Offline - Privacy First";

} // namespace ui
} // namespace zweek
