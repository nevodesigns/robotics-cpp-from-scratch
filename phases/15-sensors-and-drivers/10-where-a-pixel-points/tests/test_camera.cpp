#include <rc/test/rc_test.hpp>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <vector>

#include "solution.hpp"

namespace {

// A wide lens: 640 by 480 pixels, a focal length of 500 pixels, and a barrel distortion of the
// size a cheap wide angle camera has.
Intrinsics wide_lens() {
  Intrinsics k;
  k.k1 = -0.25;
  k.k2 = 0.05;
  return k;
}

// The distance from the centre of the picture, in pixels, of a point at normalised radius r,
// worked out by hand from the model and not from the code under test.
double distorted_pixel_radius(const Intrinsics& k, double r) {
  const double scale = 1.0 + k.k1 * r * r + k.k2 * r * r * r * r;
  return 500.0 * r * scale;
}

}  // namespace

RC_TEST("a point in front of the camera lands where the similar triangles say") {
  const Intrinsics k;   // no distortion
  bool visible = false;

  const Pixel p = project(k, 0.5, 0.2, 2.0, visible);
  RC_REQUIRE(visible);
  RC_CHECK_NEAR(p.u, 500.0 * 0.25 + 320.0, 1e-9);   // 445
  RC_CHECK_NEAR(p.v, 500.0 * 0.10 + 240.0, 1e-9);   // 290

  // On the axis it is at the centre, at any depth.
  const Pixel centre = project(k, 0.0, 0.0, 7.0, visible);
  RC_CHECK_NEAR(centre.u, 320.0, 1e-12);
  RC_CHECK_NEAR(centre.v, 240.0, 1e-12);

  // Twice as far away is half as far from the centre of the picture.
  const Pixel near_point = project(k, 0.4, 0.0, 1.0, visible);
  const Pixel far_point = project(k, 0.4, 0.0, 2.0, visible);
  RC_CHECK_NEAR((near_point.u - 320.0) / (far_point.u - 320.0), 2.0, 1e-12);
}

RC_TEST("a point at or behind the camera does not appear, instead of appearing mirrored") {
  const Intrinsics k;
  bool visible = true;

  project(k, 0.5, 0.2, 0.0, visible);
  RC_CHECK(!visible);
  visible = true;
  project(k, 0.5, 0.2, -2.0, visible);
  RC_CHECK(!visible);

  // The division would give a perfectly good pixel, which is the danger: a point two metres behind
  // the camera divides to the mirror image of one two metres in front.
  visible = false;
  project(k, 0.5, 0.2, 2.0, visible);
  RC_CHECK(visible);
}

RC_TEST("the lens pulls the edges in: how far a point moves, by how far it is from the centre") {
  const Intrinsics k = wide_lens();
  bool visible = false;

  std::cout << "  a barrel lens, k1 of -0.25 and k2 of 0.05, on a 640 by 480 picture. How far a point's pixel\n"
            << "  is from where a pinhole would put it:\n\n"
            << "    normalised radius   pinhole (px)   with the lens (px)   moved (px)\n";

  double corner_shift = 0.0;
  for (const double r : {0.1, 0.2, 0.4, 0.6, 0.8}) {
    const Pixel p = project(k, r * 2.0, 0.0, 2.0, visible);   // along the horizontal axis at two metres
    RC_REQUIRE(visible);

    const double pinhole = 500.0 * r;
    const double lens = distorted_pixel_radius(k, r);
    RC_CHECK_NEAR(p.u - 320.0, lens, 1e-9);
    std::cout << "    " << std::fixed << std::setprecision(2) << std::setw(10) << r << std::setprecision(1)
              << std::setw(17) << pinhole << std::setw(19) << lens << std::setw(13) << pinhole - lens << "\n";
    if (r == 0.8) corner_shift = pinhole - lens;
  }

  // At two metres a pixel is 4 mm, so the shift at the edge is in centimetres of the world.
  std::cout << "\n  Near the corner that is " << std::setprecision(0) << corner_shift
            << " pixels, which at two metres is " << std::setprecision(1) << corner_shift / 500.0 * 2.0 * 100.0
            << " cm on the floor if the lens is ignored.\n\n";

  RC_CHECK(corner_shift > 40.0);
}

RC_TEST("undoing the lens: a pixel is taken back to the direction it looked along") {
  const Intrinsics k = wide_lens();
  bool visible = false;

  // Every direction in a grid across the whole picture, projected and brought back.
  double worst = 0.0;
  int tried = 0;
  for (int i = -6; i <= 6; ++i) {
    for (int j = -5; j <= 5; ++j) {
      const double x = static_cast<double>(i) * 0.1;
      const double y = static_cast<double>(j) * 0.1;
      const Pixel p = project(k, x * 3.0, y * 3.0, 3.0, visible);
      const Direction d = unproject(k, p);
      worst = std::fmax(worst, std::hypot(d.x - x, d.y - y) * 500.0);
      ++tried;
    }
  }

  std::cout << "  " << tried << " directions across the picture, projected through the lens and brought back: the\n"
            << "  worst miss was " << std::scientific << std::setprecision(1) << worst << " pixels.\n\n";

  RC_CHECK_EQ(tried, 143);
  RC_CHECK(worst < 1e-6);

  // And with no lens at all it is just the division.
  const Intrinsics plain;
  const Direction straight = unproject(plain, Pixel{445.0, 290.0});
  RC_CHECK_NEAR(straight.x, 0.25, 1e-12);
  RC_CHECK_NEAR(straight.y, 0.10, 1e-12);
}

RC_TEST("the inverse has to be iterated, and the corner is where it needs most passes") {
  const Intrinsics k = wide_lens();
  bool visible = false;
  const double x = 0.64;
  const double y = 0.48;   // the corner of the picture, a normalised radius of 0.8
  const Pixel corner = project(k, x * 2.0, y * 2.0, 2.0, visible);

  std::cout << "  the corner pixel, taken back to its direction with n passes of the correction, and how far\n"
            << "  the answer is from the truth, in pixels:\n\n"
            << "    passes   error (px)\n";

  double errors[6];
  const int passes[] = {1, 2, 3, 5, 10, 20};
  for (int i = 0; i < 6; ++i) {
    const Direction d = unproject(k, corner, passes[i]);
    errors[i] = std::hypot(d.x - x, d.y - y) * 500.0;
    std::cout << "    " << std::setw(6) << passes[i] << "   " << std::scientific << std::setprecision(2) << errors[i]
              << "\n";
  }
  std::cout << "\n";

  // Each pass is nearer, and a single one, which is what a quick fix does, is out by pixels.
  RC_CHECK(errors[0] > 5.0);
  for (int i = 1; i < 6; ++i) RC_CHECK(errors[i] < errors[i - 1]);
  RC_CHECK(errors[4] < 0.01);
  RC_CHECK(errors[5] < 1e-6);

  // Negating the first coefficient, which looks like the inverse and is not.
  Intrinsics flipped = k;
  flipped.k1 = -k.k1;
  flipped.k2 = -k.k2;
  const double xd = (corner.u - k.cx) / k.fx;
  const double yd = (corner.v - k.cy) / k.fy;
  const double r2 = xd * xd + yd * yd;
  const double wrong_scale = 1.0 + flipped.k1 * r2 + flipped.k2 * r2 * r2;
  const double wrong = std::hypot(xd * wrong_scale - x, yd * wrong_scale - y) * 500.0;
  std::cout << "  and the quick fix of applying the lens model with its signs reversed is out by "
            << std::fixed << std::setprecision(1) << wrong << " pixels at the corner.\n\n";
  RC_CHECK(wrong > 3.0);
}

RC_TEST("straight lines bow, and the lens has to be undone before a line is a line") {
  const Intrinsics k = wide_lens();
  bool visible = false;

  // A straight edge: a row of points one metre above the optical axis, two metres away, from
  // 1.1 metres left to 1.1 metres right.
  std::vector<Pixel> image;
  for (int i = 0; i <= 20; ++i) image.push_back(project(k, -1.1 + 0.11 * i, 1.0, 2.0, visible));

  // How far each is from the straight line through the two ends, in the picture.
  const Pixel a = image.front();
  const Pixel b = image.back();
  double sag = 0.0;
  for (const Pixel& p : image) {
    const double t = (p.u - a.u) / (b.u - a.u);
    sag = std::fmax(sag, std::fabs(p.v - (a.v + t * (b.v - a.v))));
  }

  // And after the lens is undone each point looks along a direction whose y over z is the
  // same, so they are level again.
  double level = 0.0;
  for (const Pixel& p : image) level = std::fmax(level, std::fabs(unproject(k, p).y - 0.5));

  std::cout << "  an edge that is straight in the world, seen through the lens: the middle of it bows by "
            << std::fixed << std::setprecision(1) << sag << " pixels off the line\n  through its ends. With the lens undone it is level to "
            << std::scientific << std::setprecision(1) << level * 500.0 << " pixels.\n\n";

  RC_CHECK(sag > 5.0);
  RC_CHECK(level * 500.0 < 1e-6);
}

RC_TEST("range from size: a pixel of error in the width is more distance error the further it is") {
  const Intrinsics k;
  const double width = 0.2;   // a target a fifth of a metre wide

  std::cout << "  a target 0.2 m wide, and a width measured one pixel too wide. The distance it gives:\n\n"
            << "    true distance   appears    distance from it   error\n";

  double error_1m = 0.0;
  double error_10m = 0.0;
  for (const double z : {1.0, 2.0, 5.0, 10.0}) {
    const double pixels = k.fx * width / z;
    const double from_true = distance_from_width(k, width, pixels);
    const double from_wrong = distance_from_width(k, width, pixels + 1.0);

    std::cout << "    " << std::fixed << std::setprecision(1) << std::setw(8) << z << " m" << std::setw(8) << pixels
              << " px" << std::setprecision(3) << std::setw(14) << from_wrong << " m" << std::setw(10)
              << from_wrong - z << " m\n";

    RC_CHECK_NEAR(from_true, z, 1e-9);
    if (z == 1.0) error_1m = std::fabs(from_wrong - z);
    if (z == 10.0) error_10m = std::fabs(from_wrong - z);
  }
  std::cout << "\n  A pixel is a centimetre at a metre and nine tenths of a metre at ten: the error goes as the square of the\n"
            << "  distance, because the target gets smaller as it recedes and the pixel does not.\n\n";

  RC_CHECK(error_10m > 50.0 * error_1m);
  RC_CHECK_NEAR(distance_from_width(k, 0.2, 0.0), 0.0, 1e-12);
  RC_CHECK_NEAR(distance_from_width(k, 0.2, -5.0), 0.0, 1e-12);

  // A focal length that is two percent out makes every distance two percent out.
  Intrinsics wrong = k;
  wrong.fx = 510.0;
  RC_CHECK_NEAR(distance_from_width(wrong, 0.2, 50.0) / distance_from_width(k, 0.2, 50.0), 1.02, 1e-12);
}
