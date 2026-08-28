#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <memory>
#include <string>
#include <vector>

// A subsystem in a robot's tree of parts. Counts live instances so the tests
// can prove the whole tree is released.
class Node : public std::enable_shared_from_this<Node> {
 public:
  // Nodes are always created through this, never on the stack, because a weak
  // link to a stack object could never be locked.
  static std::shared_ptr<Node> create(std::string name) {
    return std::shared_ptr<Node>(new Node(std::move(name)));
  }

  ~Node() { --live(); }

  Node(const Node&) = delete;
  Node& operator=(const Node&) = delete;

  const std::string& name() const { return name_; }

  // The parent, or nullptr when there is none or it has already been destroyed.
  std::shared_ptr<Node> parent() const {
    // TODO: a weak link cannot be used directly. Ask it for a real handle with
    // lock(), which answers null when the object has gone.
    return nullptr;
  }

  int child_count() const { return static_cast<int>(children_.size()); }

  // Every node below this one, at any depth.
  int descendant_count() const {
    // TODO
    return 0;
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
  std::vector<std::shared_ptr<Node>> children_;

  // TODO: declare the link back to the parent.
  //
  // A std::shared_ptr<Node> here would compile, work, and leak the entire tree
  // forever, because parent and child would each keep the other's count above
  // zero. Choose the type that refers without owning.
};

// Makes parent own child, and gives child a non owning link back.
inline void add_child(const std::shared_ptr<Node>& parent,
                      const std::shared_ptr<Node>& child) {
  // TODO: ignore null arguments, then attach both directions.
  (void)parent;
  (void)child;
}

#endif  // LESSON_SOLUTION_HPP
