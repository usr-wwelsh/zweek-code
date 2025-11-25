#pragma once

namespace zweek {
namespace grammars {

// Router classification: CODE | CHAT | TOOL
constexpr const char *ROUTER_GRAMMAR = R"(
root ::= ws intent ws
intent ::= "CODE" | "CHAT" | "TOOL"
ws ::= [ \t\n]*
)";

// Planner tool calls: JSON array
constexpr const char *PLANNER_GRAMMAR = R"(
root ::= ws "[" ws tools ws "]" ws
tools ::= tool (ws "," ws tool)*
tool ::= "{" ws 
         "\"type\":" ws "\"" tool_type "\"" ws "," ws 
         "\"path\":" ws "\"" path "\"" ws
         "}"
tool_type ::= "read_file" | "write_file" | "search" | "git_diff"
path ::= [a-zA-Z0-9/._-]+
ws ::= [ \t\n]*
)";

} // namespace grammars
} // namespace zweek
