// rc/core/args.hpp
//
// Reading a command line without lying about what was on it, from lesson
// 01-13, graduated.
//
// Every program before this one had its inputs written into it. A tool is
// told them when it is run, through the two parameters of main that nobody
// explains: argc is how many strings there are, argv points at them, argv[0]
// is conventionally the program's own name, and the arguments a user actually
// typed start at argv[1].
//
// Two facts about argv worth having before touching it.
//
// argv[argc] is guaranteed to be a null pointer, so walking until null and
// counting to argc are both valid and in a well formed command line they
// agree. This reader does both and stops at whichever comes first, which
// costs nothing and matters when they disagree.
//
// A missing argument is therefore not out of bounds. Read argv[1] when the
// user typed nothing and you get that null, which is worse than a wild
// pointer because it looks like a value. Handing it to std::string is
// undefined behaviour; measured on this curriculum's libstdc++ it throws a
// std::logic_error reading "basic_string::_M_construct null not valid" and
// aborts, which is the lucky outcome, and the standard promises nothing.
//
// The design decision in this header is that a lookup has three outcomes and
// not two. A function returning a value with a fallback for "not found"
// collapses three different situations into one answer: the option was
// absent, the option was there with nothing after it, and the option was
// there with an empty value. The first is a user who did not ask. The others
// are users who asked and got it wrong, and they deserve to be told.
//
// Zero is what a collapsed answer usually becomes, and zero is a speed. It is
// also a gain, an offset and a timeout, which is how a tool ends up doing
// nothing at all and reporting success.
//
// Measured in the lesson, on the same five strings a user can type into a
// numeric option:
//
//   input      atoi   parse_number
//   42         42     accepted
//   abc        0      refused
//   (empty)    0      refused
//   3.9        3      accepted
//   7abc       7      refused
//
// Two of the five are numbers and parse_number accepts exactly those two.
// atoi gets three wrong in three different ways: 0 for two things that are
// not numbers at all, indistinguishable from a user who typed 0, and 3 for
// "3.9", a real number read as a different one because atoi only ever reads
// an integer. On a value too large for an int, atoi is undefined rather than
// merely wrong: measured standalone, atoi("99999999999999999999") returned
// -1, while strtol returned LONG_MAX and set ERANGE. A mistyped speed limit
// became a negative speed.
//
// So number() below goes through rc::core::parse_number from lesson 01-12,
// which refuses a field that merely starts with a number, and keeps the three
// command line failures apart from each other.
//
// Neither Option nor Number converts to bool or to double. That is
// deliberate, it is what checks/ in the lesson proves, and it is the only
// thing stopping the call site from collapsing the three answers again.

#ifndef RC_CORE_ARGS_HPP
#define RC_CORE_ARGS_HPP

#include <cstddef>
#include <string_view>
#include <vector>

#include <rc/core/parse.hpp>

namespace rc {
namespace core {

// What a lookup found. Three outcomes, not two, and that is the whole design.
//
// A function that returns a value with a fallback for "not found" collapses
// three different situations into one answer: the option was absent, the
// option was there with nothing after it, and the option was there with an
// empty value. The first is a user who did not ask. The second and third are
// users who asked and got it wrong, and they deserve to be told rather than
// quietly given the default.
//
// This repository's own tool does the collapsing version in
// tools/rcpp/main.cpp, which is how this lesson came to be written.
struct Option {
  bool present = false;    // the name appeared on the command line
  bool has_value = false;  // and something followed it
  std::string_view value;  // meaningful only when has_value is true
};

// A command line, read once and then asked questions.
class Arguments {
 public:
  // Takes what main was given. Stops at a null entry as well as at argc, so a
  // short argv or a hostile argc cannot walk off the end.
  Arguments(int argc, const char* const* argv) {
    if (argv == nullptr || argc <= 0) return;
    program_ = argv[0] != nullptr ? std::string_view(argv[0]) : std::string_view();
    for (int i = 1; i < argc; ++i) {
      if (argv[i] == nullptr) break;
      args_.push_back(std::string_view(argv[i]));
    }
  }

  std::string_view program() const { return program_; }

  // Everything after the program name, options and positionals alike.
  std::size_t size() const { return args_.size(); }

  // Out of range gives an empty view rather than undefined behaviour, which
  // is the one place this type is deliberately forgiving: a tool reading its
  // own arguments should not be able to crash on a short command line.
  std::string_view operator[](std::size_t i) const {
    return i < args_.size() ? args_[i] : std::string_view();
  }

  bool has(std::string_view name) const { return find(name) < args_.size(); }

  // Supports both --name value and --name=value, because users type both and
  // being strict about it teaches them nothing.
  //
  // A repeated option takes the last one, which is the convention that makes
  // a wrapper script possible: the script supplies defaults and whatever the
  // user adds afterwards wins.
  //
  // In the separate form, whatever follows is taken as the value even if it
  // looks like another option, which is what getopt does and therefore what
  // users expect. The cost is that --speed --verbose makes "--verbose" the
  // speed. That is not silently wrong, because number() below then reports
  // that it will not parse, and the suite measures exactly this case. For a
  // string option there is no such safety net, which is the argument for
  // writing --name=value in scripts.
  Option option(std::string_view name) const {
    Option found;
    for (std::size_t i = 0; i < args_.size(); ++i) {
      if (args_[i] == "--") break;
      if (args_[i] == name) {
        found.present = true;
        const bool next_is_value = i + 1 < args_.size() && args_[i + 1] != "--";
        found.has_value = next_is_value;
        found.value = next_is_value ? args_[i + 1] : std::string_view();
      } else if (args_[i].size() > name.size() && args_[i].compare(0, name.size(), name) == 0 &&
                 args_[i][name.size()] == '=') {
        found.present = true;
        found.has_value = true;
        found.value = args_[i].substr(name.size() + 1);
      }
    }
    return found;
  }

  // A number, or a refusal, with the three failure modes kept apart.
  //
  // parse_number from lesson 01-12 does the hard part: it refuses a field
  // that merely starts with a number, so --speed 1.5abc and --speed 1,5 are
  // rejected rather than read as 1.5 and 1. This adds the two cases that only
  // a command line has: the option was never given, and it was given with
  // nothing after it.
  struct Number {
    bool present = false;
    bool ok = false;        // present, had a value, and the value parsed
    double value = 0.0;
  };

  Number number(std::string_view name) const {
    const Option found = option(name);
    Number result;
    result.present = found.present;
    if (!found.present || !found.has_value) return result;
    const rc::core::ParsedNumber parsed = rc::core::parse_number(found.value);
    result.ok = parsed.ok;
    result.value = parsed.ok ? parsed.value : 0.0;
    return result;
  }

  // Arguments that are not options: filenames, mostly. Everything after a
  // bare -- is positional even if it starts with a dash, which is how you
  // pass a file called --help to a program that has a --help.
  std::vector<std::string_view> positionals() const {
    std::vector<std::string_view> rest;
    bool only_positional = false;
    for (std::size_t i = 0; i < args_.size(); ++i) {
      if (!only_positional && args_[i] == "--") {
        only_positional = true;
        continue;
      }
      if (only_positional) {
        rest.push_back(args_[i]);
        continue;
      }
      if (!args_[i].empty() && args_[i][0] == '-') {
        // An option. Skip its value too, if it took one in the separate form.
        const bool takes_next = args_[i].find('=') == std::string_view::npos &&
                                i + 1 < args_.size() && !args_[i + 1].empty() &&
                                args_[i + 1][0] != '-';
        if (takes_next) ++i;
        continue;
      }
      rest.push_back(args_[i]);
    }
    return rest;
  }

 private:
  std::size_t find(std::string_view name) const {
    for (std::size_t i = 0; i < args_.size(); ++i) {
      if (args_[i] == "--") break;
      if (args_[i] == name) return i;
      if (args_[i].size() > name.size() && args_[i].compare(0, name.size(), name) == 0 &&
          args_[i][name.size()] == '=')
        return i;
    }
    return args_.size();
  }

  std::string_view program_;
  std::vector<std::string_view> args_;
};

}  // namespace core
}  // namespace rc

#endif  // RC_CORE_ARGS_HPP
