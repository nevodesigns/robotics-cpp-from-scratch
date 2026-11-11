#include <rc/test/rc_test.hpp>

#include <iomanip>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "solution.hpp"

namespace {

// A component, written against Node and nothing else. It never mentions a bus, a clock or ROS 2.
// It takes numbers on "in", doubles them onto "out", and every second reports how many it has handled on "status".
class Doubler {
 public:
  explicit Doubler(Node& node) {
    out_ = node.create_publisher("out");
    status_ = node.create_publisher("~/status");
    node.create_subscription("in", [this](const std::string& payload) {
      ++handled_;
      out_(std::to_string(2 * std::stoi(payload)));
    });
    node.create_timer(1.0, [this]() { status_("handled " + std::to_string(handled_)); });
  }
  int handled() const { return handled_; }

 private:
  std::function<void(const std::string&)> out_;
  std::function<void(const std::string&)> status_;
  int handled_ = 0;
};

}  // namespace

RC_TEST("a component given a node publishes and subscribes under the names the node resolves, in the node's namespace") {
  World world;
  InProcessNode node(world, "/robot1", "doubler");
  Doubler doubler(node);
  std::vector<std::string> out;
  std::vector<std::string> out_elsewhere;
  InProcessNode listener(world, "/robot1", "listener");
  listener.create_subscription("out", [&out](const std::string& p) { out.push_back(p); });
  InProcessNode other_robot(world, "/robot2", "listener");
  other_robot.create_subscription("out", [&out_elsewhere](const std::string& p) { out_elsewhere.push_back(p); });
  auto send = listener.create_publisher("in");
  send("21");
  send("4");
  std::cout << "  names: " << node.full_name("out") << ", " << node.full_name("~/status") << ", " << node.full_name("/clock") << "; the listener in /robot1 heard '"
            << (out.empty() ? "" : out[0]) << "' and '" << (out.size() < 2 ? "" : out[1]) << "', the one in /robot2 heard " << out_elsewhere.size() << "\n\n";
  RC_CHECK(node.full_name("out") == "/robot1/out");
  RC_CHECK(node.full_name("~/status") == "/robot1/doubler/status");
  RC_CHECK(node.full_name("/clock") == "/clock");
  RC_CHECK(doubler.handled() == 2);
  RC_CHECK(out.size() == 2);
  RC_CHECK(out.size() == 2 && out[0] == "42" && out[1] == "8");
  RC_CHECK(out_elsewhere.empty());
}

RC_TEST("a timer on the node fires once per period of the world's clock, and the component's own status goes out on it") {
  World world;
  InProcessNode node(world, "", "doubler");
  Doubler doubler(node);
  std::vector<std::string> statuses;
  InProcessNode watcher(world, "", "watcher");
  watcher.create_subscription("/doubler/status", [&statuses](const std::string& p) { statuses.push_back(p); });
  auto send = watcher.create_publisher("in");
  for (int i = 0; i < 35; ++i) {
    world.advance(0.1);
    if (i % 10 == 4) send("1");
  }
  std::cout << "  3.5 s of the world's clock, a number sent at 0.5 s, 1.5 s, 2.5 s and 3.5 s: " << statuses.size() << " status messages:";
  for (const std::string& s : statuses) std::cout << " '" << s << "'";
  std::cout << "; the node's clock reads " << std::fixed << std::setprecision(1) << node.now() << "\n\n";
  RC_CHECK(statuses.size() == 3);
  RC_CHECK(statuses.size() == 3 && statuses[0] == "handled 1" && statuses[1] == "handled 2" && statuses[2] == "handled 3");
  RC_CHECK_NEAR(node.now(), 3.5, 1e-9);
}

RC_TEST("two nodes of the same component in two namespaces do not hear each other, and in one namespace they do") {
  World world;
  InProcessNode a(world, "/robot1", "doubler");
  InProcessNode b(world, "/robot2", "doubler");
  Doubler da(a);
  Doubler db(b);
  auto send_to_1 = a.create_publisher("in");
  send_to_1("5");
  send_to_1("6");
  std::cout << "  two robots, a number sent to robot1's input twice: robot1 handled " << da.handled() << ", robot2 handled " << db.handled() << "\n\n";
  RC_CHECK(da.handled() == 2);
  RC_CHECK(db.handled() == 0);
}

RC_TEST("a node that is destroyed takes its subscriptions and timers with it, and nothing is called into it afterwards") {
  World world;
  std::vector<std::string> status;
  InProcessNode watcher(world, "", "watcher");
  watcher.create_subscription("/gone/out", [&status](const std::string& p) { status.push_back(p); });
  int calls_into_gone_node = 0;
  {
    InProcessNode gone(world, "", "gone");
    gone.create_subscription("ping", [&calls_into_gone_node](const std::string&) { ++calls_into_gone_node; });
    gone.create_timer(0.5, [&calls_into_gone_node]() { ++calls_into_gone_node; });
    auto ping = watcher.create_publisher("ping");
    ping("a");
    world.advance(0.6);
    RC_CHECK(calls_into_gone_node == 2);                       // one subscription call and one timer firing
    RC_CHECK(world.node_count() == 2);
    RC_CHECK(world.bus.subscriber_count("/ping") == 1);
  }
  RC_CHECK(world.node_count() == 1);
  RC_CHECK(world.bus.subscriber_count("/ping") == 0);
  auto ping = watcher.create_publisher("ping");
  const int before = calls_into_gone_node;
  ping("b");
  world.advance(5.0);
  std::cout << "  a node with a subscription and a timer was destroyed: nodes alive " << world.node_count() << ", subscribers to its topic " << world.bus.subscriber_count("/ping")
            << ", calls into it afterwards " << calls_into_gone_node - before << "\n\n";
  RC_CHECK(calls_into_gone_node == before);
}

RC_TEST("a node is created and destroyed again and again with the world still working, and an invalid name does nothing instead of throwing") {
  World world;
  InProcessNode keeper(world, "/r", "keeper");
  int heard = 0;
  keeper.create_subscription("t", [&heard](const std::string&) { ++heard; });
  auto send = keeper.create_publisher("t");
  for (int i = 0; i < 200; ++i) {
    InProcessNode temp(world, "/r", "temp");
    temp.create_subscription("t", [](const std::string&) {});
    send("x");
  }
  RC_CHECK(heard == 200);
  RC_CHECK(world.node_count() == 1);
  RC_CHECK(world.bus.subscriber_count("/r/t") == 1);
  auto bad = keeper.create_publisher("bad name");
  bad("ignored");
  keeper.create_subscription("also bad!", [&heard](const std::string&) { ++heard; });
  send("y");
  std::cout << "  200 nodes made and destroyed: nodes alive " << world.node_count() << ", subscribers to the topic " << world.bus.subscriber_count("/r/t") << ", heard " << heard
            << "; a publisher and a subscription on invalid names did nothing\n\n";
  RC_CHECK(heard == 201);
  RC_CHECK(keeper.full_name("bad name").empty());
}
