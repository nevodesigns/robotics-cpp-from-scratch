#include <rc/test/rc_test.hpp>
#include <sstream>
#include <cstring>

#include <cmath>
#include <iostream>

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
    {"horizon", 106.02540378443865},
    {"d_centre", 1.1196152422706633},
    {"d_140", 4.651666049839543},
    {"d_120", 11.424027461326377},
    {"step_centre", 0.008957046693732096},
    {"step_140", 0.1393122054469682},
    {"step_120", 0.8242927604408283},
    {"pitch_err", 37.26281457032978},
    {"height_err", 10.000000000000023},
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


namespace {

const double kPi = 3.14159265358979323846;
const double kH = 0.30;
const double kPitch = 15.0 * kPi / 180.0;

double ahead_of(double v, double h = kH, double pitch = kPitch) {
  Camera cam;
  double x = 0.0, y = 0.0;
  if (!pixel_to_ground(cam, h, pitch, cam.cx, v, x, y)) return -1.0;
  return x;
}

}  // namespace

RC_TEST("a pixel looks at a point of the ground, and the point falls back on the same pixel") {
  Camera cam;
  for (int row = 130; row <= 470; row += 40) {
    for (int col = 20; col <= 620; col += 150) {
      double x = 0.0, y = 0.0, u = 0.0, v = 0.0;
      RC_CHECK(pixel_to_ground(cam, kH, kPitch, col, row, x, y));
      RC_CHECK(ground_to_pixel(cam, kH, kPitch, x, y, u, v));
      RC_CHECK_NEAR(u, col, 1e-6);
      RC_CHECK_NEAR(v, row, 1e-6);
    }
  }
  // The left of the image is the left of the vehicle.
  double xl = 0.0, yl = 0.0;
  RC_CHECK(pixel_to_ground(cam, kH, kPitch, 100.0, 300.0, xl, yl));
  RC_CHECK(yl > 0.0);
}

RC_TEST("the horizon is above the centre of the image by the focal length times the tangent of the pitch, and nothing above it is ground") {
  Camera cam;
  const double hr = horizon_row(cam, kPitch);
  rec("horizon", hr, 0);
  RC_CHECK_NEAR(hr, 240.0 - 500.0 * std::tan(kPitch), 1e-9);
  double x = 0.0, y = 0.0;
  RC_CHECK(!pixel_to_ground(cam, kH, kPitch, 320.0, hr - 1.0, x, y));
  RC_CHECK(!pixel_to_ground(cam, kH, kPitch, 320.0, 10.0, x, y));
  RC_CHECK(pixel_to_ground(cam, kH, kPitch, 320.0, hr + 1.0, x, y));
  RC_CHECK(x > 20.0);
}

RC_TEST("the distance of a row is the height over the tangent of the angle it looks down by") {
  rec("d_centre", ahead_of(240.0), 2);
  rec("d_140", ahead_of(140.0), 2);
  rec("d_120", ahead_of(120.0), 2);
  RC_CHECK_NEAR(ahead_of(240.0), kH / std::tan(kPitch), 1e-9);
  const double depression_140 = kPitch - std::atan(100.0 / 500.0);
  RC_CHECK_NEAR(ahead_of(140.0), kH / std::tan(depression_140), 1e-9);
  RC_CHECK(ahead_of(400.0) < ahead_of(240.0) && ahead_of(240.0) < ahead_of(140.0) && ahead_of(140.0) < ahead_of(120.0));
}

RC_TEST("one row spans more ground the further away it looks, and nearly all of it at the horizon") {
  Camera cam;
  const double s_centre = ground_step(cam, kH, kPitch, 240.0);
  const double s_140 = ground_step(cam, kH, kPitch, 140.0);
  const double s_120 = ground_step(cam, kH, kPitch, 120.0);
  rec("step_centre", s_centre, 4);
  rec("step_140", s_140, 3);
  rec("step_120", s_120, 2);
  RC_CHECK(s_centre < s_140 && s_140 < s_120);
  RC_CHECK(s_140 > 10.0 * s_centre);
  RC_CHECK(!std::isfinite(ground_step(cam, kH, kPitch, 100.0)));
  // The step grows as the square of the range: doubling the range quadruples it.
  const double r1 = ahead_of(200.0), r2 = ahead_of(150.0);
  const double ratio = ground_step(cam, kH, kPitch, 150.0) / ground_step(cam, kH, kPitch, 200.0);
  RC_CHECK_NEAR(ratio, (r2 * r2) / (r1 * r1), 0.25 * (r2 * r2) / (r1 * r1));
}

RC_TEST("a pitch a degree wrong moves the far ground a lot, and a height 10 percent wrong moves it by 10 percent") {
  const double truth = ahead_of(140.0);
  const double wrong_pitch = ahead_of(140.0, kH, 14.0 * kPi / 180.0);
  const double wrong_height = ahead_of(140.0, 0.33, kPitch);
  const double pitch_err = 100.0 * std::fabs(wrong_pitch - truth) / truth;
  const double height_err = 100.0 * std::fabs(wrong_height - truth) / truth;
  rec("pitch_err", pitch_err, 0);
  rec("height_err", height_err, 0);
  std::cout << "  row 140 is " << truth << " m ahead; with a pitch of 14 degrees it would be taken for " << wrong_pitch << " m, with a height of 0.33 m " << wrong_height << " m\n";
  RC_CHECK(pitch_err > 25.0);
  RC_CHECK_NEAR(height_err, 10.0, 1e-9);
  // Near the camera the pitch matters much less.
  const double near_truth = ahead_of(400.0);
  const double near_wrong = ahead_of(400.0, kH, 14.0 * kPi / 180.0);
  RC_CHECK(100.0 * std::fabs(near_wrong - near_truth) / near_truth < 10.0);
}
