#include <rc/test/rc_test.hpp>

#include <iostream>

#include "solution.hpp"

namespace {

std::vector<std::string> image(const char* tag, int chunks = 8) {
  std::vector<std::string> v;
  for (int i = 0; i < chunks; ++i) v.push_back(std::string(tag) + "-chunk-" + std::to_string(i));
  return v;
}

// A robot running release 1 from slot 0, with nothing in slot 1.
Disk running_v1() {
  Disk d;
  d.slot[0].chunks = image("v1");
  d.slot[0].version = "1";
  d.slot[0].stored_hash = image_hash(d.slot[0].chunks);
  d.boot_pointer = 0;
  return d;
}

struct Outcome {
  int old_release = 0;
  int new_release = 0;
  int bricked = 0;
};

// Install, losing power after each possible number of steps (0 to all of them), and see what boots.
Outcome cut_everywhere(const std::vector<WriteStep>& steps) {
  Outcome o;
  for (std::size_t k = 0; k <= steps.size(); ++k) {
    Disk d = running_v1();
    for (std::size_t i = 0; i < k; ++i) apply(d, steps[i]);
    const Boot b = boot(d);
    if (!b.ok) ++o.bricked;
    else if (b.version == "1") ++o.old_release;
    else ++o.new_release;
  }
  return o;
}

}  // namespace

RC_TEST("a slot is valid when its hash matches what it holds, and the robot boots the pointer's slot, then the other, then nothing") {
  Disk d = running_v1();
  RC_CHECK(slot_valid(d.slot[0]));
  RC_CHECK(!slot_valid(d.slot[1]));
  RC_CHECK(boot(d).ok && boot(d).slot == 0 && boot(d).version == "1");
  d.boot_pointer = 1;                                 // the pointer names an empty slot
  RC_CHECK(boot(d).ok && boot(d).slot == 0);          // so the other one boots
  d.slot[0].chunks[3] = "damaged";
  RC_CHECK(!slot_valid(d.slot[0]));
  const Boot none = boot(d);
  RC_CHECK(!none.ok && none.slot == -1 && none.version.empty());
  d.boot_pointer = 7;
  RC_CHECK(!boot(d).ok);
  Disk e = running_v1();
  e.boot_pointer = 9;
  RC_CHECK(boot(e).ok && boot(e).slot == 0);          // a pointer out of range is read as 0
  RC_CHECK(!slot_valid(Slot()));                      // an empty slot is not valid, though 0 might equal its hash
}

RC_TEST("overwriting the running release leaves it bricked at every power cut in the middle, and A/B never does") {
  const Disk d = running_v1();
  const std::vector<WriteStep> naive = plan_in_place(d, image("v2"), "2");
  const std::vector<WriteStep> ab = plan_ab(d, image("v2"), "2");
  const Outcome n = cut_everywhere(naive);
  const Outcome a = cut_everywhere(ab);
  std::cout << "  installing release 2 over release 1, 8 chunks, power lost after each possible step:\n";
  std::cout << "  overwrite in place (" << naive.size() << " steps): " << n.old_release << " cuts boot release 1, " << n.new_release << " boot release 2, " << n.bricked << " BRICKED\n";
  std::cout << "  two slots and a commit (" << ab.size() << " steps): " << a.old_release << " cuts boot release 1, " << a.new_release << " boot release 2, " << a.bricked << " bricked\n\n";
  RC_CHECK(naive.size() == 9);
  RC_CHECK(ab.size() == 10);
  RC_CHECK(n.old_release == 1 && n.new_release == 1 && n.bricked == 8);
  RC_CHECK(a.old_release == 10 && a.new_release == 1 && a.bricked == 0);
}

RC_TEST("the commit is the hash and the move of the pointer is last: a cut between them boots the old release, and only the whole boots the new") {
  const Disk start = running_v1();
  const std::vector<WriteStep> ab = plan_ab(start, image("v2"), "2");
  RC_CHECK(ab.size() == 10);
  if (ab.size() != 10) return;
  RC_CHECK(ab[0].kind == StepKind::write_chunk && ab[0].slot == 1);                // the slot that is not running
  RC_CHECK(ab[8].kind == StepKind::write_hash && ab[8].slot == 1);
  RC_CHECK(ab[9].kind == StepKind::switch_pointer && ab[9].slot == 1);
  Disk d = start;
  for (int i = 0; i < 9; ++i) apply(d, ab[static_cast<std::size_t>(i)]);
  RC_CHECK(slot_valid(d.slot[1]));                                                 // the new release is complete and checked
  RC_CHECK(boot(d).version == "1");                                                // and still not booted
  apply(d, ab[9]);
  RC_CHECK(boot(d).version == "2" && boot(d).slot == 1);
  RC_CHECK(slot_valid(d.slot[0]));                                                 // release 1 is untouched, for a way back
  std::cout << "  after nine of ten steps the new release is complete and checked and the robot still boots release 1; after the tenth it boots release 2, and release 1 is still there\n\n";
}

RC_TEST("an install that was cut can simply be run again, from the start, and finishes whatever step it was cut at") {
  int fine = 0;
  const Disk start = running_v1();
  const std::vector<WriteStep> first = plan_ab(start, image("v2"), "2");
  for (std::size_t k = 0; k <= first.size(); ++k) {
    Disk d = start;
    for (std::size_t i = 0; i < k; ++i) apply(d, first[i]);
    const Boot before = boot(d);
    RC_CHECK(before.ok);
    if (before.version == "2") {                                                  // it had finished
      ++fine;
      continue;
    }
    for (const WriteStep& s : plan_ab(d, image("v2"), "2")) apply(d, s);               // run again
    const Boot after = boot(d);
    if (after.ok && after.version == "2") ++fine;
  }
  std::cout << "  an install cut at each of 11 points and then run again: " << fine << " of 11 ended on release 2\n\n";
  RC_CHECK(fine == 11);
}

RC_TEST("release after release alternates the slots, a bad image falls back to the one before, and a robot with no good slot reports it") {
  Disk d = running_v1();
  const char* tags[] = {"v2", "v3", "v4", "v5"};
  const char* versions[] = {"2", "3", "4", "5"};
  std::cout << "  slots used by releases 2, 3, 4, 5:";
  for (int i = 0; i < 4; ++i) {
    for (const WriteStep& s : plan_ab(d, image(tags[i]), versions[i])) apply(d, s);
    std::cout << " " << boot(d).slot;
    RC_CHECK(boot(d).version == versions[i]);
  }
  std::cout << "\n";
  RC_CHECK(boot(d).slot == 0);                                                     // 1, 0, 1, 0: release 5 is in slot 0, and release 4 is in slot 1 to go back to
  RC_CHECK(slot_valid(d.slot[1]) && d.slot[1].version == "4");
  // A release whose image was damaged as it was written: a chunk in the new slot is wrong after the commit.
  Disk e = running_v1();
  for (const WriteStep& s : plan_ab(e, image("v2"), "2")) apply(e, s);
  RC_CHECK(e.slot[1].chunks.size() == 8);
  if (e.slot[1].chunks.size() != 8) return;
  e.slot[1].chunks[5] = "bit rot";
  const Boot fell_back = boot(e);
  std::cout << "  a chunk of release 2 damaged after the switch: the robot boots release " << fell_back.version << " from slot " << fell_back.slot << "\n";
  RC_CHECK(fell_back.ok && fell_back.version == "1" && fell_back.slot == 0);
  e.slot[0].chunks[0] = "also damaged";
  RC_CHECK(!boot(e).ok);
  std::cout << "  and with release 1 damaged too, nothing boots: it is reported, not guessed\n\n";
  Disk empty;
  const std::vector<WriteStep> recovery = plan_ab(empty, image("v1"), "1");
  RC_CHECK(recovery.size() == 10 && recovery[0].slot == 0);                        // with nothing booting, slot 0
  for (const WriteStep& s : recovery) apply(empty, s);
  RC_CHECK(boot(empty).ok && boot(empty).version == "1");
}
