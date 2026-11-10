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
  Reading reading;
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

#endif  // LESSON_SOLUTION_HPP
