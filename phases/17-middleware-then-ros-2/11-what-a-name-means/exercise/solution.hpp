#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cstddef>
#include <string>
#include <vector>

// Is this a name a topic may have? The rules, from ROS 2:
//   - not empty, and not just a slash
//   - made of letters, digits, underscores and slashes, with a tilde allowed only at the very start and only as "~" or "~/"
//   - no empty part between slashes, and no slash at the end
//   - no part starts with a digit
// Substitutions in curly braces are not handled, so braces are not allowed here.
inline bool valid_topic_name(const std::string& name) {
  // TODO: See the rules above. False for an empty name or "/". A tilde is only allowed at the start, alone or followed by a slash.
  // Walk the characters: letters, digits and underscores are fine, anything else is not; a slash must follow something (or be the first character, or the one after the tilde),
  // so "a//b" and a name ending in "/" are not valid; and a part may not begin with a digit.
  (void)name;
  return false;
}

// Make a namespace look the same however it was written: a slash in front, none at the end, and "/" for the root.
inline std::string normal_namespace(const std::string& ns) {
  // TODO: an empty namespace is "/"; otherwise make sure it starts with a slash and has none at the end, except for the root
  (void)ns;
  return std::string();
}

// The full name a node means by `name`, for a node called `node_name` in namespace `ns`.
//   "/a/b"  is absolute and means itself
//   "a/b"   is relative to the node's namespace
//   "~/a"   is private: inside a namespace named for the node, so "~" alone is the node's own
// An invalid name gives an empty string.
inline std::string resolve_name(const std::string& name, const std::string& ns, const std::string& node_name) {
  // TODO: An invalid name gives an empty string. The namespace is normalised; the root contributes nothing (so "/" + name does not become "//name").
  // An absolute name (starts with /) is itself. A name starting with ~ is namespace + "/" + node_name, followed by the rest of the name after the tilde.
  // Anything else is namespace + "/" + name.
  (void)name;
  (void)ns;
  (void)node_name;
  return std::string();
}

struct Remap {
  std::string from;
  std::string to;
};

// Apply the first rule whose `from`, resolved the same way, equals the resolved name, and give its `to`, resolved too.
// With no rule that matches, the name is as it was.
inline std::string remap_name(const std::string& resolved, const std::vector<Remap>& rules, const std::string& ns, const std::string& node_name) {
  // TODO: For each rule in order: if the rule's `from`, resolved with resolve_name, equals the resolved name, return the rule's `to` resolved.
  // No rule matches: return the name as it was.
  (void)rules;
  (void)ns;
  (void)node_name;
  return resolved;
}

#endif  // LESSON_SOLUTION_HPP
