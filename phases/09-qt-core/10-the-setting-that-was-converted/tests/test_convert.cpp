#define RC_TEST_NO_MAIN
#include <rc/test/rc_test.hpp>

#include <QtCore/QCoreApplication>
#include <QtCore/QString>
#include <QtCore/QVariant>

#include <cmath>
#include <iomanip>
#include <iostream>

#include "solution.hpp"

RC_TEST("QVariant turns 3.7 into 4, 3e9 into a negative number, and says ok every time") {
  bool ok = false;
  const int rounded = QVariant(3.7).toInt(&ok);
  const bool rounded_ok = ok;
  const int rounded_down = QVariant(-3.7).toInt();
  const int wrapped = QVariant(3e9).toInt(&ok);
  const bool wrapped_ok = ok;
  const unsigned negative_as_unsigned = QVariant(-1).toUInt(&ok);
  const bool unsigned_ok = ok;
  std::cout << "  QVariant(3.7).toInt() = " << rounded << " (ok " << rounded_ok << "), QVariant(-3.7).toInt() = " << rounded_down
            << ", QVariant(3e9).toInt() = " << wrapped << " (ok " << wrapped_ok << "), QVariant(-1).toUInt() = " << negative_as_unsigned << " (ok "
            << unsigned_ok << ")\n\n";
  RC_CHECK_EQ(rounded, 4);
  RC_CHECK(rounded_ok);
  RC_CHECK_EQ(rounded_down, -4);
  RC_CHECK_EQ(wrapped, -1294967296);
  RC_CHECK(wrapped_ok);
  RC_CHECK_EQ(negative_as_unsigned, 4294967295u);
  RC_CHECK(unsigned_ok);

  // The strict reader refuses each of those.
  RC_CHECK(!([&] { bool k; setting_int(QVariant(3.7), &k); return k; })());
  RC_CHECK(!([&] { bool k; setting_int(QVariant(3e9), &k); return k; })());
  // While a whole number is still read as one, so refusing everything would not pass.
  RC_CHECK_EQ(setting_int(QVariant(42), &ok), 42);
  RC_CHECK(ok);
}

RC_TEST("a whole number is read as one, and anything that would change it is refused") {
  bool ok = false;
  RC_CHECK_EQ(setting_int(QVariant(42), &ok), 42);
  RC_CHECK(ok);
  RC_CHECK_EQ(setting_int(QVariant(-7), &ok), -7);
  RC_CHECK(ok);
  RC_CHECK_EQ(setting_int(QVariant(5.0), &ok), 5);               // a double that is whole
  RC_CHECK(ok);
  RC_CHECK_EQ(setting_int(QVariant(QString("12")), &ok), 12);
  RC_CHECK(ok);
  RC_CHECK_EQ(setting_int(QVariant(2147483647), &ok), 2147483647);
  RC_CHECK(ok);
  RC_CHECK_EQ(setting_int(QVariant(qlonglong(1000)), &ok), 1000);
  RC_CHECK(ok);

  int refused = 0;
  const QVariant bad[] = {QVariant(3.7), QVariant(-0.5), QVariant(3e9), QVariant(-3e9), QVariant(qlonglong(3000000000LL)), QVariant(quint64(5000000000ULL)),
                          QVariant(QString("3.7")), QVariant(QString("1e3")), QVariant(QString("12abc")), QVariant(QString("")), QVariant(true),
                          QVariant(), QVariant(std::nan(""))};
  for (const QVariant& v : bad) {
    setting_int(v, &ok);
    if (!ok) ++refused;
  }
  std::cout << "  " << refused << " of " << sizeof(bad) / sizeof(bad[0]) << " conversions that would change the value were refused by setting_int\n";
  RC_CHECK_EQ(refused, static_cast<int>(sizeof(bad) / sizeof(bad[0])));
}

RC_TEST("QVariant says the words no and off are true, and so are abc and 2, and 0.1 is false") {
  const char* words[] = {"false", "0", "no", "off", "", "true", "1", "yes", "abc", "2"};
  std::cout << "  QVariant(text).toBool():";
  for (const char* w : words) std::cout << " \"" << w << "\"=" << QVariant(QString(w)).toBool();
  std::cout << "\n  QVariant(0.1).toBool() = " << QVariant(0.1).toBool() << ", QVariant(nan).toBool() = " << QVariant(std::nan("")).toBool() << "\n\n";
  RC_CHECK(QVariant(QString("no")).toBool());
  RC_CHECK(QVariant(QString("off")).toBool());
  RC_CHECK(QVariant(QString("abc")).toBool());
  RC_CHECK(QVariant(QString("2")).toBool());
  RC_CHECK(!QVariant(QString("false")).toBool());
  RC_CHECK(!QVariant(QString("0")).toBool());
  RC_CHECK(!QVariant(0.1).toBool());                              // a double is cut to a whole number first
  RC_CHECK(QVariant(std::nan("")).toBool());

  // And the strict reader says no is false.
  bool ok = false;
  RC_CHECK(!setting_bool(QVariant(QString("no")), &ok));
  RC_CHECK(ok);
}

RC_TEST("true and false are read as the words people write for them, and anything else is refused") {
  bool ok = false;
  const char* trues[] = {"true", "True", "TRUE", "yes", "on", "1", "  yes  "};
  const char* falses[] = {"false", "False", "no", "NO", "off", "0"};
  int right = 0;
  for (const char* t : trues) {
    if (setting_bool(QVariant(QString(t)), &ok) && ok) ++right;
  }
  for (const char* f : falses) {
    if (!setting_bool(QVariant(QString(f)), &ok) && ok) ++right;
  }
  std::cout << "  " << right << " of " << (sizeof(trues) + sizeof(falses)) / sizeof(trues[0]) << " spellings of true and false were read correctly\n";
  RC_CHECK_EQ(right, 13);

  RC_CHECK(setting_bool(QVariant(true), &ok));
  RC_CHECK(ok);
  RC_CHECK(setting_bool(QVariant(1), &ok));
  RC_CHECK(ok);
  RC_CHECK(!setting_bool(QVariant(0.0), &ok));
  RC_CHECK(ok);

  int refused = 0;
  const QVariant bad[] = {QVariant(QString("abc")), QVariant(QString("2")), QVariant(QString("")), QVariant(QString("maybe")), QVariant(2), QVariant(0.5),
                          QVariant(-1), QVariant(std::nan("")), QVariant()};
  for (const QVariant& v : bad) {
    setting_bool(v, &ok);
    if (!ok) ++refused;
  }
  std::cout << "  " << refused << " of " << sizeof(bad) / sizeof(bad[0]) << " that are neither true nor false were refused\n\n";
  RC_CHECK_EQ(refused, static_cast<int>(sizeof(bad) / sizeof(bad[0])));
}

RC_TEST("a number is read from a number or from text in the plain locale, and not a number and infinity are refused") {
  bool ok = false;
  RC_CHECK_NEAR(setting_double(QVariant(1.5), &ok), 1.5, 1e-15);
  RC_CHECK(ok);
  RC_CHECK_NEAR(setting_double(QVariant(QString("-2.25")), &ok), -2.25, 1e-15);
  RC_CHECK(ok);
  RC_CHECK_NEAR(setting_double(QVariant(QString("1e3")), &ok), 1000.0, 1e-9);
  RC_CHECK(ok);
  RC_CHECK_NEAR(setting_double(QVariant(7), &ok), 7.0, 1e-15);
  RC_CHECK(ok);

  setting_double(QVariant(QString("1,5")), &ok);
  RC_CHECK(!ok);
  setting_double(QVariant(QString("abc")), &ok);
  RC_CHECK(!ok);
  setting_double(QVariant(std::nan("")), &ok);
  RC_CHECK(!ok);
  setting_double(QVariant(HUGE_VAL), &ok);
  RC_CHECK(!ok);
  setting_double(QVariant(true), &ok);
  RC_CHECK(!ok);
  setting_double(QVariant(), &ok);
  RC_CHECK(!ok);
}

int main(int argc, char** argv) {
  qputenv("QT_QPA_PLATFORM", "offscreen");
  QCoreApplication app(argc, argv);
  return rc::test::run_all();
}
