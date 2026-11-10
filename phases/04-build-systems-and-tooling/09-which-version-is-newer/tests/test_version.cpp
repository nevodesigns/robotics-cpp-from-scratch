#include <rc/test/rc_test.hpp>

#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

#include "solution.hpp"

namespace {

Version v(const char* text) {
  Version out;
  const bool ok = parse_version(text, &out);
  if (!ok) out.major = -1;
  return out;
}

}  // namespace

RC_TEST("versions are read, and the bad ones are refused and leave the output alone") {
  Version out;
  RC_CHECK(parse_version("1.10.2", &out));
  RC_CHECK_EQ(out.major, 1);
  RC_CHECK_EQ(out.minor, 10);
  RC_CHECK_EQ(out.patch, 2);
  RC_CHECK(out.pre.empty());

  RC_CHECK(parse_version("1.0.0-rc.1", &out));
  RC_CHECK_EQ(out.pre, std::string("rc.1"));
  RC_CHECK(parse_version("2.3.4+build7", &out));
  RC_CHECK_EQ(out.patch, 4);
  RC_CHECK(out.pre.empty());
  RC_CHECK(parse_version("0.0.0", &out));

  const char* refused[] = {"", "1", "1.2", "1.2.3.4", "1.x.0", "1..3", "01.2.3", "1.02.3", "-1.0.0", "1.0.0-",
                           "1.2.3-", "a.b.c", " 1.2.3", "1.2.3 ", "99999999999.0.0"};
  int count = 0;
  for (const char* text : refused) {
    Version before;
    before.major = 7;
    if (!parse_version(text, &before)) ++count;
  }
  std::cout << "  " << count << " of " << sizeof(refused) / sizeof(refused[0]) << " malformed versions refused\n\n";
  RC_CHECK_EQ(count, static_cast<int>(sizeof(refused) / sizeof(refused[0])));

  Version kept;
  kept.major = 7;
  RC_CHECK(!parse_version("1.2", &kept));
  RC_CHECK_EQ(kept.major, 7);
}

RC_TEST("1.9.0 is older than 1.10.0, though as text it sorts after") {
  const bool as_text = std::string("1.9.0") < std::string("1.10.0");
  std::cout << "  \"1.9.0\" < \"1.10.0\" as text: " << (as_text ? "true" : "false") << "; as versions compare gives "
            << compare(v("1.9.0"), v("1.10.0")) << "\n\n";
  RC_CHECK(!as_text);
  RC_CHECK_EQ(compare(v("1.9.0"), v("1.10.0")), -1);
  RC_CHECK_EQ(compare(v("1.10.0"), v("1.9.0")), 1);
  RC_CHECK_EQ(compare(v("2.0.0"), v("1.99.99")), 1);
  RC_CHECK_EQ(compare(v("1.2.3"), v("1.2.3")), 0);
  RC_CHECK_EQ(compare(v("1.2.3+a"), v("1.2.3+b")), 0);
  RC_CHECK_EQ(compare(v("0.0.9"), v("0.0.10")), -1);
}

RC_TEST("a release candidate comes before its release, and the pre-releases sort in the order the spec gives") {
  const char* chain[] = {"1.0.0-alpha", "1.0.0-alpha.1", "1.0.0-alpha.beta", "1.0.0-beta", "1.0.0-beta.2",
                         "1.0.0-beta.11", "1.0.0-rc.1", "1.0.0"};
  const int n = static_cast<int>(sizeof(chain) / sizeof(chain[0]));
  for (int i = 0; i + 1 < n; ++i) {
    RC_CHECK_EQ(compare(v(chain[i]), v(chain[i + 1])), -1);
    RC_CHECK_EQ(compare(v(chain[i + 1]), v(chain[i])), 1);
  }

  // As plain text "beta.11" sorts before "beta.2", which is the wrong way round.
  const bool text_wrong = std::string("1.0.0-beta.11") < std::string("1.0.0-beta.2");
  std::cout << "  \"1.0.0-beta.11\" < \"1.0.0-beta.2\" as text: " << (text_wrong ? "true" : "false")
            << "; as versions compare gives " << compare(v("1.0.0-beta.11"), v("1.0.0-beta.2")) << "\n";
  RC_CHECK(text_wrong);
  RC_CHECK_EQ(compare(v("1.0.0-beta.11"), v("1.0.0-beta.2")), 1);

  // Shuffled, then sorted with compare, they come out as the chain.
  std::vector<Version> shuffled;
  const int order[] = {5, 2, 7, 0, 6, 3, 1, 4};
  for (int i : order) shuffled.push_back(v(chain[i]));
  std::sort(shuffled.begin(), shuffled.end(), [](const Version& a, const Version& b) { return compare(a, b) < 0; });
  bool in_order = true;
  for (int i = 0; i < n; ++i) {
    if (compare(shuffled[static_cast<std::size_t>(i)], v(chain[i])) != 0) in_order = false;
  }
  std::cout << "  eight versions shuffled and sorted with compare came out in the published order: "
            << (in_order ? "yes" : "no") << "\n\n";
  RC_CHECK(in_order);
}

RC_TEST("a very long number in a pre-release cannot overflow the comparison") {
  RC_CHECK_EQ(compare(v("1.0.0-rc.99999999999999999999"), v("1.0.0-rc.100000000000000000000")), -1);
  RC_CHECK_EQ(compare(v("1.0.0-rc.100000000000000000000"), v("1.0.0-rc.99999999999999999999")), 1);
  RC_CHECK_EQ(compare(v("1.0.0-1"), v("1.0.0-alpha")), -1);      // a number is lower than a word
}

RC_TEST("compatible means the same major version and at least as new, with 0.x more strict, and no pre-release unless one was asked for") {
  RC_CHECK(compatible(v("1.4.0"), v("1.4.0")));
  RC_CHECK(compatible(v("1.4.0"), v("1.10.2")));
  RC_CHECK(!compatible(v("1.4.0"), v("1.3.9")));                  // older
  RC_CHECK(!compatible(v("1.4.0"), v("2.0.0")));                  // newer, but a new major version may break it
  RC_CHECK(!compatible(v("2.1.0"), v("1.9.0")));

  // Below 1.0.0 nothing is promised, so the minor number must match as well.
  RC_CHECK(compatible(v("0.3.1"), v("0.3.5")));
  RC_CHECK(!compatible(v("0.3.1"), v("0.4.0")));

  // A release candidate is not the release, and is not offered to a request for one.
  RC_CHECK(!compatible(v("1.2.0"), v("1.2.0-rc.1")));
  RC_CHECK(!compatible(v("1.4.0"), v("1.11.0-rc.1")));
  RC_CHECK(compatible(v("1.2.0-rc.1"), v("1.2.0")));
  RC_CHECK(compatible(v("1.2.0-rc.1"), v("1.2.0-rc.2")));

  // Choosing among what is installed: the newest that satisfies the request.
  const char* installed[] = {"1.2.0", "1.9.4", "2.0.1", "1.10.0", "1.11.0-rc.1", "0.9.0"};
  int best = -1;
  Version best_v;
  Version wanted = v("1.4.0");
  for (int i = 0; i < 6; ++i) {
    Version have = v(installed[i]);
    if (!compatible(wanted, have)) continue;
    if (best < 0 || compare(have, best_v) > 0) {
      best = i;
      best_v = have;
    }
  }
  std::cout << "  asked for 1.4.0 among 1.2.0, 1.9.4, 2.0.1, 1.10.0, 1.11.0-rc.1, 0.9.0: chose "
            << (best >= 0 ? installed[best] : "nothing") << "\n\n";
  RC_REQUIRE(best >= 0);
  RC_CHECK_EQ(std::string(installed[best]), std::string("1.10.0"));
}
