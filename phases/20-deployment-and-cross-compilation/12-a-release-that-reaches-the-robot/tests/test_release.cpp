#include <rc/test/rc_test.hpp>
#include <rc/deploy/rollout.hpp>

#include <cstdint>
#include <iomanip>
#include <iostream>

#include "solution.hpp"

namespace {

using rc::deploy::FileData;

std::vector<std::string> chunks(const char* tag, int n = 8) {
  std::vector<std::string> v;
  for (int i = 0; i < n; ++i) v.push_back(std::string(tag) + "-chunk-" + std::to_string(i));
  return v;
}

// A robot of the aarch64 kind, running 1.4.2 from slot 0, with a floor of 1.3.0.
RobotState robot_142() {
  RobotState r;
  r.triple = "aarch64-linux-gnu";
  r.installed_version = "1.4.2";
  r.minimum_version = "1.3.0";
  r.disk.slot[0].chunks = chunks("old");
  r.disk.slot[0].version = "1.4.2";
  r.disk.slot[0].stored_hash = rc::deploy::image_hash(r.disk.slot[0].chunks);
  r.disk.boot_pointer = 0;
  r.board.cores = 4.0;
  r.board.ram_mb = 4096.0;
  r.board.flash_mb = 16384.0;
  r.data_mb = 2000.0;
  r.log_mb = 200.0;
  return r;
}

Release release(const char* version, const char* triple = "aarch64-linux-gnu") {
  Release r;
  r.version = version;
  r.triple = triple;
  r.files = {{"bin/robot_node", std::string("node ") + version}, {"etc/robot.conf", "speed = 1.5\n"}};
  r.manifest_text = rc::deploy::make_manifest(version, r.files).text;
  r.image_chunks = chunks(version);
  r.size_mb = 3000.0;
  r.behaviour.healthy_at = 20.0;
  return r;
}

std::string trace(const Outcome& o) {
  std::string s;
  for (const StageResult& g : o.stages) s += (s.empty() ? "" : " > ") + g.stage + (g.passed ? "" : " FAILED (" + g.detail + ")");
  return s;
}

}  // namespace

RC_TEST("versions are three numbers, compared by major then minor then patch, and the rules of whether to take one are checked in order") {
  RC_CHECK(parse_version("1.4.2").valid && parse_version("1.4.2").minor == 4);
  const char* bad[] = {"", "1.4", "1.4.2.1", "1.4.x", "1..2", ".1.2", "1.4.2 ", "-1.4.2", "1.4.1234567890", "v1.4.2"};
  for (const char* b : bad) RC_CHECK(!parse_version(b).valid);
  RC_CHECK(compare(parse_version("1.10.0"), parse_version("1.9.0")) == 1);                  // 10 is more than 9, which a text comparison gets wrong
  RC_CHECK(compare(parse_version("1.4.2"), parse_version("1.4.2")) == 0);
  RC_CHECK(compare(parse_version("0.9.9"), parse_version("1.0.0")) == -1);
  RC_CHECK("1.10.0" < std::string("1.9.0"));                                                  // as text, 1.10.0 sorts before 1.9.0
  RC_CHECK(decide("1.4.2", "1.5.0", "1.3.0", false) == VersionDecision::install);
  RC_CHECK(decide("1.4.2", "1.4.2", "1.3.0", false) == VersionDecision::refuse_same);
  RC_CHECK(decide("1.4.2", "1.4.0", "1.3.0", false) == VersionDecision::refuse_older);
  RC_CHECK(decide("1.4.2", "1.4.0", "1.3.0", true) == VersionDecision::install);          // a deliberate rollback
  RC_CHECK(decide("1.4.2", "1.2.0", "1.3.0", true) == VersionDecision::refuse_below_minimum);   // the floor cannot be authorised away
  RC_CHECK(decide("1.4.2", "1.2.0", "1.3.0", false) == VersionDecision::refuse_below_minimum);
  RC_CHECK(decide("1.4.2", "oops", "1.3.0", false) == VersionDecision::refuse_invalid);
  RC_CHECK(decide("", "1.5.0", "1.3.0", false) == VersionDecision::refuse_invalid);
  std::cout << "  1.10.0 against 1.9.0: newer by number, and older as text; a release below the floor is refused even with a rollback authorised\n\n";
}

RC_TEST("a good release goes through every stage, is booted from the other slot, and confirms itself") {
  RobotState robot = robot_142();
  const Outcome o = deploy(robot, release("1.5.0"), -1, false);
  std::cout << "  " << trace(o) << "\n  running " << o.running_version << ", confirmed " << (o.confirmed ? "yes" : "no") << ", the old release still in slot 0: " << (rc::deploy::slot_valid(robot.disk.slot[0]) ? "yes" : "no") << "\n\n";
  RC_CHECK(o.failed_stage.empty());
  RC_CHECK(o.stages.size() == 6);
  RC_CHECK(o.running_version == "1.5.0" && o.confirmed && o.up && !o.rolled_back && !o.bricked);
  RC_CHECK(robot.installed_version == "1.5.0");
  RC_CHECK(rc::deploy::boot(robot.disk).slot == 1 && rc::deploy::slot_valid(robot.disk.slot[0]));
}

RC_TEST("a release for the wrong machine, a damaged one, an older one and an oversize one are each refused before the disk is touched") {
  struct Case { const char* what; Release rel; bool rollback; const char* stage; const char* detail; };
  Release wrong = release("1.5.0", "arm-linux-gnueabihf");
  Release truncated = release("1.5.0");
  truncated.manifest_text = truncated.manifest_text.substr(0, truncated.manifest_text.rfind("end "));
  Release altered = release("1.5.0");
  altered.files[0].content[0] = 'X';
  Release other_version = release("1.5.0");
  other_version.manifest_text = rc::deploy::make_manifest("1.5.1", other_version.files).text;
  Release older = release("1.4.0");
  Release floor = release("1.2.0");
  Release same = release("1.4.2");
  Release big = release("1.5.0");
  big.size_mb = 6400.0;
  const Case cases[] = {{"built for 32 bit ARM", wrong, false, "target", "built for arm-linux-gnueabihf, robot is aarch64-linux-gnu"},
                        {"manifest cut short", truncated, false, "manifest", "no end line"},
                        {"a file altered", altered, false, "manifest", "files differ"},
                        {"manifest of another version", other_version, false, "manifest", "wrong version"},
                        {"older than installed", older, false, "version", "refused"},
                        {"below the floor, rollback authorised", floor, true, "version", "refused"},
                        {"the same version", same, false, "version", "refused"},
                        {"6400 MB, over the room for two slots", big, false, "fit", "too big"}};
  std::cout << "  release                                      refused at\n";
  for (const Case& c : cases) {
    RobotState robot = robot_142();
    const RobotState untouched = robot_142();
    const Outcome o = deploy(robot, c.rel, -1, c.rollback);
    RC_CHECK(!o.stages.empty());
    if (o.stages.empty()) return;
    std::cout << "  " << c.what << std::string(44 - std::string(c.what).size(), ' ') << o.failed_stage << " (" << o.stages.back().detail << ")\n";
    RC_CHECK(o.failed_stage == c.stage);
    RC_CHECK(o.stages.back().detail == c.detail);
    RC_CHECK(o.running_version == "1.4.2" && !o.bricked);
    RC_CHECK(robot.disk.boot_pointer == untouched.disk.boot_pointer && robot.disk.slot[1].chunks.empty());           // nothing was written
    RC_CHECK(robot.installed_version == "1.4.2");
  }
  std::cout << "\n";
  RobotState rb = robot_142();
  RC_CHECK(deploy(rb, release("1.4.0"), -1, true).failed_stage.empty());                      // a rollback somebody authorised, above the floor
  RC_CHECK(rb.installed_version == "1.4.0");
}

RC_TEST("power lost at each of the ten steps of the install, the robot always boots, and run again each one ends on the new release") {
  const Release rel = release("1.5.0");
  int old_release = 0;
  int bricked = 0;
  int retried_ok = 0;
  for (int k = 0; k < 10; ++k) {
    RobotState robot = robot_142();
    const Outcome cut = deploy(robot, rel, k, false);
    if (cut.bricked) ++bricked;
    if (cut.running_version == "1.4.2") ++old_release;
    RC_CHECK(cut.failed_stage == "install");
    const Outcome again = deploy(robot, rel, -1, false);
    if (again.failed_stage.empty() && again.running_version == "1.5.0" && again.confirmed) ++retried_ok;
  }
  std::cout << "  cut after 0 to 9 steps of the ten: " << old_release << " of 10 boot release 1.4.2, " << bricked << " bricked; run again: " << retried_ok << " of 10 end on 1.5.0 and confirmed\n\n";
  RC_CHECK(bricked == 0);
  RC_CHECK(old_release == 10);
  RC_CHECK(retried_ok == 10);
}

RC_TEST("a release that crashes is put on trial and taken back by the robot, and one that fails only after the soak is not, which is the limit of the trial") {
  RobotState a = robot_142();
  Release crashes = release("1.5.0");
  crashes.behaviour.crash_at = 2.0;
  crashes.behaviour.healthy_at = -1.0;
  const Outcome oa = deploy(a, crashes, -1, false);
  RobotState b = robot_142();
  Release late = release("1.5.0");
  late.behaviour.healthy_at = 20.0;
  late.behaviour.crash_at = 90.0;                                                            // healthy for 90 s, then crashes, every boot
  const Outcome ob = deploy(b, late, -1, false);
  std::cout << "  crashes at 2 s:           " << trace(oa) << "; running " << oa.running_version << ", up " << (oa.up ? "yes" : "no") << "\n";
  std::cout << "  healthy, crashes at 90 s:  " << trace(ob) << "; running " << ob.running_version << ", confirmed " << (ob.confirmed ? "yes" : "no") << ", up " << (ob.up ? "yes" : "NO") << "\n\n";
  RC_CHECK(oa.failed_stage == "trial" && oa.rolled_back && oa.running_version == "1.4.2" && oa.up && !oa.bricked);
  RC_CHECK(a.installed_version == "1.4.2" && a.disk.boot_pointer == 0);
  RC_CHECK(rc::deploy::slot_valid(a.disk.slot[0]) && rc::deploy::slot_valid(a.disk.slot[1]));
  RC_CHECK(ob.failed_stage.empty() && ob.running_version == "1.5.0" && ob.confirmed);        // 20 + 30 is 50 s: it is confirmed before it crashes
  RC_CHECK(!ob.up);
}

RC_TEST("a release with a fault on three robots in ten is stopped by the first wave, none of the robots it touched is bricked, and the one that failed is back on the old release") {
  const int n = 1000;
  std::vector<bool> variant(static_cast<std::size_t>(n), false);
  for (int i = 0; i < 300; ++i) variant[static_cast<std::size_t>(i)] = true;
  std::uint64_t s = 99;
  for (std::size_t i = variant.size() - 1; i > 0; --i) {
    s = s * 6364136223846793005ULL + 1442695040888963407ULL;
    const std::size_t j = static_cast<std::size_t>((s >> 33) % (i + 1));
    const bool t = variant[i];
    variant[i] = variant[j];
    variant[j] = t;
  }
  auto release_for = [&variant](std::size_t robot) {
    Release r = release("1.5.0");
    if (variant[robot]) {
      r.behaviour.crash_at = 2.0;
      r.behaviour.healthy_at = -1.0;
    }
    return r;
  };
  std::vector<rc::deploy::FleetRobot> fleet(static_cast<std::size_t>(n));
  for (std::size_t i = 0; i < fleet.size(); ++i) {
    RobotState probe = robot_142();
    fleet[i].fails = deploy(probe, release_for(i), -1, false).rolled_back;
  }
  rc::deploy::RolloutPolicy policy;
  policy.cumulative = {0.01, 0.1, 0.5, 1.0};
  policy.max_failure_ratio = 0.05;
  policy.min_reported_ratio = 0.8;
  const rc::deploy::RolloutResult plan = rc::deploy::rollout(fleet, policy);
  int touched = 0;
  int back_on_old = 0;
  int on_new = 0;
  int bricked = 0;
  for (int i = 0; i < plan.attempted; ++i) {
    RobotState robot = robot_142();
    const Outcome o = deploy(robot, release_for(static_cast<std::size_t>(i)), -1, false);
    ++touched;
    if (o.bricked) ++bricked;
    if (o.running_version == "1.4.2") ++back_on_old;
    if (o.running_version == "1.5.0") ++on_new;
  }
  int all_at_once_fail = 0;
  for (int i = 0; i < n; ++i)
    if (fleet[static_cast<std::size_t>(i)].fails) ++all_at_once_fail;
  std::cout << "  a release that crashes on 300 of 1000 robots: given to everyone, " << all_at_once_fail << " go through a trial and roll back; staged, the rollout " << (plan.halted ? "halted" : "did not halt") << " at wave " << plan.halted_at_wave << " after "
            << plan.attempted << " robots\n  of those " << touched << ": " << on_new << " are on 1.5.0, " << back_on_old << " back on 1.4.2, " << bricked << " bricked\n\n";
  RC_CHECK(all_at_once_fail == 300);
  RC_CHECK(plan.halted && plan.halted_at_wave == 0 && plan.attempted == 10);
  RC_CHECK(bricked == 0);
  RC_CHECK(on_new + back_on_old == touched);
  RC_CHECK(back_on_old == plan.failed);
}
