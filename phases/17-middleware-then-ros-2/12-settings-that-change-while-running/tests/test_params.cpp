#include <rc/test/rc_test.hpp>

#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include "solution.hpp"

namespace {

ParameterServer make() {
  ParameterServer p;
  Descriptor ranged;
  ranged.has_range = true;
  ranged.lowest = 0.0;
  ranged.highest = 100.0;
  p.declare("kp", Value::of(2.0), ranged);
  p.declare("max_speed", Value::of(1.0), Descriptor());
  p.declare("min_speed", Value::of(0.1), Descriptor());
  p.declare("name", Value::of(std::string("arm")), Descriptor());
  p.declare("enabled", Value::of(true), Descriptor());
  p.declare("channels", Value::of(8L), Descriptor());
  Descriptor fixed;
  fixed.read_only = true;
  p.declare("serial", Value::of(std::string("A-1001")), fixed);
  return p;
}

}  // namespace

RC_TEST("a setting is read back as it was declared, and one that was never declared is not there") {
  ParameterServer p = make();
  RC_CHECK(p.has("kp"));
  RC_CHECK(!p.has("ki"));
  RC_CHECK(p.get("kp").type == ParamType::real);
  RC_CHECK_NEAR(p.get("kp").d, 2.0, 1e-12);
  RC_CHECK(p.get("channels").type == ParamType::integer && p.get("channels").i == 8);
  RC_CHECK(p.get("enabled").b);
  RC_CHECK(p.get("name").s == "arm");
  const SetResult r = p.set("ki", Value::of(1.0));
  std::cout << "  set of a name never declared: " << (r.ok ? "accepted" : "refused") << " (" << r.reason << ")\n\n";
  RC_CHECK(!r.ok);
  RC_CHECK(!p.has("ki"));
}

RC_TEST("a valid change takes effect and is recorded with what it replaced") {
  ParameterServer p = make();
  const SetResult r = p.set("kp", Value::of(3.5));
  RC_CHECK(r.ok);
  RC_CHECK_NEAR(p.get("kp").d, 3.5, 1e-12);
  RC_CHECK(p.changes().size() == 1);
  RC_CHECK(p.changes().size() == 1 && p.changes()[0].name == "kp");
  RC_CHECK(p.changes().size() == 1 && p.changes()[0].before.d == 2.0 && p.changes()[0].after.d == 3.5);
  RC_CHECK(p.set("kp", Value::of(3.5)).ok);              // the same value again is allowed and is not a change
  RC_CHECK(p.changes().size() == 1);
}

RC_TEST("a wrong type, a value out of range and a read only setting are refused, and nothing changes") {
  ParameterServer p = make();
  const SetResult as_int = p.set("kp", Value::of(5L));
  const SetResult too_big = p.set("kp", Value::of(250.0));
  const SetResult negative = p.set("kp", Value::of(-1.0));
  const SetResult locked = p.set("serial", Value::of(std::string("B-2002")));
  const SetResult as_real = p.set("channels", Value::of(8.0));
  std::cout << "  kp as an integer: " << as_int.reason << "\n  kp = 250: " << too_big.reason << "\n  kp = -1: " << negative.reason << "\n  serial: "
            << locked.reason << "\n  channels as a real: " << as_real.reason << "\n\n";
  RC_CHECK(!as_int.ok);
  RC_CHECK(!too_big.ok);
  RC_CHECK(!negative.ok);
  RC_CHECK(!locked.ok);
  RC_CHECK(!as_real.ok);
  RC_CHECK_NEAR(p.get("kp").d, 2.0, 1e-12);
  RC_CHECK(p.get("serial").s == "A-1001");
  RC_CHECK(p.changes().empty());
  RC_CHECK(p.refused() == 5);
  RC_CHECK(p.set("kp", Value::of(100.0)).ok);            // the ends of the range are in
  RC_CHECK(p.set("kp", Value::of(0.0)).ok);
}

RC_TEST("several settings changed together are all applied or none, even when the last is the bad one") {
  ParameterServer p = make();
  const SetResult r = p.set_many({{"kp", Value::of(9.0)}, {"enabled", Value::of(false)}, {"channels", Value::of(16.0)}});   // the last is the wrong type
  std::cout << "  three changes, the last the wrong type: " << (r.ok ? "applied" : "refused") << " (" << r.reason << "); kp is still " << std::fixed
            << std::setprecision(1) << p.get("kp").d << ", enabled is " << (p.get("enabled").b ? "true" : "false") << "\n\n";
  RC_CHECK(!r.ok);
  RC_CHECK_NEAR(p.get("kp").d, 2.0, 1e-12);
  RC_CHECK(p.get("enabled").b);
  RC_CHECK(p.changes().empty());

  const SetResult good = p.set_many({{"kp", Value::of(9.0)}, {"enabled", Value::of(false)}, {"channels", Value::of(16L)}});
  RC_CHECK(good.ok);
  RC_CHECK_NEAR(p.get("kp").d, 9.0, 1e-12);
  RC_CHECK(!p.get("enabled").b);
  RC_CHECK(p.get("channels").i == 16);
  RC_CHECK(p.changes().size() == 3);
}

RC_TEST("a rule about the settings together is checked on how they would end up, so a pair that is fine together can be changed together") {
  ParameterServer p = make();
  p.set_validator([](const std::map<std::string, Value>& s) -> std::string {
    if (s.at("min_speed").d >= s.at("max_speed").d) return "min_speed must be below max_speed";
    return std::string();
  });
  // The pair (0.1, 1.0) is fine. Move both up to (2.0, 3.0): fine at the end.
  const SetResult first = p.set("min_speed", Value::of(2.0));       // one at a time, the first step is already wrong
  const SetResult together = p.set_many({{"min_speed", Value::of(2.0)}, {"max_speed", Value::of(3.0)}});
  const SetResult bad = p.set_many({{"min_speed", Value::of(5.0)}, {"max_speed", Value::of(4.0)}});
  std::cout << "  min 0.1, max 1.0 moved to min 2.0, max 3.0\n  one at a time, min first: " << (first.ok ? "ok" : "refused") << " (" << first.reason
            << ")\n  together: " << (together.ok ? "ok" : "refused") << "\n  min 5.0 with max 4.0 together: " << (bad.ok ? "ok" : "refused") << " (" << bad.reason << ")\n\n";
  RC_CHECK(!first.ok);
  RC_CHECK(together.ok);
  RC_CHECK_NEAR(p.get("min_speed").d, 2.0, 1e-12);
  RC_CHECK_NEAR(p.get("max_speed").d, 3.0, 1e-12);
  RC_CHECK(!bad.ok);
  RC_CHECK_NEAR(p.get("min_speed").d, 2.0, 1e-12);                   // unchanged by the refusal
  RC_CHECK(bad.reason == "min_speed must be below max_speed");
}
