// tools/rcpp/commands.hpp
// Every subcommand has the same shape: it receives its own arguments and
// returns the process exit code.

#ifndef RCPP_COMMANDS_HPP
#define RCPP_COMMANDS_HPP

#include <string>
#include <vector>

namespace rcpp {

using Args = std::vector<std::string>;

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

bool has_flag(const Args& args, const std::string& flag);
std::string flag_value(const Args& args, const std::string& flag, const std::string& fallback);

}  // namespace rcpp

#endif  // RCPP_COMMANDS_HPP
