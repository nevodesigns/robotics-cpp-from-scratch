#include <rc/test/rc_test.hpp>
#include <sstream>
#include <cstring>

#include <cmath>

#include "solution.hpp"
#include <iomanip>
#include <iostream>

// The numbers this lesson publishes: its page, its header and its quiz quote them, so each is held here as it was
// measured. A change that moves one by more than half a unit of its last printed digit fails the suite.
struct Pin {
  const char* key;
  double value;
};
static const Pin kPins[] = {
    {"job_e", 0.6},
    {"job_h", 0.8},
    {"need", 95.0},
    {"allow", 5.0},
    {"dg_test", 0.0002157990700743729},
    {"line_test", 0.012403614251111224},
    {"dg_success", 98.0},
    {"dg_handover", 3.171312032630863},
    {"clone_fail", 1.0},
    {"clone_test", 0.016182083104990128},
    {"clone_handover", 11.59755268524813},
    {"tight_z", 1.0},
    {"tight_fail", 3.0},
    {"tight_handover", 9.653297076818491},
    {"tiny_epochs", 2.0},
    {"tiny_fail", 0.0},
    {"tiny_test", 0.0005609215442347595},
    {"hard_e", 0.95},
    {"hard_h", 1.2},
    {"hard_fail", 2.0},
    {"hard_success", 73.0},
};

static void rec(const char* key, double value, int digits = 3) {
  std::cout << "@" << key << " " << std::fixed << std::setprecision(digits) << value << std::endl;
  std::cout.unsetf(std::ios::floatfield);
  for (const Pin& pin : kPins) {
    if (std::strcmp(pin.key, key) != 0) continue;
    double allowed = 0.5;
    for (int i = 0; i < digits; ++i) allowed /= 10.0;
    if (!(std::fabs(value - pin.value) <= allowed)) {
      std::ostringstream message;
      message << "the published number " << key << " was " << pin.value << " and is now " << value;
      rc::test::record(__FILE__, __LINE__, message.str());
    }
  }
}
[[maybe_unused]] static void recs(const char* key, const std::string& value) { std::cout << "@" << key << " " << value << std::endl; }


static std::vector<Sample> narrow_demos() { return demonstrations(LaneTask(), 30, 100, 0.2, 0.1, 0.0, 0.0, 21); }

// Train and test episodes for the line comparison: a consistent expert with some noise, from the job's starts.
static void held_out(std::vector<Sample>& train, std::vector<Sample>& test) {
  const std::vector<Sample> d = demonstrations(LaneTask(), 40, 100, 0.6, 0.8, 0.05, 0.0, 22);
  split_by_episode(d, 5, train, test);
}

// Built once and kept: training it is the slow part of this suite.
static Candidate build_dagger_candidate() {
  const LaneTask t;
  std::vector<Sample> data = narrow_demos();
  Learner l = clone(data, 16, 200, 0.02, 4);
  // two rounds of DAgger, keeping the data so the release can train it again
  std::vector<Sample> all = data;
  for (int k = 1; k <= 2; ++k) {
    const std::vector<Sample> extra = collect_on_learner(t, l, 0.6, 0.8, 20, 100, 1000 * k, 500 + static_cast<unsigned long long>(k));
    all.insert(all.end(), extra.begin(), extra.end());
    l = clone(all, 16, 100, 0.02, 4);
  }
  Candidate c;
  c.learner = l;
  c.guard.seen = l.scale;
  c.data = all;
  return c;
}

static const Candidate& dagger_candidate() {
  static const Candidate c = build_dagger_candidate();
  return c;
}

RC_TEST("a policy trained with DAgger, behind its guard, passes all six requirements and is released") {
  const LaneTask t;
  PolicyJob j;
  std::vector<Sample> train, test;
  held_out(train, test);
  const Candidate c = dagger_candidate();
  const PolicyVerdict v = check_policy(t, j, c, train, test);
  rec("job_e", j.offset0, 1);
  rec("job_h", j.heading0, 1);
  rec("need", j.need_success * 100.0, 0);
  rec("allow", j.max_handover * 100.0, 0);
  rec("dg_test", v.test_error, 5);
  rec("line_test", v.line_error, 5);
  rec("dg_success", v.success * 100.0, 0);
  rec("dg_handover", v.handover * 100.0, 1);
  RC_CHECK(v.beats_line && v.success_ok && v.handover_ok && v.bad_input_ok && v.limit_ok && v.repeatable);
  RC_CHECK(v.released && v.first_fail == 0);
}

RC_TEST("the plain clone fails the first requirement on the job's episodes, a guard set too tight fails the third, and one well-shaped unit is enough") {
  const LaneTask t;
  PolicyJob j;
  std::vector<Sample> train, test;
  held_out(train, test);
  const std::vector<Sample> d = narrow_demos();
  Candidate c;
  c.data = d;
  c.epochs = 200;
  c.learner = clone(d, 16, 200, 0.02, 4);
  c.guard.seen = c.learner.scale;
  const PolicyVerdict v = check_policy(t, j, c, train, test);
  rec("clone_fail", v.first_fail, 0);
  rec("clone_test", v.test_error, 5);
  rec("clone_handover", v.handover * 100.0, 0);
  RC_CHECK(!v.released && v.first_fail == 1);
  RC_CHECK(v.handover > j.max_handover);   // and it would have failed the third as well
  // the good policy behind a guard set too tight
  Candidate tight = dagger_candidate();
  tight.guard.z_max = 1.0;
  const PolicyVerdict w = check_policy(t, j, tight, train, test);
  rec("tight_z", tight.guard.z_max, 0);
  rec("tight_fail", w.first_fail, 0);
  rec("tight_handover", w.handover * 100.0, 0);
  RC_CHECK(w.first_fail == 3);
  // one hidden unit, two epochs: one tanh is close to the expert's clipped line, and it passes
  Candidate tiny = dagger_candidate();
  tiny.hidden = 1;
  tiny.epochs = 2;
  tiny.learner = clone(tiny.data, 1, 2, 0.02, 4);
  const PolicyVerdict u = check_policy(t, j, tiny, train, test);
  rec("tiny_epochs", tiny.epochs, 0);
  rec("tiny_fail", u.first_fail, 0);
  rec("tiny_test", u.test_error, 5);
  RC_CHECK(u.released);
}

RC_TEST("a job of starts no policy can manage fails on success, and the verdict keeps every figure") {
  const LaneTask t;
  PolicyJob j;
  j.offset0 = 0.95;
  j.heading0 = 1.2;
  j.max_handover = 1.0;
  std::vector<Sample> train, test;
  held_out(train, test);
  const Candidate c = dagger_candidate();
  const PolicyVerdict v = check_policy(t, j, c, train, test);
  rec("hard_e", j.offset0, 2);
  rec("hard_h", j.heading0, 1);
  rec("hard_fail", v.first_fail, 0);
  rec("hard_success", v.success * 100.0, 0);
  RC_CHECK(v.first_fail == 2);
  RC_CHECK(v.beats_line);   // it is a good policy; the job is too hard
  RC_CHECK(v.largest_turn <= t.max_turn);
}
