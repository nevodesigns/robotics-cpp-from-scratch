#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <QtCore/QByteArray>
#include <QtCore/QDataStream>
#include <QtCore/QIODevice>
#include <QtCore/QString>

// One reading as the wire format wants it.
struct Reading {
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
inline QByteArray encode_reading(const Reading& r) {
  // TODO: the name as UTF-8 (r.name.toUtf8()); if it is longer than 255 bytes return an empty QByteArray.
  // Otherwise write into a QByteArray through a QDataStream opened WriteOnly: stream.setByteOrder(QDataStream::LittleEndian),
  // stream.setFloatingPointPrecision(QDataStream::DoublePrecision), then stream << r.id, stream << r.value,
  // stream << static_cast<quint8>(name.size()), and the name bytes with stream.writeRawData(name.constData(), name.size()).
  (void)r;
  return QByteArray("not written");
}

struct DecodedReading {
  Reading reading;
  bool ok = false;
};

// Read a frame back, and say whether it was a frame: exactly the bytes the format describes, no fewer and no more.
inline DecodedReading decode_reading(const QByteArray& bytes) {
  // TODO: a QDataStream on the bytes with the same little endian order and double precision. Read an id (quint16), a value
  // (double) and a length (quint8) with >>. If stream.status() is not QDataStream::Ok it ended too soon: not ok.
  // Read `length` name bytes with stream.readRawData into a QByteArray of that size; if the count read is not `length`, not ok.
  // If !stream.atEnd() there are extra bytes: not ok. Otherwise fill in the reading (the name with QString::fromUtf8) and set ok.
  (void)bytes;
  return DecodedReading();
}

#endif  // LESSON_SOLUTION_HPP
