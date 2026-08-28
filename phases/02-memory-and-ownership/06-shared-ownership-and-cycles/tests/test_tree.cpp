#include <rc/test/rc_test.hpp>

#include <memory>

#include "solution.hpp"

RC_TEST("a node is alive while a handle is held") {
  const int before = Node::live();
  {
    const std::shared_ptr<Node> base = Node::create("base");
    RC_CHECK_EQ(Node::live(), before + 1);
    RC_CHECK_EQ(base->name(), std::string("base"));
  }
  RC_CHECK_EQ(Node::live(), before);
}

RC_TEST("a parent owns its children") {
  const std::shared_ptr<Node> base = Node::create("base");
  add_child(base, Node::create("arm"));
  add_child(base, Node::create("mast"));
  RC_CHECK_EQ(base->child_count(), 2);
}

RC_TEST("a child can reach its parent") {
  const std::shared_ptr<Node> base = Node::create("base");
  const std::shared_ptr<Node> arm = Node::create("arm");
  add_child(base, arm);

  const std::shared_ptr<Node> found = arm->parent();
  RC_REQUIRE(found != nullptr);
  RC_CHECK_EQ(found->name(), std::string("base"));
}

RC_TEST("a root has no parent") {
  const std::shared_ptr<Node> base = Node::create("base");
  RC_CHECK(base->parent() == nullptr);
}

RC_TEST("adding a null changes nothing and does not crash") {
  const std::shared_ptr<Node> base = Node::create("base");
  add_child(base, nullptr);
  add_child(nullptr, base);
  RC_CHECK_EQ(base->child_count(), 0);
}

RC_TEST("descendants are counted at every depth") {
  const std::shared_ptr<Node> base = Node::create("base");
  const std::shared_ptr<Node> arm = Node::create("arm");
  const std::shared_ptr<Node> gripper = Node::create("gripper");
  add_child(base, arm);
  add_child(arm, gripper);
  add_child(gripper, Node::create("sensor"));

  RC_CHECK_EQ(base->descendant_count(), 3);
  RC_CHECK_EQ(arm->descendant_count(), 2);
  RC_CHECK_EQ(gripper->descendant_count(), 1);
}

RC_TEST("the whole tree is released when the root is dropped") {
  // This is the test the cycle fails. With an owning link in both directions
  // every count stays above zero and not one destructor runs.
  const int before = Node::live();
  {
    const std::shared_ptr<Node> base = Node::create("base");
    const std::shared_ptr<Node> arm = Node::create("arm");
    add_child(base, arm);
    add_child(arm, Node::create("gripper"));
    RC_CHECK_EQ(Node::live(), before + 3);
  }
  RC_CHECK_EQ(Node::live(), before);
}

RC_TEST("a deep tree is released completely") {
  const int before = Node::live();
  {
    const std::shared_ptr<Node> root = Node::create("root");
    std::shared_ptr<Node> current = root;
    for (int depth = 0; depth < 50; ++depth) {
      const std::shared_ptr<Node> next = Node::create("link");
      add_child(current, next);
      current = next;
    }
    RC_CHECK_EQ(Node::live(), before + 51);
  }
  RC_CHECK_EQ(Node::live(), before);
}

RC_TEST("the parent link does not keep the parent alive") {
  const int before = Node::live();
  std::shared_ptr<Node> arm;
  {
    const std::shared_ptr<Node> base = Node::create("base");
    arm = Node::create("arm");
    add_child(base, arm);
    RC_CHECK(arm->parent() != nullptr);
  }
  // The base is gone even though the arm still holds a link upward, and the
  // arm survives because the test still holds it.
  RC_CHECK_EQ(Node::live(), before + 1);
  RC_CHECK(arm->parent() == nullptr);
}

RC_TEST("a child stays alive while the parent holds it, even with no other handle") {
  const int before = Node::live();
  {
    const std::shared_ptr<Node> base = Node::create("base");
    add_child(base, Node::create("arm"));
    // Nothing outside the tree refers to the arm, and it must still exist.
    RC_CHECK_EQ(Node::live(), before + 2);
    RC_CHECK_EQ(base->child_count(), 1);
  }
  RC_CHECK_EQ(Node::live(), before);
}
