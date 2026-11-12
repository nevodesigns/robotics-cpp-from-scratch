#include <rc/test/rc_test.hpp>

#include <cstdint>
#include <iomanip>
#include <iostream>

#include "solution.hpp"

namespace {

const std::uint64_t kMB = 1024ULL * 1024ULL;

}  // namespace

RC_TEST("a policy is usable when a file can hold something and the total allows a whole file, and its budget is the smaller of two limits") {
  LogPolicy p;
  RC_CHECK(policy_valid(p));
  RC_CHECK(budget(p) == 200 * kMB);                    // 20 files of 10 MB is exactly the 200 MB total
  LogPolicy few = p;
  few.max_files = 5;
  RC_CHECK(budget(few) == 50 * kMB);                   // the number of files is the tighter limit
  LogPolicy big = p;
  big.max_total_bytes = 30 * kMB;
  RC_CHECK(budget(big) == 30 * kMB);
  LogPolicy zero_file = p;
  zero_file.max_file_bytes = 0;
  LogPolicy no_files = p;
  no_files.max_files = 0;
  LogPolicy small_total = p;
  small_total.max_total_bytes = kMB;
  RC_CHECK(!policy_valid(zero_file) && !policy_valid(no_files) && !policy_valid(small_total));
  std::cout << "  files of 10 MB, at most 20 files and 200 MB: budget " << budget(p) / kMB << " MB; at most 5 files: " << budget(few) / kMB << " MB; a total of 30 MB: " << budget(big) / kMB << " MB\n\n";
}

RC_TEST("records fill a file, the next one closes it, and the oldest files go first so that what is left is always the newest, with no holes") {
  LogPolicy p;
  p.max_file_bytes = 100;
  p.max_total_bytes = 300;
  p.max_files = 10;
  LogStore s(p);
  for (int i = 0; i < 9; ++i) s.append(40);             // 2 per file: 80 bytes
  RC_CHECK(s.files() == 4 && s.first_seq() == 1 && s.last_seq() == 4);     // five files were started, the first went when the total passed 300
  RC_CHECK(s.total() == 280 && s.deleted() == 80);
  RC_CHECK(s.last_seq() - s.first_seq() + 1 == static_cast<std::uint64_t>(s.files()));          // consecutive: no file missing in the middle
  RC_CHECK(s.total() + s.deleted() == 9 * 40);
  LogStore z(p);
  z.append(0);
  RC_CHECK(z.files() == 0 && z.total() == 0);
  std::cout << "  nine records of 40 bytes, files of 100, a total of 300: " << s.files() << " files kept (numbers " << s.first_seq() << " to " << s.last_seq() << "), " << s.total() << " bytes, " << s.deleted() << " deleted\n\n";
}

RC_TEST("a day of logging at 100 KB a second fills a 4 GB disk in eleven hours with no limit, and stays inside the budget with one") {
  LogPolicy p;                                          // 10 MB files, 20 files, 200 MB
  LogStore s(p);
  const std::uint64_t record = 10000;                   // ten records a second of 10 KB
  const std::uint64_t disk = 4000ULL * 1000ULL * 1000ULL;
  std::uint64_t unlimited = 0;
  double full_at = -1.0;
  std::uint64_t peak = 0;
  int most_files = 0;
  for (int sec = 0; sec < 86400; ++sec) {
    for (int k = 0; k < 10; ++k) {
      s.append(record);
      unlimited += record;
    }
    if (full_at < 0.0 && unlimited >= disk) full_at = sec / 3600.0;
    peak = std::max(peak, s.total());
    most_files = std::max(most_files, s.files());
  }
  std::cout << "  24 hours at 100 KB/s:\n  no limit: " << std::fixed << std::setprecision(1) << unlimited / 1e9 << " GB written, a 4 GB disk full after " << full_at << " hours\n";
  std::cout << "  budget of " << budget(p) / kMB << " MB: peak " << peak / kMB << " MB in at most " << most_files << " files, " << std::setprecision(2) << s.deleted() / 1e9 << " GB deleted along the way\n\n";
  RC_CHECK(full_at > 11.0 && full_at < 11.2);
  RC_CHECK(unlimited == 8640000000ULL);
  RC_CHECK(peak <= budget(p));
  RC_CHECK(most_files <= p.max_files);
  RC_CHECK(s.total() + s.deleted() == unlimited);
  RC_CHECK(s.last_seq() - s.first_seq() + 1 == static_cast<std::uint64_t>(s.files()));
}

RC_TEST("whatever the sizes of the records, the log never goes over its budget, and one record bigger than a file is kept until the next arrives") {
  LogPolicy p;
  p.max_file_bytes = 1000;
  p.max_total_bytes = 5000;
  p.max_files = 8;
  LogStore s(p);
  std::uint64_t x = 88172645463325252ULL;
  std::uint64_t worst = 0;
  int breaches = 0;
  for (int i = 0; i < 100000; ++i) {
    x ^= x << 13;
    x ^= x >> 7;
    x ^= x << 17;
    s.append(1 + x % 900);
    worst = std::max(worst, s.total());
    if (s.total() > budget(p) || s.files() > p.max_files) ++breaches;
  }
  std::cout << "  100000 records of 1 to 900 bytes: the most it ever held was " << worst << " bytes of a budget of " << budget(p) << ", " << breaches << " breaches\n";
  RC_CHECK(breaches == 0);
  RC_CHECK(worst <= 5000);
  LogStore b(p);
  b.append(100);
  b.append(9000);                                         // a record bigger than the whole budget
  RC_CHECK(b.total() == 9000);                             // it is the current file, which is never deleted
  RC_CHECK(b.files() == 1);
  b.append(50);                                            // the next one closes it, and it is the oldest closed file, so it goes
  RC_CHECK(b.total() == 50 && b.files() == 1);
  std::cout << "  a record of 9000 bytes against a budget of 5000: held (" << 9000 << ") until the next record, then dropped\n\n";
}

RC_TEST("one error said a thousand times a second is written five times and then counted, and said again after the window") {
  FloodGuard g(5, 10.0);
  int written = 0;
  int refused = 0;
  for (int i = 0; i < 10000; ++i) {                        // 1000 a second for 10 s
    const double t = i / 1000.0;
    if (g.allow("camera: no frame", t)) ++written;
    else ++refused;
  }
  std::cout << "  'camera: no frame' 1000 times a second for 10 s: " << written << " written, " << refused << " suppressed (" << g.suppressed("camera: no frame") << " counted)\n";
  RC_CHECK(written == 5);
  RC_CHECK(refused == 9995);
  RC_CHECK(g.suppressed("camera: no frame") == 9995);
  RC_CHECK(g.allow("lidar: timeout", 1.0));                // another message is not held back
  RC_CHECK(g.allow("camera: no frame", 10.0));             // after the window: a new one opens
  RC_CHECK(g.take_suppressed("camera: no frame") == 9995);
  RC_CHECK(g.take_suppressed("camera: no frame") == 0);
  RC_CHECK(g.take_suppressed("never seen") == 0);
  RC_CHECK(g.suppressed("never seen") == 0);
  FloodGuard back(2, 10.0);
  RC_CHECK(back.allow("x", 100.0) && back.allow("x", 100.1) && !back.allow("x", 100.2));
  RC_CHECK(back.allow("x", 50.0));                          // the clock went back: a new window, not a muted message
  std::cout << "  another message is not held back, a message after the window opens a new one, and the count is handed over once\n\n";
}
