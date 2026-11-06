#include <rc/test/rc_test.hpp>

#include <cstdio>
#include <iomanip>
#include <iostream>
#include <set>
#include <string>
#include <vector>

#include "solution.hpp"

RC_TEST("the same text always has the same name, and the empty text has the starting number") {
  RC_CHECK(content_id("hello") == content_id("hello"));
  RC_CHECK(content_id("hello") != content_id("hellp"));
  RC_CHECK(content_id("") == 14695981039346656037ULL);
  // The published value of FNV-1a 64 for "a", so the arithmetic is the real one.
  RC_CHECK(content_id("a") == 0xaf63dc4c8601ec8cULL);
}

RC_TEST("counting the differing bits") {
  RC_CHECK_EQ(differing_bits(0, 0), 0);
  RC_CHECK_EQ(differing_bits(0, 1), 1);
  RC_CHECK_EQ(differing_bits(0xFFULL, 0x0FULL), 4);
  RC_CHECK_EQ(differing_bits(~0ULL, 0), 64);
  RC_CHECK_EQ(differing_bits(5, 5), 0);
}

RC_TEST("changing one character changes about half of the 64 bits, wherever it is") {
  std::string text(200, 'x');
  for (std::size_t i = 0; i < text.size(); ++i) text[i] = static_cast<char>('a' + i % 26);

  const Id base = content_id(text.c_str());
  double total = 0.0;
  int worst_low = 64;
  int worst_high = 0;
  for (std::size_t i = 0; i < text.size(); ++i) {
    std::string changed = text;
    changed[i] = changed[i] == 'z' ? 'a' : static_cast<char>(changed[i] + 1);
    const int bits = differing_bits(base, content_id(changed.c_str()));
    total += bits;
    if (bits < worst_low) worst_low = bits;
    if (bits > worst_high) worst_high = bits;
  }
  const double mean = total / static_cast<double>(text.size());
  std::cout << "  one character changed in a 200 character text, all 200 places: mean " << std::fixed
            << std::setprecision(1) << mean << " of 64 bits differ, between " << worst_low << " and " << worst_high
            << "\n\n";
  RC_CHECK(mean > 28.0);
  RC_CHECK(mean < 36.0);
  RC_CHECK(worst_low >= 10);
}

RC_TEST("a snapshot has the same name however its files are listed, and a different one if anything changes") {
  const File a[] = {{"main.cpp", "int main() {}"}, {"README", "a robot"}, {"CMakeLists.txt", "project(r)"}};
  const File b[] = {{"CMakeLists.txt", "project(r)"}, {"main.cpp", "int main() {}"}, {"README", "a robot"}};
  const File edited[] = {{"main.cpp", "int main() { }"}, {"README", "a robot"}, {"CMakeLists.txt", "project(r)"}};
  const File renamed[] = {{"app.cpp", "int main() {}"}, {"README", "a robot"}, {"CMakeLists.txt", "project(r)"}};
  const File fewer[] = {{"main.cpp", "int main() {}"}, {"README", "a robot"}};

  RC_CHECK(snapshot_id(a, 3) == snapshot_id(b, 3));
  RC_CHECK(snapshot_id(a, 3) != snapshot_id(edited, 3));
  RC_CHECK(snapshot_id(a, 3) != snapshot_id(renamed, 3));
  RC_CHECK(snapshot_id(a, 3) != snapshot_id(fewer, 2));
  RC_CHECK(snapshot_id(fewer, 0) != snapshot_id(a, 3));

  // The content of a file does not depend on its name, and its place in the snapshot does.
  RC_CHECK(content_id(a[0].text) == content_id(renamed[0].text));
  RC_CHECK(file_id(a[0]) != file_id(renamed[0]));
}

RC_TEST("two versions that share most files share most names, so only the change needs keeping") {
  std::vector<std::string> names;
  std::vector<std::string> texts;
  for (int i = 0; i < 100; ++i) {
    names.push_back("file" + std::to_string(i) + ".cpp");
    texts.push_back("// contents of file number " + std::to_string(i));
  }

  std::vector<File> v1;
  std::vector<File> v2;
  for (int i = 0; i < 100; ++i) v1.push_back(File{names[i].c_str(), texts[i].c_str()});
  v2 = v1;
  const std::string changed = "// contents of file number 41, edited";
  v2[41].text = changed.c_str();

  std::set<Id> distinct;
  for (const File& f : v1) distinct.insert(content_id(f.text));
  for (const File& f : v2) distinct.insert(content_id(f.text));

  std::cout << "  two versions of a hundred files, one file edited: " << v1.size() + v2.size() << " files listed, "
            << distinct.size() << " different contents to store\n\n";
  RC_CHECK_EQ(distinct.size(), static_cast<std::size_t>(101));
  RC_CHECK(snapshot_id(v1.data(), 100) != snapshot_id(v2.data(), 100));
}

RC_TEST("a thousand different texts get a thousand different names") {
  std::set<Id> seen;
  for (int i = 0; i < 1000; ++i) seen.insert(content_id(("text number " + std::to_string(i)).c_str()));
  std::cout << "  a thousand texts, " << seen.size() << " different names\n\n";
  RC_CHECK_EQ(seen.size(), static_cast<std::size_t>(1000));
}
