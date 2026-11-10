#include <rc/test/rc_test.hpp>

#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

#include <rc/core/random.hpp>

#include "solution.hpp"

namespace {

std::vector<std::string> lines(const char* a, const char* b = nullptr, const char* c = nullptr, const char* d = nullptr,
                               const char* e = nullptr) {
  std::vector<std::string> v;
  for (const char* p : {a, b, c, d, e}) {
    if (p != nullptr) v.push_back(p);
  }
  return v;
}

// The log of a short run, as the robot writes it: a time and an address change every run.
std::vector<std::string> run_log(double start_time, unsigned address, double final_speed) {
  std::vector<std::string> v;
  v.push_back("t=" + std::to_string(start_time) + " motor enabled at 0x" + std::to_string(address));
  v.push_back("t=" + std::to_string(start_time + 0.5) + " speed 0.500000");
  v.push_back("t=" + std::to_string(start_time + 1.0) + " speed " + std::to_string(final_speed));
  v.push_back("t=" + std::to_string(start_time + 1.5) + " motor disabled");
  return v;
}

std::vector<std::string> normalized(const std::vector<std::string>& v) {
  std::vector<std::string> out;
  for (const std::string& line : v) out.push_back(normalize(line));
  return out;
}

// Take a diff and apply it to `expected`: the result should be `actual`.
std::vector<std::string> apply(const std::vector<DiffLine>& d) {
  std::vector<std::string> out;
  for (const DiffLine& line : d) {
    if (line.op != '-') out.push_back(line.text);
  }
  return out;
}

std::vector<std::string> expected_side(const std::vector<DiffLine>& d) {
  std::vector<std::string> out;
  for (const DiffLine& line : d) {
    if (line.op != '+') out.push_back(line.text);
  }
  return out;
}

}  // namespace

RC_TEST("the time and an address in a log line are replaced and nothing else is") {
  RC_CHECK_EQ(normalize("t=12.345 motor enabled"), std::string("t=<time> motor enabled"));
  RC_CHECK_EQ(normalize("t=0 start"), std::string("t=<time> start"));
  RC_CHECK_EQ(normalize("buffer at 0x7ffd5a3c ok"), std::string("buffer at 0x<addr> ok"));
  RC_CHECK_EQ(normalize("t=1.5 copy 0xDEADbeef to 0x10"), std::string("t=<time> copy 0x<addr> to 0x<addr>"));
  RC_CHECK_EQ(normalize("speed 0.5 at limit 1.0"), std::string("speed 0.5 at limit 1.0"));          // numbers that matter stay
  RC_CHECK_EQ(normalize("target=3 t= none"), std::string("target=3 t= none"));                         // t= with no number is not a time
  RC_CHECK_EQ(normalize("0x is not an address"), std::string("0x is not an address"));
  RC_CHECK_EQ(normalize("soft=2.5"), std::string("soft=2.5"));                                         // t= inside a word is not a time
  RC_CHECK_EQ(normalize(""), std::string(""));
}

RC_TEST("a difference keeps what is common, removes what went and adds what came") {
  const std::vector<DiffLine> same = diff(lines("a", "b", "c"), lines("a", "b", "c"));
  RC_CHECK_EQ(same.size(), static_cast<std::size_t>(3));
  RC_CHECK_EQ(changes(same), 0);

  const std::vector<DiffLine> added = diff(lines("a", "c"), lines("a", "b", "c"));
  RC_CHECK_EQ(changes(added), 1);
  RC_REQUIRE(added.size() == 3);
  RC_CHECK_EQ(added[1].op, '+');
  RC_CHECK_EQ(added[1].text, std::string("b"));

  const std::vector<DiffLine> removed = diff(lines("a", "b", "c"), lines("a", "c"));
  RC_CHECK_EQ(changes(removed), 1);
  RC_REQUIRE(removed.size() == 3);
  RC_CHECK_EQ(removed[1].op, '-');

  // A line that changed is one removed and one added.
  const std::vector<DiffLine> replaced = diff(lines("a", "b", "c"), lines("a", "B", "c"));
  RC_CHECK_EQ(changes(replaced), 2);

  RC_CHECK_EQ(changes(diff(lines("a"), std::vector<std::string>())), 1);
  RC_CHECK_EQ(changes(diff(std::vector<std::string>(), lines("a", "b"))), 2);
  RC_CHECK(diff(std::vector<std::string>(), std::vector<std::string>()).empty());
}

RC_TEST("the difference of random line lists rebuilds both sides and is never longer than removing everything and adding everything") {
  rc::core::Rng rng(2026);
  int checked = 0;
  for (int round = 0; round < 500; ++round) {
    std::vector<std::string> a;
    std::vector<std::string> b;
    const int na = static_cast<int>(rng.below(12));
    const int nb = static_cast<int>(rng.below(12));
    for (int i = 0; i < na; ++i) a.push_back(std::string(1, static_cast<char>('a' + rng.below(5))));
    for (int i = 0; i < nb; ++i) b.push_back(std::string(1, static_cast<char>('a' + rng.below(5))));
    const std::vector<DiffLine> d = diff(a, b);
    if (expected_side(d) != a) continue;
    if (apply(d) != b) continue;
    if (changes(d) > na + nb) continue;
    ++checked;
  }
  std::cout << "  " << checked << " of 500 random pairs of line lists: the difference rebuilt both sides and was no longer than the worst case\n\n";
  RC_CHECK_EQ(checked, 500);
}

RC_TEST("two runs of the same program differ in every line until the log is normalised, and not after") {
  const std::vector<std::string> first = run_log(1000.25, 4095, 1.0);
  const std::vector<std::string> second = run_log(2000.75, 8191, 1.0);

  int raw_different = 0;
  int normalized_different = 0;
  for (std::size_t i = 0; i < first.size(); ++i) {
    if (first[i] != second[i]) ++raw_different;
    if (normalize(first[i]) != normalize(second[i])) ++normalized_different;
  }
  std::cout << "  two runs, " << first.size() << " lines each: " << raw_different << " lines differ as written, " << normalized_different
            << " after normalising\n";
  RC_CHECK_EQ(raw_different, 4);
  RC_CHECK_EQ(normalized_different, 0);
  RC_CHECK_EQ(normalize(first[0]), std::string("t=<time> motor enabled at 0x<addr>"));       // what is left is still the line
  RC_CHECK_EQ(changes(diff(normalized(first), normalized(second))), 0);
  RC_CHECK_EQ(diff(normalized(first), normalized(second)).size(), first.size());
}

RC_TEST("a real change is found after normalising, and the difference says which line") {
  const std::vector<std::string> saved = normalized(run_log(1000.25, 4095, 1.0));
  const std::vector<std::string> now = normalized(run_log(5000.5, 12287, 1.25));       // the final speed changed
  const std::vector<DiffLine> d = diff(saved, now);

  std::cout << "  the saved log against this run:\n";
  for (const DiffLine& line : d) std::cout << "    " << line.op << " " << line.text << "\n";
  std::cout << "\n";
  RC_CHECK_EQ(changes(d), 2);
  int removed = 0;
  int added = 0;
  for (const DiffLine& line : d) {
    if (line.op == '-' && line.text.find("1.000000") != std::string::npos) ++removed;
    if (line.op == '+' && line.text.find("1.250000") != std::string::npos) ++added;
  }
  RC_CHECK_EQ(removed, 1);
  RC_CHECK_EQ(added, 1);
}
