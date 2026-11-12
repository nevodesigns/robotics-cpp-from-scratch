#include <rc/test/rc_test.hpp>

#include <iostream>
#include <string>
#include <vector>

#include "solution.hpp"

namespace {

std::string joined(const std::vector<std::string>& v) {
  std::string s;
  for (const std::string& x : v) s += (s.empty() ? "" : ", ") + x;
  return s.empty() ? "none" : s;
}

Channel ch(const char* name, double safe, FailPolicy p) {
  Channel c;
  c.name = name;
  c.safe_value = safe;
  c.on_failure = p;
  return c;
}

}  // namespace

RC_TEST("hold keeps the last command, zero gives zero, and a safe value gives the channel's own") {
  RC_CHECK(output_on_failure(ch("a", 5.0, FailPolicy::hold), 0.8) == 0.8);
  RC_CHECK(output_on_failure(ch("a", 5.0, FailPolicy::zero), 0.8) == 0.0);
  RC_CHECK(output_on_failure(ch("a", 5.0, FailPolicy::safe_value), 0.8) == 5.0);
  RC_CHECK(output_on_failure(ch("a", 5.0, FailPolicy::hold), -3.0) == -3.0);
}

RC_TEST("an output stage left on its default keeps driving every channel that was moving when the controller died") {
  const std::vector<std::string> names = {"drive_left", "drive_right", "steering", "brake_coil", "horn"};
  const std::vector<double> last = {0.8, 0.8, 0.3, 24.0, 0.0};
  std::vector<Channel> channels;
  for (const std::string& n : names) channels.push_back(ch(n.c_str(), 0.0, FailPolicy::hold));
  const std::vector<std::string> unsafe = unsafe_on_failure(channels, last);
  std::cout << "  five channels, all on the default (hold), controller dead: unsafe: " << joined(unsafe) << "\n";
  RC_CHECK(unsafe.size() == 4);
  RC_CHECK(joined(unsafe) == "drive_left, drive_right, steering, brake_coil");
  for (Channel& c : channels) c.on_failure = FailPolicy::zero;
  const std::vector<std::string> after = unsafe_on_failure(channels, last);
  std::cout << "  the same five set to zero: unsafe: " << joined(after) << "\n\n";
  RC_CHECK(after.empty());
  RC_CHECK(unsafe_on_failure(channels, std::vector<double>()).empty());   // fewer commands than channels: only the pairs
}

RC_TEST("zero is only safe where zero means stopped, so a channel whose neutral is not zero needs its own safe value") {
  // A servo speed controller is neutral at a 1500 microsecond pulse. A pulse of zero is no signal at all, and what a controller does with that is its own business.
  std::vector<Channel> channels = {ch("esc_pulse_us", 1500.0, FailPolicy::zero), ch("drive_left", 0.0, FailPolicy::zero)};
  const std::vector<double> last = {1700.0, 0.8};
  const std::vector<std::string> unsafe = unsafe_on_failure(channels, last);
  std::cout << "  an ESC pulse channel (neutral 1500 us) on zero: unsafe: " << joined(unsafe) << "\n";
  RC_CHECK(unsafe.size() == 1);
  if (unsafe.size() != 1) return;
  RC_CHECK(unsafe[0] == "esc_pulse_us");
  channels[0].on_failure = FailPolicy::safe_value;
  const std::vector<std::string> after = unsafe_on_failure(channels, last);
  std::cout << "  with its own safe value: unsafe: " << joined(after) << " (the pulse went to " << output_on_failure(channels[0], 1700.0) << ")\n\n";
  RC_CHECK(after.empty());
}

RC_TEST("a brake that is applied by a spring is engaged by a cut wire, a lost supply and a dead controller; one applied by power is engaged by none of them") {
  // The robot is moving, so the brake is released: a spring applied brake has 24 V in its coil, and a power applied one has nothing.
  struct Case { const char* what; double coil; bool intact; };
  const Case spring_cases[] = {
      {"supply lost", 0.0, true},
      {"wire cut", 24.0, false},
      {"controller dead, output stage drives zero", 0.0, true},
      {"controller dead, output stage holds its last command", 24.0, true},
  };
  const Case power_cases[] = {
      {"supply lost", 0.0, true},
      {"wire cut", 0.0, false},
      {"controller dead, output stage drives zero", 0.0, true},
      {"controller dead, output stage holds its last command", 0.0, true},
  };
  int spring_safe = 0;
  int power_safe = 0;
  std::cout << "  moving, then:                                               spring applied   power applied\n";
  for (int i = 0; i < 4; ++i) {
    const bool s = brake_engaged(BrakeDesign::spring_applied, spring_cases[i].coil, spring_cases[i].intact);
    const bool p = brake_engaged(BrakeDesign::power_applied, power_cases[i].coil, power_cases[i].intact);
    if (s) ++spring_safe;
    if (p) ++power_safe;
    std::cout << "  " << spring_cases[i].what;
    for (std::size_t k = std::string(spring_cases[i].what).size(); k < 58; ++k) std::cout << ' ';
    std::cout << (s ? "engaged" : "RELEASED") << "          " << (p ? "engaged" : "RELEASED") << "\n";
  }
  std::cout << "  engaged: spring applied " << spring_safe << " of 4, power applied " << power_safe << " of 4\n\n";
  RC_CHECK(spring_safe == 3);
  RC_CHECK(power_safe == 0);
  RC_CHECK(brake_engaged(BrakeDesign::spring_applied, 11.9, true));         // below the release voltage
  RC_CHECK(!brake_engaged(BrakeDesign::spring_applied, 12.0, true));
  RC_CHECK(brake_engaged(BrakeDesign::power_applied, 24.0, true));
  RC_CHECK(!brake_engaged(BrakeDesign::power_applied, 24.0, false));
}

RC_TEST("the one case a spring does not cover is the output stage that holds, and a safe-value policy covers it") {
  // The brake coil as a channel: the coil must be at 0 V (spring applies) for the machine to be safe.
  Channel coil = ch("brake_coil", 0.0, FailPolicy::hold);
  const double held = output_on_failure(coil, 24.0);
  RC_CHECK(brake_engaged(BrakeDesign::spring_applied, held, true) == false);
  coil.on_failure = FailPolicy::safe_value;
  const double fixed = output_on_failure(coil, 24.0);
  RC_CHECK(brake_engaged(BrakeDesign::spring_applied, fixed, true));
  std::cout << "  hold: coil stays at " << held << " V, brake released; safe value: coil goes to " << fixed << " V, brake engaged: 4 of 4 now\n\n";
}
