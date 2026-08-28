# Shared Ownership, and the Cycle That Never Releases

> Reach for shared_ptr when you genuinely cannot say who owns something. Reach for it because a unique_ptr would not compile, and you have written a leak with extra steps.

**Type:** Build
**Time:** about 105 minutes
**Platforms:** Ubuntu 22.04, Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 02-05

## The Problem

A robot is a tree of subsystems. The base owns an arm, the arm owns a gripper,
the gripper owns a sensor. Each part needs to reach its parent, to ask for the
current safety state or to report a fault upward.

The obvious implementation is for children to be held by `shared_ptr` and for
each child to keep a `shared_ptr` to its parent. It compiles, the links work in
both directions, and every test passes.

It also never frees anything. Not one node, for the entire life of the process.

## The Concept

### What shared_ptr actually is

`std::shared_ptr<T>` holds an object plus a count of how many shared_ptrs refer
to it. Copying one increments the count, destroying one decrements it, and when
the count reaches zero the object is destroyed.

That is genuine shared ownership: the object lives exactly as long as the last
handle to it, and no single owner has to be identified.

It costs more than `unique_ptr`: an extra allocation for the control block unless
you use `std::make_shared`, and an atomic increment and decrement on every copy,
which is not free in a control loop running at a kilohertz.

### When it is the right answer

Rarely, and always for the same reason: **the lifetime genuinely cannot be tied
to one owner**. A cache handing out entries that callers may hold for any length
of time. A callback registry where the subscriber may outlive the publisher or
the other way round. Objects passed between threads where neither can guarantee
which finishes last.

If you can name the owner, use `unique_ptr` and hand out plain references or non
owning pointers. That is faster, and it documents the design instead of hiding
it.

### The cycle

Here is the bug in full:

```cpp
struct Node {
  std::vector<std::shared_ptr<Node>> children;
  std::shared_ptr<Node> parent;      // this line is the leak
};
```

A parent holds its child, so the child's count is at least one. The child holds
its parent, so the parent's count is at least one. Drop every external handle and
both counts are still one, held by each other. Neither reaches zero. Neither
destructor runs. The memory is unreachable and never released, which is a leak
that a reference counting scheme cannot see by design.

Reference counting cannot collect cycles. That is not an implementation gap, it
is a property of counting, and it is the main reason languages with tracing
garbage collectors exist.

### weak_ptr breaks it

A `std::weak_ptr<T>` refers to an object without contributing to the count. It
cannot be dereferenced directly, because the object may already be gone. You ask
it for a real handle:

```cpp
std::shared_ptr<Node> alive = parent_.lock();
if (alive != nullptr) {
  alive->report(fault);
}
```

`lock()` answers a `shared_ptr` when the object still exists and null when it does
not. The check is compulsory, and that is the feature: a weak link forces the
holder to admit the target may be gone, which for a parent that has been shut
down is exactly the truth.

So the rule for any structure with links in two directions: **one direction owns,
the other observes.** Children are owned. Parents are observed.

## Build It

`exercise/solution.hpp` provides a `Node` that counts live instances. Implement:

- `Node::create(name)` returns a `std::shared_ptr<Node>`. Nodes are only ever
  created this way, because a weak link to a stack object could not work.
- `add_child(parent, child)` makes the parent own the child and gives the child a
  non owning link back to the parent.
- `Node::parent()` returns a `std::shared_ptr<Node>` to the parent, or nullptr
  when there is none, or when the parent has already been destroyed.
- `Node::child_count()` and `Node::descendant_count()`, the latter counting the
  whole subtree below this node.

The tests build trees, drop the root, and check the live count returns to zero.
With an owning link in both directions it will not.

```
rcpp verify 02-06
```

## Use It

Qt solves this a different way, with parent objects owning their children
outright and children holding a plain pointer upward, which works because Qt
guarantees the parent outlives the child. You will meet that in phase 09 onward.

ROS 2 hands out `shared_ptr` for nodes and messages, because a message may be
held by several subscribers with no way to know which finishes last. That is
shared ownership used for its actual purpose.

The pattern to avoid is a codebase where everything is a `shared_ptr` because
somebody hit a compile error and reached for the type that made it stop. That is
not a design, and the cycles arrive later.

## What Breaks First

- **Nothing is ever destroyed.** An owning link in both directions. See
  `E-MEM-0009`.
- **A crash when reaching for a parent.** You dereferenced the result of `lock()`
  without checking it, and the parent was gone. See `E-MEM-0003`.
- **The compiler refuses to copy a unique_ptr into a second place.** That is the
  question being asked: who owns this? See `E-MEM-0007`.

## Ship It

The subsystem tree goes into `rc::core`. The rule goes with you: in any structure
with links both ways, decide which direction owns, and make the other one weak.
