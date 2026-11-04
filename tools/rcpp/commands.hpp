// tools/rcpp/commands.hpp
// Every subcommand has the same shape: it receives its own arguments and
// returns the process exit code.

#ifndef RCPP_COMMANDS_HPP
#define RCPP_COMMANDS_HPP

#include <string>
#include <vector>

#include <rc/core/args.hpp>

namespace rcpp {

// The arguments after the subcommand name, read through the header lesson
// 01-13 graduated. program() is therefore the subcommand itself.
//
// This used to be a std::vector<std::string> with two helpers over it, and
// those helpers had the defect E-CPP-0056 describes. Measured before the
// change, on a tool that knows 102 lessons:
//
//   rcpp list --phase 01     13 lessons, correct
//   rcpp list --phase       102 lessons, silently
//   rcpp list --phse 01     102 lessons, silently
//
// The user asked to narrow and got everything, twice, with nothing on stderr.
using Args = rc::core::Arguments;

int cmd_doctor(const Args& args);
int cmd_audit(const Args& args);
int cmd_verify(const Args& args);
int cmd_explain(const Args& args);
int cmd_catalog(const Args& args);
int cmd_list(const Args& args);
int cmd_start(const Args& args);
int cmd_readme(const Args& args);
int cmd_next(const Args& args);
int cmd_status(const Args& args);
int cmd_platforms(const Args& args);
int cmd_targets(const Args& args);

// Reads a string option while keeping its three outcomes apart.
//
// Absent leaves `into` as the caller set it, which is the default and the
// right answer for somebody who did not ask. Present with a value replaces
// it. Present with nothing after it is somebody who asked and got it wrong,
// so this says so on stderr and returns false, and the caller exits non zero
// instead of quietly doing something else.
bool read_option(const Args& args, const char* name, std::string& into, const char* command);

// The same for a count, which must also be a whole number of at least one.
// std::atoi cannot report any of the ways this fails: measured in lesson
// 01-13 it returns 0 for "abc" and for the empty string, 3 for "3.9", and is
// undefined for a value too large for an int.
bool read_count(const Args& args, const char* name, int& into, const char* command);

}  // namespace rcpp

#endif  // RCPP_COMMANDS_HPP
