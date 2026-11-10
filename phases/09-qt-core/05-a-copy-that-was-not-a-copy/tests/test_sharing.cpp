// A QByteArray needs no application object, but the harness main is replaced anyway so the offscreen platform is set the same way as in every Qt lesson.
#define RC_TEST_NO_MAIN
#include <rc/test/rc_test.hpp>

#include <QtCore/QByteArray>
#include <QtCore/QCoreApplication>

#include <iomanip>
#include <iostream>
#include <utility>

#include "solution.hpp"

namespace {

QByteArray make_frame(int n) {
  QByteArray frame(n, 0);
  for (int i = 0; i < n; ++i) frame[i] = static_cast<char>(i % 251);
  return frame;
}

int expected_sum(int n) {
  int total = 0;
  for (int i = 0; i < n; ++i) total += i % 251;
  return total;
}

}  // namespace

RC_TEST("a copy shares the bytes until one of them is written to") {
  const QByteArray a = make_frame(1000);
  QByteArray b = a;
  std::cout << "  after b = a: same bytes " << (a.constData() == b.constData() ? "yes" : "no") << "\n";
  RC_CHECK(a.constData() == b.constData());

  b[0] = 99;                                       // a write: b takes its own bytes
  std::cout << "  after writing b[0]: same bytes " << (a.constData() == b.constData() ? "yes" : "no") << ", a[0] is " << int(a.at(0))
            << ", b[0] is " << int(b.at(0)) << "\n\n";
  RC_CHECK(a.constData() != b.constData());
  RC_CHECK_EQ(int(a.at(0)), 0);
  RC_CHECK_EQ(int(b.at(0)), 99);
  RC_CHECK_EQ(sum_const(a), expected_sum(1000));                       // and the one that was not written to is still whole
}

RC_TEST("reading through something that is not const copies the bytes, and at() does not") {
  const QByteArray a = make_frame(1000);
  const char* original = a.constData();

  QByteArray by_index = a;
  const char first = by_index[0];                 // only reads, but the array is not const so operator[] is the one that can write
  (void)first;

  QByteArray by_at = a;
  const char first_at = by_at.at(0);
  (void)first_at;

  QByteArray by_data = a;
  (void)by_data.data();

  QByteArray by_const_data = a;
  (void)by_const_data.constData();
  (void)by_const_data.size();

  std::cout << "  a read that only looks, on a copy of a 1000 byte array:\n"
            << "    b[0]            copied: " << (by_index.constData() != original ? "yes" : "no") << "\n"
            << "    b.at(0)         copied: " << (by_at.constData() != original ? "yes" : "no") << "\n"
            << "    b.data()        copied: " << (by_data.constData() != original ? "yes" : "no") << "\n"
            << "    b.constData()   copied: " << (by_const_data.constData() != original ? "yes" : "no") << "\n\n";
  RC_CHECK(by_index.constData() != original);
  RC_CHECK(by_at.constData() == original);
  RC_CHECK(by_data.constData() != original);
  RC_CHECK(by_const_data.constData() == original);
  RC_CHECK_EQ(sum_const(by_at), expected_sum(1000));          // reading the shared one with the function of this lesson changes nothing
  RC_CHECK(by_at.constData() == original);
}

RC_TEST("a loop that only reads copies the bytes for every frame it is given, unless it looks through const") {
  const int frames = 1000;
  const QByteArray source = make_frame(4096);
  const char* original = source.constData();

  int copied_by_nonconst = 0;
  int copied_by_const = 0;
  for (int i = 0; i < frames; ++i) {
    QByteArray f = source;
    int sum = 0;
    for (char c : f) sum += c;                    // range-for over a non-const array
    (void)sum;
    if (f.constData() != original) ++copied_by_nonconst;

    QByteArray g = source;
    int sum2 = 0;
    for (char c : std::as_const(g)) sum2 += c;    // the same loop over a const view
    (void)sum2;
    if (g.constData() != original) ++copied_by_const;
  }
  std::cout << "  " << frames << " frames of 4096 bytes read by a range-for: " << copied_by_nonconst << " copied through a non-const array, "
            << copied_by_const << " through std::as_const\n\n";
  RC_CHECK_EQ(copied_by_nonconst, frames);
  RC_CHECK_EQ(copied_by_const, 0);
  RC_CHECK_EQ(sum_const(source), expected_sum(4096));
}

RC_TEST("summing a const array reads every byte without a copy") {
  const QByteArray frame = make_frame(1000);
  const char* original = frame.constData();
  RC_CHECK_EQ(sum_const(frame), expected_sum(1000));
  RC_CHECK(frame.constData() == original);
  RC_CHECK_EQ(sum_const(QByteArray()), 0);
}

RC_TEST("an array passed by value is a shared copy, and a loop over a const view keeps it shared") {
  const QByteArray frame = make_frame(1000);
  bool detached = true;
  const int total = sum_range_for(frame, frame.constData(), &detached);
  std::cout << "  an array passed by value and read with std::as_const: sum " << total << ", copied: " << (detached ? "yes" : "no") << "\n";
  RC_CHECK_EQ(total, expected_sum(1000));
  RC_CHECK(!detached);
}

RC_TEST("putting a marker in a frame copies it once and leaves the caller's frame alone") {
  const QByteArray frame = make_frame(1000);
  const char* original = frame.constData();
  const QByteArray marked = with_marker(frame, 77);
  std::cout << "  with_marker on a 1000 byte frame: the result's first byte is " << int(marked.at(0)) << ", the caller's is still "
            << int(frame.at(0)) << ", and the result is " << (marked.constData() == original ? "the same bytes" : "its own bytes") << "\n";
  RC_CHECK_EQ(int(marked.at(0)), 77);
  RC_CHECK_EQ(int(frame.at(0)), 0);
  RC_CHECK(frame.constData() == original);
  RC_CHECK(marked.constData() != original);
  RC_CHECK_EQ(with_marker(QByteArray(), 5).size(), 0);
}

int main(int argc, char** argv) {
  qputenv("QT_QPA_PLATFORM", "offscreen");
  QCoreApplication app(argc, argv);
  return rc::test::run_all();
}
