#include <rc/test/rc_test.hpp>

#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include "solution.hpp"

namespace {

// A node whose callbacks write a log, and whose result for each can be set.
struct Rig {
  LifecycleNode node;
  std::vector<std::string> log;
  CallbackResult configure_says = CallbackResult::success;
  CallbackResult activate_says = CallbackResult::success;
  CallbackResult deactivate_says = CallbackResult::success;
  CallbackResult cleanup_says = CallbackResult::success;
  CallbackResult shutdown_says = CallbackResult::success;
  CallbackResult recovery_says = CallbackResult::success;

  Rig() {
    node.on(Transition::configure, [this]() { log.push_back("configure"); return configure_says; });
    node.on(Transition::activate, [this]() { log.push_back("activate"); return activate_says; });
    node.on(Transition::deactivate, [this]() { log.push_back("deactivate"); return deactivate_says; });
    node.on(Transition::cleanup, [this]() { log.push_back("cleanup"); return cleanup_says; });
    node.on(Transition::shutdown, [this]() { log.push_back("shutdown"); return shutdown_says; });
    node.on_error([this]() { log.push_back("on_error"); return recovery_says; });
  }
};

}  // namespace

RC_TEST("a node goes from unconfigured to inactive to active, and back down again, one step at a time") {
  Rig r;
  RC_CHECK(r.node.state() == LifeState::unconfigured);
  std::cout << "  the way up and down:";
  const Transition steps[] = {Transition::configure, Transition::activate, Transition::deactivate, Transition::cleanup, Transition::shutdown};
  const LifeState expected[] = {LifeState::inactive, LifeState::active, LifeState::inactive, LifeState::unconfigured, LifeState::finalized};
  for (int i = 0; i < 5; ++i) {
    const TransitionResult t = r.node.trigger(steps[i]);
    std::cout << " -> " << name_of(t.state);
    RC_CHECK(t.legal);
    RC_CHECK(t.state == expected[i]);
    RC_CHECK(r.node.state() == expected[i]);
  }
  std::cout << "\n\n";
  RC_CHECK(r.log.size() == 5);
  RC_CHECK(r.log.size() == 5 && r.log[0] == "configure" && r.log[4] == "shutdown");
}

RC_TEST("a transition asked for from the wrong state is refused and its callback is not run") {
  Rig r;
  const TransitionResult a = r.node.trigger(Transition::activate);        // not configured yet
  const TransitionResult b = r.node.trigger(Transition::deactivate);
  const TransitionResult c = r.node.trigger(Transition::cleanup);
  r.node.trigger(Transition::configure);
  const TransitionResult d = r.node.trigger(Transition::configure);       // twice
  r.node.trigger(Transition::activate);
  const TransitionResult e = r.node.trigger(Transition::cleanup);         // still active
  const TransitionResult f = r.node.trigger(Transition::activate);        // twice
  std::cout << "  six requests out of order: legal " << a.legal << b.legal << c.legal << d.legal << e.legal << f.legal << "; the callbacks that ran:";
  for (const std::string& s : r.log) std::cout << " " << s;
  std::cout << "\n\n";
  RC_CHECK(!a.legal && !b.legal && !c.legal && !d.legal && !e.legal && !f.legal);
  RC_CHECK(a.state == LifeState::unconfigured);
  RC_CHECK(e.state == LifeState::active);
  RC_CHECK(r.log.size() == 2);                                            // only the configure and the activate that were legal
  RC_CHECK(r.node.state() == LifeState::active);
}

RC_TEST("a failed step leaves the node where it was, so it can be tried again, and a failed shutdown does not finalize") {
  Rig r;
  r.configure_says = CallbackResult::failure;
  const TransitionResult first = r.node.trigger(Transition::configure);
  RC_CHECK(first.legal);
  RC_CHECK(first.callback == CallbackResult::failure);
  RC_CHECK(first.state == LifeState::unconfigured);
  r.configure_says = CallbackResult::success;
  RC_CHECK(r.node.trigger(Transition::configure).state == LifeState::inactive);          // the retry

  r.activate_says = CallbackResult::failure;
  RC_CHECK(r.node.trigger(Transition::activate).state == LifeState::inactive);
  r.activate_says = CallbackResult::success;
  RC_CHECK(r.node.trigger(Transition::activate).state == LifeState::active);

  r.deactivate_says = CallbackResult::failure;
  RC_CHECK(r.node.trigger(Transition::deactivate).state == LifeState::active);           // could not stop: still active
  r.shutdown_says = CallbackResult::failure;
  const TransitionResult s = r.node.trigger(Transition::shutdown);
  std::cout << "  configure failed: " << name_of(first.state) << ", tried again: inactive; a deactivate that failed: still active; a shutdown that failed: " << name_of(s.state) << "\n\n";
  RC_CHECK(s.state == LifeState::active);
}

RC_TEST("an error sends the node to error processing, and its own recovery decides between starting over and being finished") {
  Rig recoverable;
  recoverable.node.trigger(Transition::configure);
  recoverable.node.trigger(Transition::activate);
  recoverable.deactivate_says = CallbackResult::error;
  recoverable.recovery_says = CallbackResult::success;
  const TransitionResult a = recoverable.node.trigger(Transition::deactivate);
  RC_CHECK(a.callback == CallbackResult::error);
  RC_CHECK(a.state == LifeState::unconfigured);                                           // recovered: back to the start
  RC_CHECK(recoverable.log.back() == "on_error");
  RC_CHECK(recoverable.node.trigger(Transition::configure).state == LifeState::inactive);  // and it can be brought up again

  Rig hopeless;
  hopeless.node.trigger(Transition::configure);
  hopeless.configure_says = CallbackResult::success;
  hopeless.activate_says = CallbackResult::error;
  hopeless.recovery_says = CallbackResult::failure;
  const TransitionResult b = hopeless.node.trigger(Transition::activate);
  std::cout << "  an error with a recovery that works: " << name_of(a.state) << ", brought up again; an error whose recovery fails: " << name_of(b.state) << "\n\n";
  RC_CHECK(b.state == LifeState::finalized);
  RC_CHECK(!hopeless.node.trigger(Transition::configure).legal);                         // finalized is the end
  RC_CHECK(!hopeless.node.trigger(Transition::shutdown).legal);
}

RC_TEST("shutdown is allowed from every state a node can be in while running, and not from finalized") {
  int finalized = 0;
  for (int i = 0; i < 3; ++i) {
    Rig r;
    if (i >= 1) r.node.trigger(Transition::configure);
    if (i == 2) r.node.trigger(Transition::activate);
    const LifeState from = r.node.state();
    const TransitionResult t = r.node.trigger(Transition::shutdown);
    std::cout << "  shutdown from " << name_of(from) << ": " << name_of(t.state) << "\n";
    if (t.legal && t.state == LifeState::finalized) ++finalized;
  }
  std::cout << "\n";
  RC_CHECK(finalized == 3);
  Rig done;
  done.node.trigger(Transition::shutdown);
  RC_CHECK(!done.node.trigger(Transition::shutdown).legal);
  RC_CHECK(done.node.state() == LifeState::finalized);
}

RC_TEST("a node only speaks while it is active, and what it is told to send at any other time is dropped and counted") {
  Rig r;
  std::vector<std::string> wire;
  const std::function<void(const std::string&)> send = [&wire](const std::string& s) { wire.push_back(s); };
  r.node.publish("a", send);                                 // unconfigured
  r.node.trigger(Transition::configure);
  r.node.publish("b", send);                                 // inactive
  r.node.trigger(Transition::activate);
  r.node.publish("c", send);
  r.node.publish("d", send);
  r.node.trigger(Transition::deactivate);
  r.node.publish("e", send);                                 // inactive again
  std::cout << "  five sent over the life of the node: on the wire:";
  for (const std::string& s : wire) std::cout << " " << s;
  std::cout << "; sent " << r.node.sent() << ", dropped " << r.node.dropped() << "\n\n";
  RC_CHECK(wire.size() == 2);
  RC_CHECK(wire.size() == 2 && wire[0] == "c" && wire[1] == "d");
  RC_CHECK(r.node.sent() == 2);
  RC_CHECK(r.node.dropped() == 3);
}
