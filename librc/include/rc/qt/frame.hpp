// rc/qt/frame.hpp
//
// A reading written to and read from a documented byte layout with a QDataStream, from lesson 09-07, graduated.
//
// A QDataStream does not write what a wire format usually wants. The 16 bit number 0x0102 went out as 01 02 by default and 02 01 little endian; the float 1.5 went out as eight bytes (3f f8 00 00 00 00 00 00) and as four with single precision
// (3f c0 00 00); and the string "ab" as eight bytes, 00 00 00 04 00 61 00 62, a four byte length in bytes and UTF-16. So the format says each of those, writes the name as raw UTF-8 after a length byte, and an id of 258, a value of 1.5 and the name micro-s came out as 14 bytes:
// 02 01 00 00 00 00 00 00 f8 3f 03 c2 b5 73. A frame that was short at any of its 14 prefixes, long, or had a length byte of 200 was refused. A read past the end gave 0 and the status ReadPastEnd.

#ifndef RC_QT_FRAME
#define RC_QT_FRAME

#include <QtCore/QByteArray>
#include <QtCore/QDataStream>
#include <QtCore/QIODevice>
#include <QtCore/QString>

namespace rc {
namespace qt {

// One reading as the wire format wants it.
struct FrameReading {
  quint16 id = 0;
  double value = 0.0;
  QString name;
};

// The wire format, written down once:
//
//   bytes 0 to 1    the id, a 16 bit number, LEAST significant byte first
//   bytes 2 to 9    the value, an IEEE 754 double, least significant byte first
//   byte  10        the length of the name in bytes, 0 to 255
//   bytes 11 on     the name in UTF-8, that many bytes
//
// A QDataStream does not write that by default. Its byte order is big endian, so the first two bytes would be the wrong way round. A float
// goes out as eight bytes unless it is told otherwise. And a QString goes out as a four byte length in bytes followed by UTF-16, which is
// nothing like a length byte and UTF-8. So each of those is said, and the name is written as raw bytes.

// Empty if the name does not fit in 255 bytes.
inline QByteArray encode_reading(const FrameReading& r) {
  const QByteArray name = r.name.toUtf8();
  if (name.size() > 255) return QByteArray();

  QByteArray out;
  QDataStream stream(&out, QIODevice::WriteOnly);
  stream.setByteOrder(QDataStream::LittleEndian);
  stream.setFloatingPointPrecision(QDataStream::DoublePrecision);
  stream << r.id;
  stream << r.value;
  stream << static_cast<quint8>(name.size());
  stream.writeRawData(name.constData(), name.size());
  return out;
}

struct DecodedReading {
  FrameReading reading;
  bool ok = false;
};

// Read a frame back, and say whether it was a frame: exactly the bytes the format describes, no fewer and no more.
inline DecodedReading decode_reading(const QByteArray& bytes) {
  DecodedReading d;
  QDataStream stream(bytes);
  stream.setByteOrder(QDataStream::LittleEndian);
  stream.setFloatingPointPrecision(QDataStream::DoublePrecision);

  quint16 id = 0;
  double value = 0.0;
  quint8 length = 0;
  stream >> id >> value >> length;
  if (stream.status() != QDataStream::Ok) return d;          // it ended before the fixed part did

  QByteArray name(length, 0);
  if (stream.readRawData(name.data(), length) != length) return d;   // the name was cut short
  if (!stream.atEnd()) return d;                                      // there is more than the format describes

  d.reading.id = id;
  d.reading.value = value;
  d.reading.name = QString::fromUtf8(name);
  d.ok = true;
  return d;
}

}  // namespace qt
}  // namespace rc

#endif  // RC_QT_FRAME
