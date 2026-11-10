#define RC_TEST_NO_MAIN
#include <rc/test/rc_test.hpp>

#include <QtCore/QByteArray>
#include <QtCore/QCoreApplication>
#include <QtCore/QDataStream>
#include <QtCore/QIODevice>

#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

#include "solution.hpp"

namespace {

std::string hex(const QByteArray& b) {
  std::ostringstream out;
  for (int i = 0; i < b.size(); ++i) {
    if (i > 0) out << ' ';
    out << std::hex << std::setw(2) << std::setfill('0') << int(static_cast<unsigned char>(b.at(i)));
  }
  return out.str();
}

Reading sample() {
  Reading r;
  r.id = 0x0102;                              // 258
  r.value = 1.5;
  r.name = QString::fromUtf8("\xC2\xB5s");    // the micro sign and an s: three bytes in UTF-8
  return r;
}

}  // namespace

RC_TEST("a QDataStream writes big endian unless told otherwise, and a float as eight bytes, and a string as UTF-16") {
  QByteArray plain;
  {
    QDataStream s(&plain, QIODevice::WriteOnly);
    s << quint16(0x0102);
  }
  QByteArray little;
  {
    QDataStream s(&little, QIODevice::WriteOnly);
    s.setByteOrder(QDataStream::LittleEndian);
    s << quint16(0x0102);
  }
  QByteArray as_float;
  {
    QDataStream s(&as_float, QIODevice::WriteOnly);
    s << 1.5f;
  }
  QByteArray single;
  {
    QDataStream s(&single, QIODevice::WriteOnly);
    s.setFloatingPointPrecision(QDataStream::SinglePrecision);
    s << 1.5f;
  }
  QByteArray text;
  {
    QDataStream s(&text, QIODevice::WriteOnly);
    s << QString("ab");
  }
  std::cout << "  the 16 bit number 0x0102, by default:       " << hex(plain) << "\n"
            << "  the same, little endian:                    " << hex(little) << "\n"
            << "  the float 1.5, by default (" << as_float.size() << " bytes):       " << hex(as_float) << "\n"
            << "  the float 1.5, single precision (" << single.size() << " bytes):  " << hex(single) << "\n"
            << "  the string \"ab\" (" << text.size() << " bytes):             " << hex(text) << "\n\n";
  RC_CHECK_EQ(hex(plain), std::string("01 02"));
  RC_CHECK_EQ(hex(little), std::string("02 01"));
  RC_CHECK_EQ(as_float.size(), 8);
  RC_CHECK_EQ(single.size(), 4);
  RC_CHECK_EQ(hex(text), std::string("00 00 00 04 00 61 00 62"));
  // And the frame of this lesson says every one of those things for itself, so it is 14 bytes and not what the defaults would give.
  RC_CHECK_EQ(encode_reading(sample()).size(), 14);
}

RC_TEST("a reading is written to exactly the bytes the format describes") {
  const QByteArray frame = encode_reading(sample());
  std::cout << "  id 258, value 1.5, name micro-s: " << frame.size() << " bytes: " << hex(frame) << "\n\n";
  RC_CHECK_EQ(hex(frame), std::string("02 01 00 00 00 00 00 00 f8 3f 03 c2 b5 73"));
  RC_CHECK_EQ(frame.size(), 14);

  Reading empty_name;
  empty_name.id = 7;
  empty_name.value = -2.0;
  RC_CHECK_EQ(hex(encode_reading(empty_name)), std::string("07 00 00 00 00 00 00 00 00 c0 00"));

  Reading too_long = sample();
  too_long.name = QString(300, QLatin1Char('x'));
  RC_CHECK(encode_reading(too_long).isEmpty());
  too_long.name = QString(255, QLatin1Char('x'));
  RC_CHECK_EQ(encode_reading(too_long).size(), 11 + 255);
}

RC_TEST("a frame is read back as the reading it was written from") {
  const DecodedReading d = decode_reading(encode_reading(sample()));
  RC_CHECK(d.ok);
  RC_CHECK_EQ(int(d.reading.id), 258);
  RC_CHECK_NEAR(d.reading.value, 1.5, 1e-15);
  RC_CHECK(d.reading.name == sample().name);
  RC_CHECK_EQ(d.reading.name.length(), 2);

  // A value that is not exact in decimal comes back exactly.
  Reading r = sample();
  r.value = 0.1 + 0.2;
  RC_CHECK(decode_reading(encode_reading(r)).reading.value == r.value);
}

RC_TEST("a frame that is short, long or cut in the name is refused, at every length") {
  const QByteArray frame = encode_reading(sample());
  RC_CHECK_EQ(frame.size(), 14);
  if (frame.size() != 14) return;

  int refused_short = 0;
  for (int n = 0; n < frame.size(); ++n) {
    if (!decode_reading(frame.left(n)).ok) ++refused_short;
  }
  QByteArray longer = frame;
  longer.append('\x00');
  const bool refused_long = !decode_reading(longer).ok;
  QByteArray much_longer = frame;
  much_longer.append(QByteArray(20, 'x'));
  const bool refused_much_longer = !decode_reading(much_longer).ok;

  // A length byte that says more than there is.
  QByteArray lies = frame;
  lies[10] = static_cast<char>(200);
  const bool refused_lie = !decode_reading(lies).ok;

  std::cout << "  every prefix of the 14 byte frame, from 0 to 13 bytes: " << refused_short << " of 14 refused; one byte too many: "
            << (refused_long ? "refused" : "accepted") << "; a length byte of 200: " << (refused_lie ? "refused" : "accepted") << "\n";
  RC_CHECK_EQ(refused_short, 14);
  RC_CHECK(refused_long);
  RC_CHECK(refused_much_longer);
  RC_CHECK(refused_lie);
  RC_CHECK(decode_reading(frame).ok);                         // and the whole frame is accepted
}

RC_TEST("reading past the end of a QDataStream gives zero and a status, and the bytes that were read before it are not lost") {
  const QByteArray two("\x01\x02", 2);
  QDataStream stream(two);
  quint16 first = 77;
  quint16 second = 88;
  stream >> first;
  const bool after_first_ok = stream.status() == QDataStream::Ok;
  stream >> second;                                           // nothing left
  std::cout << "  two bytes, read as two 16 bit numbers: the first is " << std::hex << first << std::dec << " (status " << (after_first_ok ? "Ok" : "bad")
            << "), the second is " << second << " with status " << (stream.status() == QDataStream::ReadPastEnd ? "ReadPastEnd" : "other") << "\n\n";
  RC_CHECK(after_first_ok);
  RC_CHECK_EQ(int(first), 0x0102);
  RC_CHECK_EQ(int(second), 0);                                // not 88: the value was overwritten with zero
  RC_CHECK(stream.status() == QDataStream::ReadPastEnd);
  RC_CHECK(!decode_reading(QByteArray("\x01\x02", 2)).ok);
  RC_CHECK(decode_reading(encode_reading(sample())).ok);       // refusing everything would not pass
}

int main(int argc, char** argv) {
  qputenv("QT_QPA_PLATFORM", "offscreen");
  QCoreApplication app(argc, argv);
  return rc::test::run_all();
}
