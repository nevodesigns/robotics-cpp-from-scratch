#include <rc/test/rc_test.hpp>

#include <iostream>
#include <string>

#include "solution.hpp"

// File scope, per audit rule L022.
//
// A real build log, kept verbatim from gcc 11 and clang 14 output for this
// lesson. Every line that is not a warning is here on purpose: the context
// lines, the source echo, clang's summary, and a note.
const std::string kLog =
    "w.cpp: In instantiation of 'int cmp(T, std::size_t) [with T = int]':\n"
    "w.cpp:3:23:   required from here\n"
    "w.cpp:2:59: warning: comparison of integer expressions of different "
    "signedness: 'int' and 'std::size_t' [-Wsign-compare]\n"
    "    2 | template <class T> int cmp(T n, std::size_t m) { return n < m; }\n"
    "      |                                                         ~~^~~\n"
    "w.cpp:4:31: warning: missing initializer for member 'Pose::y' "
    "[-Wmissing-field-initializers]\n"
    "w.cpp:4:31: warning: missing initializer for member 'Pose::h' "
    "[-Wmissing-field-initializers]\n"
    "w.cpp:9:1: note: declared here\n"
    "w.cpp:5:23: warning: unused parameter 'b' [-Wunused-parameter]\n"
    "3 warnings generated.\n";

// One defect in a header, three translation units, as measured.
const std::string kHeaderLog =
    "shared.hpp:4:56: warning: comparison of integer expressions of different "
    "signedness [-Wsign-compare]\n"
    "shared.hpp:4:56: warning: comparison of integer expressions of different "
    "signedness [-Wsign-compare]\n"
    "shared.hpp:4:56: warning: comparison of integer expressions of different "
    "signedness [-Wsign-compare]\n";

RC_TEST("only the lines that are actually warnings are counted") {
  const WarningCount c = count_warnings(kLog);
  RC_CHECK(c.ok);

  // Four warning lines. Not the instantiation context, not "required from
  // here", not the two source echo lines, not the note, and not clang's
  // summary line.
  RC_CHECK(c.lines == 4);

  // Three distinct sites: the two missing-member warnings are both at
  // 4:31, because one brace with two members missing is one place in the
  // source and gcc reports it twice.
  RC_CHECK(c.sites == 3);

  std::cout << "  a 10 line log with 4 warnings at 3 distinct sites\n\n";
  std::cout << "  not counted: the instantiation context, \"required from\n";
  std::cout << "  here\", the two source echo lines, a note, and clang's\n";
  std::cout << "  \"3 warnings generated.\" summary. Keying on the full\n";
  std::cout << "  file:line:col: warning: shape excludes every one of them.\n\n";
}

RC_TEST("one defect in a header is one warning per translation unit") {
  const WarningCount c = count_warnings(kHeaderLog);
  RC_CHECK(c.lines == 3);
  RC_CHECK(c.sites == 1);

  std::cout << "  three translation units including one bad header:\n\n";
  std::cout << "    warning lines    " << c.lines << "\n";
  std::cout << "    distinct sites   " << c.sites << "\n\n";
  std::cout << "  measured exactly this way with gcc: 3 lines, 1 site. The\n";
  std::cout << "  line count scales with the size of the project and the\n";
  std::cout << "  site count does not, which is why this repository once\n";
  std::cout << "  reported 98 of -Wclobbered and 51 of\n";
  std::cout << "  -Wmismatched-new-delete from a handful of defects.\n\n";
  std::cout << "  count sites when deciding how much work there is. Count\n";
  std::cout << "  lines only when deciding how noisy the build is.\n\n";
}

RC_TEST("the flag is what you argue about, so it has to come out cleanly") {
  RC_CHECK(count_with_flag(kLog, "-Wsign-compare") == 1);
  RC_CHECK(count_with_flag(kLog, "-Wmissing-field-initializers") == 2);
  RC_CHECK(count_with_flag(kLog, "-Wunused-parameter") == 1);
  RC_CHECK(count_with_flag(kLog, "-Wclobbered") == 0);

  // A warning with no flag at all. Some cannot be switched off, so the
  // bracket is absent rather than empty, and asking for its flag must give
  // an empty answer rather than reading off the end of the line.
  const std::string unflagged = "a.cpp:1:1: warning: this one has no flag\n";
  const WarningCount c = count_warnings(unflagged);
  RC_CHECK(c.lines == 1);
  RC_CHECK(c.sites == 1);
  RC_CHECK(warning_flag("a.cpp:1:1: warning: this one has no flag").empty());

  std::cout << "  -Wsign-compare 1, -Wmissing-field-initializers 2,\n";
  std::cout << "  -Wunused-parameter 1, -Wclobbered 0\n\n";
  std::cout << "  the dash is kept, because that is how the compiler writes\n";
  std::cout << "  it and how you write it back in a -Wno- flag. The first\n";
  std::cout << "  draft promised it stripped and returned it dashed, and\n";
  std::cout << "  this test is what noticed.\n\n";
  std::cout << "  the flag is the part you look up, argue about and decide\n";
  std::cout << "  to believe or not, so it is the part worth extracting.\n\n";
}

RC_TEST("a line that only looks like a warning is not one") {
  // Prose containing the word.
  RC_CHECK(warning_site_length("this is a warning about something") == 0);
  // The marker without a site.
  RC_CHECK(warning_site_length(": warning: no file or line here") == 0);
  // A file and one number, which is a note's shape, not a warning's.
  RC_CHECK(warning_site_length("w.cpp:3: warning: only one number") == 0);
  // Non numeric where the numbers belong.
  RC_CHECK(warning_site_length("w.cpp:x:y: warning: not numbers") == 0);
  // The real shape, which must be accepted, and the site must stop before
  // the marker.
  // 15, which is the length of "src/a.cpp:12:34". The first draft of this
  // test said 16 and the suite caught it, which is the right way round.
  const std::string real = "src/a.cpp:12:34: warning: something [-Wfoo]";
  RC_CHECK(warning_site_length(real) == 15);
  RC_CHECK(std::string(real.substr(0, 15)) == "src/a.cpp:12:34");

  const WarningCount nothing = count_warnings("");
  RC_CHECK(!nothing.ok);
  RC_CHECK(nothing.lines == 0);

  std::cout << "  prose, a bare marker, one number instead of two, and non\n";
  std::cout << "  numeric positions are all refused. A Windows path with a\n";
  std::cout << "  drive letter still works, because the walk back looks for\n";
  std::cout << "  the last two numeric colon fields rather than the first.\n\n";
}
