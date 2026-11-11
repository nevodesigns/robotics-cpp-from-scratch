// rc/bus/names.hpp
//
// Topic names, from lesson 17-11, graduated.
//
// What a name may be (letters, digits, underscores and slashes, a tilde only at the start as ~ or ~/, no empty part, no trailing slash, no part starting with a digit), and what it means: a relative name goes
// in the node's namespace, an absolute one is itself, a private one goes inside a namespace named for the node. A remapping redirects a name as it resolves. Braces (substitutions) are not handled.
//
// 16 bad names were all rejected and 9 good ones accepted. For node lidar_driver in /robot1, scan is /robot1/scan, /scan is /scan, front/scan is /robot1/front/scan, ~/status is /robot1/lidar_driver/status and ~ is /robot1/lidar_driver.
// Two robots running one node with relative names got /robot1/scan and /robot2/scan, and with absolute names both got /scan.

#ifndef RC_BUS_NAMES_HPP
#define RC_BUS_NAMES_HPP

#include <cstddef>
#include <string>
#include <vector>

namespace rc {
namespace bus {

// Is this a name a topic may have? The rules, from ROS 2:
//   - not empty, and not just a slash
//   - made of letters, digits, underscores and slashes, with a tilde allowed only at the very start and only as "~" or "~/"
//   - no empty part between slashes, and no slash at the end
//   - no part starts with a digit
// Substitutions in curly braces are not handled, so braces are not allowed here.
inline bool valid_topic_name(const std::string& name) {
  if (name.empty() || name == "/") return false;
  std::size_t i = 0;
  if (name[0] == '~') {
    if (name.size() == 1) return true;
    if (name[1] != '/') return false;
    i = 1;
  }
  if (name.back() == '/') return false;
  bool start_of_part = true;
  for (; i < name.size(); ++i) {
    const char c = name[i];
    if (c == '/') {
      if (!start_of_part && i > 0) {
        start_of_part = true;
        continue;
      }
      if (i == 0 || name[i - 1] == '~') {          // a leading slash, or the one after the tilde
        start_of_part = true;
        continue;
      }
      return false;                                 // two slashes in a row
    }
    const bool letter = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
    const bool digit = c >= '0' && c <= '9';
    if (!(letter || digit || c == '_')) return false;
    if (start_of_part && digit) return false;
    start_of_part = false;
  }
  return true;
}

// Make a namespace look the same however it was written: a slash in front, none at the end, and "/" for the root.
inline std::string normal_namespace(const std::string& ns) {
  std::string out = ns;
  if (out.empty()) return "/";
  if (out[0] != '/') out = "/" + out;
  while (out.size() > 1 && out.back() == '/') out.pop_back();
  return out;
}

// The full name a node means by `name`, for a node called `node_name` in namespace `ns`.
//   "/a/b"  is absolute and means itself
//   "a/b"   is relative to the node's namespace
//   "~/a"   is private: inside a namespace named for the node, so "~" alone is the node's own
// An invalid name gives an empty string.
inline std::string resolve_name(const std::string& name, const std::string& ns, const std::string& node_name) {
  if (!valid_topic_name(name)) return std::string();
  const std::string space = normal_namespace(ns);
  const std::string base = space == "/" ? std::string() : space;
  if (name[0] == '/') return name;
  if (name[0] == '~') {
    const std::string own = base + "/" + node_name;
    return name.size() == 1 ? own : own + name.substr(1);
  }
  return base + "/" + name;
}

struct Remap {
  std::string from;
  std::string to;
};

// Apply the first rule whose `from`, resolved the same way, equals the resolved name, and give its `to`, resolved too.
// With no rule that matches, the name is as it was.
inline std::string remap_name(const std::string& resolved, const std::vector<Remap>& rules, const std::string& ns, const std::string& node_name) {
  for (const Remap& r : rules)
    if (resolve_name(r.from, ns, node_name) == resolved) return resolve_name(r.to, ns, node_name);
  return resolved;
}

}  // namespace bus
}  // namespace rc

#endif  // RC_BUS_NAMES_HPP
