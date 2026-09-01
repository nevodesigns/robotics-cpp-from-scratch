// rc/core/subsystem.hpp
//
// The subsystem tree from lesson 02-06, graduated.
//
// Ownership points one way, down, and the way back up is a weak reference. Two
// shared pointers pointing at each other keep each other alive for ever, and
// the leak is invisible because nothing is obviously wrong with either object.

#ifndef RC_CORE_SUBSYSTEM_HPP
#define RC_CORE_SUBSYSTEM_HPP

#include <memory>
#include <string>
#include <vector>

namespace rc {
namespace core {

class Node : public std::enable_shared_from_this<Node> {
 public:
  static std::shared_ptr<Node> create(std::string name) {
    return std::shared_ptr<Node>(new Node(std::move(name)));
  }

  ~Node() { --live(); }

  Node(const Node&) = delete;
  Node& operator=(const Node&) = delete;

  const std::string& name() const { return name_; }

  // lock() answers a usable handle when the object is still alive and null when
  // it is not. Returning that directly passes the obligation to check on to the
  // caller, which is honest: a parent really can have been shut down already.
  std::shared_ptr<Node> parent() const { return parent_.lock(); }

  int child_count() const { return static_cast<int>(children_.size()); }

  int descendant_count() const {
    int total = 0;
    for (const std::shared_ptr<Node>& child : children_) {
      total += 1 + child->descendant_count();
    }
    return total;
  }

  static int& live() {
    static int count = 0;
    return count;
  }

 private:
  explicit Node(std::string name) : name_(std::move(name)) { ++live(); }

  friend void add_child(const std::shared_ptr<Node>& parent,
                        const std::shared_ptr<Node>& child);

  std::string name_;

  // One direction owns. Children are kept alive by their parent.
  std::vector<std::shared_ptr<Node>> children_;

  // The other direction observes. A shared_ptr here would keep the parent alive
  // from below, and since the parent already keeps the child alive from above,
  // neither count would ever reach zero and the whole tree would leak.
  std::weak_ptr<Node> parent_;
};

inline void add_child(const std::shared_ptr<Node>& parent,
                      const std::shared_ptr<Node>& child) {
  if (parent == nullptr || child == nullptr) return;

  parent->children_.push_back(child);

  // Assigning a shared_ptr to a weak_ptr is the point at which the ownership
  // decision is made: the link is recorded without the count being raised.
  child->parent_ = parent;
}

}  // namespace core
}  // namespace rc

#endif  // RC_CORE_SUBSYSTEM_HPP
