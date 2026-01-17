#pragma once

namespace zweek {
namespace grammars {

// Router classification: CODE | CHAT | TOOL
constexpr const char *ROUTER_GRAMMAR =
    "root ::= ws intent ws\n"
    "intent ::= \"CODE\" | \"CHAT\" | \"TOOL\"\n"
    "ws ::= [ \\t\\n]*";

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

// Recursive Agent grammar: Forces THOUGHT + CMD structure
// This is the "prosthetic" that constrains the model to structured reasoning
// The model MUST output exactly this format - no freeform text allowed
//
// Commands available:
//   READ_LINES <path> <start>-<end>   - Read specific lines (max 50)
//   GREP <pattern> <path>             - Search for pattern
//   LIST <path>                       - Directory listing
//   FILE_INFO <path>                  - Get metadata (no content)
//   CREATE <path>                     - Create empty file
//   WRITE <path> <start>-<end>        - Replace lines
//   INSERT <path> <after_line>        - Insert after line
//   DELETE_LINES <path> <start>-<end> - Delete lines
//   FINISH <summary>                  - Task complete
//
constexpr const char *AGENT_GRAMMAR = R"(
root ::= thought command

thought ::= "THOUGHT: " thought-text "\n"
thought-text ::= [^\n]+

command ::= "CMD: " cmd-body

cmd-body ::= read-cmd | grep-cmd | list-cmd | file-info-cmd | create-cmd | write-cmd | insert-cmd | delete-cmd | finish-cmd

read-cmd ::= "READ_LINES " path " " line-range "\n"
grep-cmd ::= "GREP " pattern " " path "\n"
list-cmd ::= "LIST " path "\n"
file-info-cmd ::= "FILE_INFO " path "\n"
create-cmd ::= "CREATE " path "\n"
write-cmd ::= "WRITE " path " " line-range "\n" content-block "END_WRITE\n"
insert-cmd ::= "INSERT " path " " number "\n" content-block "END_INSERT\n"
delete-cmd ::= "DELETE_LINES " path " " line-range "\n"
finish-cmd ::= "FINISH " [^\n]+ "\n"

line-range ::= number "-" number
number ::= [0-9]+
path ::= [a-zA-Z0-9_./-]+
pattern ::= "\"" [^\"]* "\"" | [a-zA-Z0-9_.*?|\\[\\]^$]+
content-block ::= content-line*
content-line ::= [^\n]* "\n"
)";

} // namespace grammars
} // namespace zweek
