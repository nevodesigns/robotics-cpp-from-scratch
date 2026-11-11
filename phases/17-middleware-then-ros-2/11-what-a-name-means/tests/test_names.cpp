#include <rc/test/rc_test.hpp>

#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include "solution.hpp"

RC_TEST("names that are allowed, and the ways a name can be wrong") {
  const char* good[] = {"scan", "/scan", "robot1/scan", "/a/b/c", "~", "~/status", "my_topic_2", "_hidden", "A"};
  const char* bad[] = {"", "/", "scan/", "a//b", "//a", "1scan", "a/2b", "a b", "a-b", "a.b", "~x", "a~", "a/~/b", "/~", "{node}/x", "a/"};
  int accepted_good = 0;
  int rejected_bad = 0;
  std::cout << "  rejected:";
  for (const char* g : good)
    if (valid_topic_name(g)) ++accepted_good;
  for (const char* b : bad) {
    if (!valid_topic_name(b)) ++rejected_bad;
    else std::cout << " [accepted: '" << b << "']";
  }
  std::cout << " " << rejected_bad << " of " << sizeof(bad) / sizeof(bad[0]) << " bad names, and " << accepted_good << " of " << sizeof(good) / sizeof(good[0]) << " good ones accepted\n\n";
  RC_CHECK(accepted_good == static_cast<int>(sizeof(good) / sizeof(good[0])));
  RC_CHECK(rejected_bad == static_cast<int>(sizeof(bad) / sizeof(bad[0])));
}

RC_TEST("a namespace means the same however it was written") {
  RC_CHECK(normal_namespace("") == "/");
  RC_CHECK(normal_namespace("/") == "/");
  RC_CHECK(normal_namespace("robot1") == "/robot1");
  RC_CHECK(normal_namespace("/robot1/") == "/robot1");
  RC_CHECK(normal_namespace("/a/b") == "/a/b");
  RC_CHECK(normal_namespace("a/b//") == "/a/b");
}

RC_TEST("a relative name is put in the node's namespace, an absolute one is not, and a private one goes inside the node's own") {
  std::cout << "  node 'lidar_driver' in namespace '/robot1'\n  written          means\n";
  const char* names[] = {"scan", "/scan", "front/scan", "~/status", "~"};
  for (const char* n : names)
    std::cout << "  " << std::left << std::setw(16) << n << " " << std::right << resolve_name(n, "/robot1", "lidar_driver") << "\n";
  std::cout << "\n";
  RC_CHECK(resolve_name("scan", "/robot1", "lidar_driver") == "/robot1/scan");
  RC_CHECK(resolve_name("/scan", "/robot1", "lidar_driver") == "/scan");
  RC_CHECK(resolve_name("front/scan", "/robot1", "lidar_driver") == "/robot1/front/scan");
  RC_CHECK(resolve_name("~/status", "/robot1", "lidar_driver") == "/robot1/lidar_driver/status");
  RC_CHECK(resolve_name("~", "/robot1", "lidar_driver") == "/robot1/lidar_driver");
  RC_CHECK(resolve_name("scan", "robot1/", "lidar_driver") == "/robot1/scan");     // the namespace written loosely
  RC_CHECK(resolve_name("scan", "/", "n") == "/scan");                              // the root namespace: no double slash
  RC_CHECK(resolve_name("scan", "", "n") == "/scan");
  RC_CHECK(resolve_name("~/x", "/", "n") == "/n/x");
  RC_CHECK(resolve_name("bad name", "/", "n").empty());
}

RC_TEST("two robots running the same node with relative names do not collide, and with absolute names they do") {
  const std::string a1 = resolve_name("scan", "/robot1", "lidar");
  const std::string a2 = resolve_name("scan", "/robot2", "lidar");
  const std::string b1 = resolve_name("/scan", "/robot1", "lidar");
  const std::string b2 = resolve_name("/scan", "/robot2", "lidar");
  std::cout << "  relative: " << a1 << " and " << a2 << (a1 == a2 ? "  (the same)" : "  (apart)") << "\n  absolute: " << b1 << " and " << b2
            << (b1 == b2 ? "  (the same: one topic, two publishers)" : "  (apart)") << "\n\n";
  RC_CHECK(a1 != a2);
  RC_CHECK(a1 == "/robot1/scan");
  RC_CHECK(b1 == b2);
  RC_CHECK(b1 == "/scan");
}

RC_TEST("a remapping redirects a name without changing the node, and applies to the name as it resolves") {
  const std::vector<Remap> rules = {{"scan", "/filtered/scan"}, {"~/status", "/diagnostics/lidar"}};
  const std::string scan = resolve_name("scan", "/robot1", "lidar");
  const std::string status = resolve_name("~/status", "/robot1", "lidar");
  const std::string other = resolve_name("pose", "/robot1", "lidar");
  RC_CHECK(remap_name(scan, rules, "/robot1", "lidar") == "/filtered/scan");
  RC_CHECK(remap_name(status, rules, "/robot1", "lidar") == "/diagnostics/lidar");
  RC_CHECK(remap_name(other, rules, "/robot1", "lidar") == "/robot1/pose");
  // The same rule in another node's namespace is a rule about a different name, so it does not apply here.
  const std::string elsewhere = resolve_name("scan", "/robot2", "lidar");
  RC_CHECK(remap_name(elsewhere, rules, "/robot1", "lidar") == "/robot2/scan");
  std::cout << "  rules scan -> /filtered/scan and ~/status -> /diagnostics/lidar: " << scan << " becomes " << remap_name(scan, rules, "/robot1", "lidar")
            << ", " << status << " becomes " << remap_name(status, rules, "/robot1", "lidar") << ", " << other << " is unchanged\n\n";
}
