#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cctype>
#include <sstream>
#include <string>
#include <vector>

#include <rc/core/compat.hpp>

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
  // TODO
  //
  // A frame line, once trimmed, starts with # and a digit. The function name
  // follows " in ". A debugger then writes " at file:line"; a sanitizer just
  // puts the location last. Preferring " at " when it is present and otherwise
  // taking the final token handles both without a format flag.
  //
  // Keep a frame that has no usable location, with an empty file and line zero.
  // Dropping it would renumber everything after it.
  (void)report;
  return {};
}

inline bool starts_with_text(const std::string& text, const std::string& prefix) {
  return text.size() >= prefix.size() && text.compare(0, prefix.size(), prefix) == 0;
}

inline bool is_system_frame(const Frame& frame) {
  // TODO: true for a frame with no file, or a file under /usr/ or /lib, or a
  // relative path into the C library such as ../sysdeps/ or ../csu/.
  (void)frame;
  return false;
}

inline rc::expected<Frame, TriageError> first_own_frame(const std::vector<Frame>& frames) {
  // TODO: walk from frame zero outward and return the first frame that is not a
  // system frame. Report NoFrames for an empty backtrace and NoOwnFrame when
  // every frame belongs to somebody else.
  (void)frames;
  return rc::unexpected(TriageError::NoFrames);
}

inline bool report_mentions(const std::string& report, const std::string& phrase) {
  return report.find(phrase) != std::string::npos;
}

inline CrashKind classify(const std::string& report) {
  // TODO: read the kind off the first line. Check use after free before the
  // overflow kinds, because a report can mention more than one address.
  (void)report;
  return CrashKind::Unknown;
}

#endif  // LESSON_SOLUTION_HPP
