#include <rc/test/rc_test.hpp>

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "solution.hpp"

namespace {

struct Run {
  int status = -1;
  std::string out;
  std::string err;
};

Run run(const char* typed) {
  std::istringstream in(typed);
  std::ostringstream out;
  std::ostringstream err;
  Run r;
  r.status = run_drive(in, out, err);
  r.out = out.str();
  r.err = err.str();
  return r;
}

std::vector<std::string> lines_of(const std::string& text) {
  std::vector<std::string> lines;
  std::istringstream in(text);
  std::string line;
  while (std::getline(in, line)) lines.push_back(line);
  return lines;
}

bool has(const std::string& text, const char* part) { return text.find(part) != std::string::npos; }

}  // namespace

RC_TEST("a quarter circle: the status says it worked, the last line says where it ended") {
  const Run r = run("0.75\n1.25\n1.5707963\n");
  const std::vector<std::string> lines = lines_of(r.out);
  RC_CHECK_EQ(r.status, 0);
  RC_CHECK(r.err.empty());
  RC_REQUIRE(lines.size() == 13);
  std::cout << "  last line: " << lines.back() << "\n\n";
  RC_CHECK_EQ(lines.back(), std::string("end x=1.000 y=1.000 heading=90.0"));
}

RC_TEST("the picture starts at the bottom left and ends at the top right, and a circle is drawn as one") {
  const Run r = run("0.75\n1.25\n1.5707963\n");
  const std::vector<std::string> lines = lines_of(r.out);
  RC_REQUIRE(lines.size() == 13);

  std::cout << "  the picture:\n";
  for (int i = 0; i < 12; ++i) std::cout << "    |" << lines[i] << "\n";
  std::cout << "\n";

  RC_CHECK(has(lines[0], "E"));       // y grows upward and the picture flips it
  RC_CHECK(has(lines[11], "S"));
  RC_CHECK_EQ(lines[11].find('S'), static_cast<std::size_t>(0));

  // Rows 12, columns about twice that: a quarter circle is as wide as it is tall, and a character is
  // about twice as tall as it is wide.
  const std::size_t column = lines[0].find('E');
  RC_CHECK(column >= 20 && column <= 24);

  int stars = 0;
  for (int i = 0; i < 12; ++i) {
    for (char c : lines[i]) {
      if (c == '*') ++stars;
    }
  }
  RC_CHECK(stars >= 10);
}

RC_TEST("a straight drive is a line, not a blank picture") {
  const Run r = run("1\n1\n2\n");
  const std::vector<std::string> lines = lines_of(r.out);
  RC_CHECK_EQ(r.status, 0);
  RC_REQUIRE(lines.size() == 13);
  RC_CHECK_EQ(lines.back(), std::string("end x=2.000 y=0.000 heading=0.0"));

  int rows_with_marks = 0;
  for (int i = 0; i < 12; ++i) {
    if (!lines[i].empty()) ++rows_with_marks;
  }
  std::cout << "  straight drive: " << rows_with_marks << " row of the picture has anything on it\n\n";
  RC_CHECK_EQ(rows_with_marks, 1);
  RC_CHECK(has(r.out, "S"));
  RC_CHECK(has(r.out, "E"));
}

RC_TEST("a bad value stops the run before anything is drawn, with the misused status") {
  const Run r = run("0.75\nfast\n1\n");
  std::cout << "  typed 'fast' for the right speed: status " << r.status << ", stderr: " << r.err;
  std::cout << "  stdout has " << r.out.size() << " characters\n\n";
  RC_CHECK_EQ(r.status, 2);
  RC_CHECK(r.out.empty());
}

RC_TEST("the message says which value and what was wrong with it, and what was allowed") {
  const Run text = run("0.75\nfast\n1\n");
  RC_CHECK(has(text.err, "right speed"));
  RC_CHECK(has(text.err, "not a number"));

  const Run range = run("0.5\n9\n1\n");
  RC_CHECK(has(range.err, "right speed"));
  RC_CHECK(has(range.err, "out of range"));
  RC_CHECK(has(range.err, "allowed 0 to 2"));

  const Run missing = run("0.5\n0.5\n");
  RC_CHECK(has(missing.err, "duration"));
  RC_CHECK(has(missing.err, "no input"));
  RC_CHECK_EQ(missing.status, 2);

  const Run trailing = run("1x\n1\n1\n");
  RC_CHECK(has(trailing.err, "left speed"));
  RC_CHECK_EQ(trailing.status, 2);
}
