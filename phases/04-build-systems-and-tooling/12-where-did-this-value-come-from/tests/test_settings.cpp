#include <rc/test/rc_test.hpp>

#include <iostream>
#include <string>
#include <vector>

#include "solution.hpp"

namespace {

std::vector<std::string> known_names() {
  std::vector<std::string> k;
  k.push_back("speed_limit");
  k.push_back("wheel_base");
  k.push_back("port");
  k.push_back("log_level");
  k.push_back("timeout");
  return k;
}

}  // namespace

RC_TEST("the command line beats the environment, which beats the file, which beats the default") {
  Settings s;
  s.set(kDefault, "speed_limit", "1.0");
  std::string v;
  int src = -1;
  RC_REQUIRE(s.get("speed_limit", &v, &src));
  RC_CHECK_EQ(v, std::string("1.0"));
  RC_CHECK_EQ(src, kDefault);

  s.set(kFile, "speed_limit", "1.5");
  s.get("speed_limit", &v, &src);
  RC_CHECK_EQ(v, std::string("1.5"));
  RC_CHECK_EQ(src, kFile);

  s.set(kEnvironment, "speed_limit", "2.0");
  s.get("speed_limit", &v, &src);
  RC_CHECK_EQ(v, std::string("2.0"));
  RC_CHECK_EQ(src, kEnvironment);

  s.set(kCommandLine, "speed_limit", "0.5");
  s.get("speed_limit", &v, &src);
  RC_CHECK_EQ(v, std::string("0.5"));
  RC_CHECK_EQ(src, kCommandLine);
}

RC_TEST("a weaker source never replaces a stronger one, whatever order they arrive in") {
  // The command line arrives first, then the file is read, which must not undo it.
  Settings s;
  s.set(kCommandLine, "port", "9000");
  s.set(kFile, "port", "8080");
  s.set(kDefault, "port", "80");
  std::string v;
  int src = -1;
  RC_REQUIRE(s.get("port", &v, &src));
  std::cout << "  command line 9000, then file 8080, then default 80: port is " << v << " from " << source_name(src) << "\n";
  RC_CHECK_EQ(v, std::string("9000"));
  RC_CHECK_EQ(src, kCommandLine);

  // The same source twice: the later line wins, as in a file that sets a key twice.
  Settings t;
  t.set(kFile, "timeout", "5");
  t.set(kFile, "timeout", "10");
  t.get("timeout", &v, &src);
  RC_CHECK_EQ(v, std::string("10"));

  RC_CHECK(!t.get("absent", &v, &src));
  RC_CHECK(t.get("timeout", &v, nullptr));
}

RC_TEST("a value can say where it came from") {
  Settings s;
  s.set(kDefault, "log_level", "info");
  s.set(kEnvironment, "log_level", "debug");
  s.set(kDefault, "timeout", "30");
  const std::string a = s.explain("log_level");
  const std::string b = s.explain("timeout");
  const std::string c = s.explain("nothing");
  std::cout << "  " << a << "\n  " << b << "\n  " << c << "\n\n";
  RC_CHECK_EQ(a, std::string("log_level = debug (environment)"));
  RC_CHECK_EQ(b, std::string("timeout = 30 (default)"));
  RC_CHECK_EQ(c, std::string("nothing is not set"));
}

RC_TEST("edit distance counts single-character changes") {
  RC_CHECK_EQ(edit_distance("", ""), 0);
  RC_CHECK_EQ(edit_distance("abc", "abc"), 0);
  RC_CHECK_EQ(edit_distance("", "abc"), 3);
  RC_CHECK_EQ(edit_distance("abc", ""), 3);
  RC_CHECK_EQ(edit_distance("kitten", "sitting"), 3);
  RC_CHECK_EQ(edit_distance("speed_limit", "speed_limt"), 1);        // a letter left out
  RC_CHECK_EQ(edit_distance("speed_limit", "sped_limti"), 3);        // one letter out and two swapped, which costs two changes
  RC_CHECK_EQ(edit_distance("wheel_base", "wheelbase"), 1);
  RC_CHECK_EQ(edit_distance("port", "ports"), 1);
  RC_CHECK_EQ(edit_distance("port", "prot"), 2);                      // a swap is two changes, not one
  RC_CHECK_EQ(edit_distance("flaw", "lawn"), 2);
  RC_CHECK_EQ(edit_distance("LOG_LEVEL", "log_level"), 8);             // every letter is the wrong case
}

RC_TEST("a key nobody reads is found, and the nearest known name is offered") {
  Settings s;
  s.set(kFile, "speed_limit", "1.0");        // fine
  s.set(kFile, "speed_limt", "2.0");         // a letter missing
  s.set(kCommandLine, "wheelbase", "0.4");   // an underscore missing
  s.set(kEnvironment, "prot", "9000");       // two letters swapped
  s.set(kFile, "LOG_LEVEL", "debug");        // the wrong case
  s.set(kFile, "colour_scheme", "red");      // nothing like any known name

  const std::vector<std::string> known = known_names();
  const std::vector<std::string> unknown = s.unknown_keys(known);
  std::cout << "  " << unknown.size() << " unknown keys:\n";
  int suggestions = 0;
  for (const std::string& key : unknown) {
    const std::string guess = suggest(key, known);
    std::cout << "    " << key << " -> " << (guess.empty() ? "(no suggestion)" : "did you mean " + guess + "?") << "\n";
    if (!guess.empty()) ++suggestions;
  }
  std::cout << "\n";
  RC_CHECK_EQ(unknown.size(), static_cast<std::size_t>(5));
  RC_CHECK_EQ(suggestions, 3);

  RC_CHECK_EQ(suggest("speed_limt", known), std::string("speed_limit"));
  RC_CHECK_EQ(suggest("wheelbase", known), std::string("wheel_base"));
  RC_CHECK_EQ(suggest("prot", known), std::string("port"));
  RC_CHECK_EQ(suggest("colour_scheme", known), std::string(""));
  RC_CHECK_EQ(suggest("LOG_LEVEL", known), std::string(""));       // eight edits away: case matters here
}

RC_TEST("a short word is not matched to everything, and an exact name is its own suggestion") {
  const std::vector<std::string> known = known_names();
  RC_CHECK_EQ(suggest("x", known), std::string(""));         // one letter: every name is within its length
  RC_CHECK_EQ(suggest("ab", known), std::string(""));
  RC_CHECK_EQ(suggest("port", known), std::string("port"));
  RC_CHECK(Settings().unknown_keys(known).empty());
}
