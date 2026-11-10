// rc/qt/text.hpp
//
// Where bytes become letters, and the dot a locale moves, from lesson 09-06, graduated.
//
// A QString length counts 16 bit units: the micro sign is 2 bytes in UTF-8, length 1 and 1 letter, an emoji is 4 bytes, length 2 and 1 letter. The bytes C2 B5 73 read as Latin-1 are 3 letters and as UTF-8 are 2, and the euro sign in Latin-1 is a question mark
// (the byte 63). A byte that is not valid UTF-8 becomes U+FFFD and the caller is told; a character cut off at the end of a whole frame is an error for a stateless decoder, and a decoder that keeps its state waits for the rest in the next chunk and makes the euro sign from two. With German as the
// application's locale QLocale().toString(1.5) was "1,5", QLocale().toDouble("1.500") was 1500 and "1.5" was refused; QLocale::c() gives 1.5 and 1.5 whatever the machine says.

#ifndef RC_QT_TEXT
#define RC_QT_TEXT

#include <QtCore/QByteArray>
#include <QtCore/QLocale>
#include <QtCore/QString>
#include <QtCore/QStringDecoder>

namespace rc {
namespace qt {

// What came of turning bytes into text: the text, and whether every byte made sense.
struct Decoded {
  QString text;
  bool clean = true;
};

// Bytes to text, saying so when some of them were not valid.
//
// A byte string is not text until it is said which encoding it is in. This reads UTF-8. A byte that cannot be part of valid UTF-8 does
// not stop the decoding: it becomes the replacement character, U+FFFD, and the rest carries on, which is kind to a log file and
// treacherous to a protocol, so the caller is told.
//
// The decoder is Stateless: this is a whole frame, so a character cut off at the end is an error. An ordinary decoder remembers a cut
// off character and waits for the rest in the next chunk, which is right for a stream and wrong here.
inline Decoded decode_utf8(const QByteArray& bytes) {
  Decoded d;
  QStringDecoder decoder(QStringDecoder::Utf8, QStringConverter::Flag::Stateless);
  d.text = decoder(bytes);
  d.clean = !decoder.hasError();
  return d;
}

// Text to bytes, in UTF-8.
inline QByteArray encode_utf8(const QString& text) { return text.toUtf8(); }

// How many characters a person would count.
//
// QString::length() counts the 16 bit units it stores, and a character outside the first 65536 (an emoji, say) takes two. Counting code points means converting to
// UCS-4, in which every character is one number.
inline int letters_in(const QString& text) { return static_cast<int>(text.toUcs4().size()); }

// A number from text, the same on every machine.
//
// QLocale::c() is the plain locale: a dot for the decimal point, no grouping. Whatever the machine's own locale says is a matter for a screen a
// person reads, and a protocol, a file or a configuration must not change with it.
inline double parse_number(const QString& text, bool* ok) { return QLocale::c().toDouble(text, ok); }

// A number as text, the same on every machine, with `decimals` places after the point.
inline QString format_number(double value, int decimals) { return QLocale::c().toString(value, 'f', decimals); }

}  // namespace qt
}  // namespace rc

#endif  // RC_QT_TEXT
