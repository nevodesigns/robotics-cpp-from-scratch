#include <rc/test/rc_test.hpp>

#include <cmath>

#include "solution.hpp"
#include <iomanip>
#include <iostream>

static void rec(const char* key, double value, int digits = 3) {
  std::cout << "@" << key << " " << std::fixed << std::setprecision(digits) << value << std::endl;
  std::cout.unsetf(std::ios::floatfield);
}
[[maybe_unused]] static void recs(const char* key, const std::string& value) { std::cout << "@" << key << " " << value << std::endl; }


RC_TEST("the push to bring the peg in is zero inside the clearance and grows with the stiffness and the offset") {
  PegHole h;
  RC_CHECK_NEAR(push_to_centre(h, 0.00004, 1000.0), 0.0, 1e-12);
  const double a = push_to_centre(h, 0.002, 1000.0);
  RC_CHECK_NEAR(a, 1000.0 * (0.002 - 0.00005) / (1.0 - 0.3), 1e-6);
  RC_CHECK_NEAR(push_to_centre(h, 0.002, 2000.0), 2.0 * a, 1e-6);
  RC_CHECK(push_to_centre(h, 0.003, 1000.0) > a);
  RC_CHECK_NEAR(push_to_centre(h, -0.002, 1000.0), a, 1e-9);
}

RC_TEST("a soft hand brings the peg in with a small push where a stiff one jams, and a peg beyond the chamfer is missed") {
  PegHole h;
  const double off = 0.002, fmax = 50.0;
  const double ks = 300.0, km = 2000.0, kr = 100000.0;
  rec("cl_um", h.clearance * 1e6, 0);
  rec("cw_mm", h.chamfer_w * 1000.0, 0);
  rec("off_mm", off * 1000.0, 0);
  rec("k_soft", ks, 0);
  rec("k_mid", km, 0);
  rec("k_stiff", kr, 0);
  rec("f_soft", push_to_centre(h, off, ks), 1);
  rec("f_mid", push_to_centre(h, off, km), 1);
  rec("f_stiff", push_to_centre(h, off, kr), 0);
  rec("fmax", fmax, 0);
  RC_CHECK(insert(h, off, ks, fmax) == kInserted);
  RC_CHECK(insert(h, off, km, fmax) == kInserted);
  RC_CHECK(insert(h, off, kr, fmax) == kJammed);
  RC_CHECK(insert(h, 0.0035, ks, fmax) == kMissed);
  RC_CHECK(insert(h, 0.00003, kr, fmax) == kInserted);
}

RC_TEST("a peg on a slope not steeper than the friction cannot slide, however soft the hand") {
  PegHole h;
  h.mu = 1.2;
  rec("mu_hi", h.mu, 1);
  rec("slide_hi", can_slide(h) ? 1 : 0, 0);
  RC_CHECK(!can_slide(h));
  RC_CHECK(insert(h, 0.002, 1.0, 1000.0) == kJammed);
  PegHole g;
  RC_CHECK(can_slide(g));
}

RC_TEST("over a range of offsets the soft hand gets the peg in more often than the stiff one") {
  PegHole h;
  int ok_soft = 0, ok_stiff = 0, n = 0;
  for (double e = 0.0; e <= 0.004 + 1e-12; e += 0.00025) {
    ++n;
    if (insert(h, e, 300.0, 50.0) == kInserted) ++ok_soft;
    if (insert(h, e, 100000.0, 50.0) == kInserted) ++ok_stiff;
  }
  rec("n_off", n, 0);
  rec("ok_soft", ok_soft, 0);
  rec("ok_stiff", ok_stiff, 0);
  RC_CHECK(ok_soft > ok_stiff);
  RC_CHECK(ok_stiff >= 1);
  RC_CHECK(ok_soft < n);
}
