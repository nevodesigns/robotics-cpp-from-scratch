// rc/core/backtrace.hpp
//
// The crash triage from lesson 04-02, graduated.
//
// A backtrace is a wall of text with two useful lines in it, and finding them
// is mechanical: skip the frames belonging to the runtime and the sanitizer,
// and stop at the first one that is your own. The kind of crash and the first
// frame of your code are what a report needs, and everything else is noise
// somebody will paste anyway.

#ifndef RC_CORE_BACKTRACE_HPP
#define RC_CORE_BACKTRACE_HPP

#include <cctype>
#include <cstddef>
#include <sstream>
#include <string>
#include <vector>
#include <rc/core/compat.hpp>

namespace rc {
namespace core {

struct Frame {
  int index = 0;
  std::string function;
  std::string file;
  int line = 0;
};

enum class CrashKind {
  StackBufferOverflow,
  HeapBufferOverflow,
  UseAfterFree,
  MemoryLeak,
  SegmentationFault,
  ArithmeticFault,
  Unknown,
};

enum class TriageError {
  NoFrames,
  NoOwnFrame,
};

inline std::string trim_spaces(const std::string& text) {
  const std::size_t first = text.find_first_not_of(" \t\r\n");
  if (first == std::string::npos) return "";
  const std::size_t last = text.find_last_not_of(" \t\r\n");
  return text.substr(first, last - first + 1);
}

// Splits a trailing "path:line" into its parts. Returns false when the text does
// not end in a colon and digits, which is how frames with no source information
// are recognised.
inline bool split_location(const std::string& text, std::string& file, int& line) {
  const std::size_t colon = text.rfind(':');
  if (colon == std::string::npos || colon + 1 >= text.size()) return false;

  for (std::size_t i = colon + 1; i < text.size(); ++i) {
    if (std::isdigit(static_cast<unsigned char>(text[i])) == 0) return false;
  }
  file = text.substr(0, colon);
  line = std::stoi(text.substr(colon + 1));
  return true;
}

inline std::vector<Frame> parse_frames(const std::string& report) {
  std::vector<Frame> frames;
  std::istringstream lines(report);
  std::string line;

  while (std::getline(lines, line)) {
    const std::string content = trim_spaces(line);
    if (content.size() < 2 || content[0] != '#') continue;
    if (std::isdigit(static_cast<unsigned char>(content[1])) == 0) continue;

    const std::size_t in_at = content.find(" in ");
    if (in_at == std::string::npos) continue;

    Frame frame;
    frame.index = std::stoi(content.substr(1));

    // A debugger writes " at file:line" at the end. A sanitizer just puts the
    // location last. Preferring " at " when present, and otherwise taking the
    // final token, covers both with no format flag.
    const std::string after_in = content.substr(in_at + 4);
    const std::size_t at_marker = after_in.rfind(" at ");

    std::string location;
    if (at_marker != std::string::npos) {
      frame.function = trim_spaces(after_in.substr(0, at_marker));
      location = trim_spaces(after_in.substr(at_marker + 4));
    } else {
      const std::size_t last_space = after_in.find_last_of(' ');
      if (last_space == std::string::npos) {
        frame.function = trim_spaces(after_in);
      } else {
        frame.function = trim_spaces(after_in.substr(0, last_space));
        location = trim_spaces(after_in.substr(last_space + 1));
      }
    }

    // A frame with no usable location keeps its function name. Dropping it
    // would renumber everything after it and make the report harder to follow.
    if (!split_location(location, frame.file, frame.line)) {
      frame.file.clear();
      frame.line = 0;
    }

    frames.push_back(frame);
  }
  return frames;
}

inline bool starts_with_text(const std::string& text, const std::string& prefix) {
  return text.size() >= prefix.size() && text.compare(0, prefix.size(), prefix) == 0;
}

inline bool is_system_frame(const Frame& frame) {
  // No file means the frame came from a library with no debug information, so
  // there is nothing of yours to look at there either.
  if (frame.file.empty()) return true;

  return starts_with_text(frame.file, "/usr/") ||
         starts_with_text(frame.file, "/lib") ||
         starts_with_text(frame.file, "../sysdeps/") ||
         starts_with_text(frame.file, "../csu/");
}

inline rc::expected<Frame, TriageError> first_own_frame(const std::vector<Frame>& frames) {
  if (frames.empty()) return rc::unexpected(TriageError::NoFrames);

  // Walking from frame zero outward is the rule: the innermost frame in code
  // somebody wrote is where the defect almost always is.
  for (const Frame& frame : frames) {
    if (!is_system_frame(frame)) return frame;
  }
  return rc::unexpected(TriageError::NoOwnFrame);
}

inline bool report_mentions(const std::string& report, const std::string& phrase) {
  return report.find(phrase) != std::string::npos;
}

inline CrashKind classify(const std::string& report) {
  // Ordered from most specific to least. Checking use after free before the
  // overflow kinds matters because a report can mention several addresses.
  if (report_mentions(report, "heap-use-after-free")) return CrashKind::UseAfterFree;
  if (report_mentions(report, "stack-buffer-overflow")) return CrashKind::StackBufferOverflow;
  if (report_mentions(report, "heap-buffer-overflow")) return CrashKind::HeapBufferOverflow;
  if (report_mentions(report, "LeakSanitizer") || report_mentions(report, "detected memory leaks"))
    return CrashKind::MemoryLeak;
  if (report_mentions(report, "SIGSEGV") || report_mentions(report, "Segmentation fault"))
    return CrashKind::SegmentationFault;
  if (report_mentions(report, "SIGFPE") || report_mentions(report, "Arithmetic exception"))
    return CrashKind::ArithmeticFault;
  return CrashKind::Unknown;
}

}  // namespace core
}  // namespace rc

#endif  // RC_CORE_BACKTRACE_HPP
