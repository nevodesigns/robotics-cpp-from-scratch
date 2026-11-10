#define RC_TEST_NO_MAIN
#include <rc/test/rc_test.hpp>

#include <QtCore/QByteArray>
#include <QtCore/QCoreApplication>
#include <QtCore/QLocale>
#include <QtCore/QString>
#include <QtCore/QStringDecoder>

#include <iomanip>
#include <iostream>

#include "solution.hpp"

namespace {

// Leaves the application's default locale as it found it.
class LocaleScope {
 public:
  explicit LocaleScope(const QLocale& to) : saved_(QLocale()) { QLocale::setDefault(to); }
  ~LocaleScope() { QLocale::setDefault(saved_); }

 private:
  QLocale saved_;
};

// Accessors that cannot read past the end, so a wrong answer from the code under test is a failed check and not an assertion inside Qt.
int unit_at(const QString& text, int i) { return i >= 0 && i < text.length() ? int(text.at(i).unicode()) : -1; }
int byte_at(const QByteArray& bytes, int i) { return i >= 0 && i < bytes.size() ? int(static_cast<unsigned char>(bytes.at(i))) : -1; }

}  // namespace

RC_TEST("a QString length counts 16 bit units, a person counts letters, and a byte string counts bytes") {
  const Decoded plain = decode_utf8("abc");
  const Decoded mu = decode_utf8("\xC2\xB5");                     // the micro sign, as it is written to a wire
  const Decoded emoji = decode_utf8("\xF0\x9F\x98\x80");          // a smiling face
  std::cout << "  text      bytes in UTF-8   length()   letters\n";
  std::cout << "  abc       " << std::setw(13) << encode_utf8(plain.text).size() << std::setw(11) << plain.text.length() << std::setw(10)
            << letters_in(plain.text) << "\n";
  std::cout << "  micro     " << std::setw(13) << encode_utf8(mu.text).size() << std::setw(11) << mu.text.length() << std::setw(10)
            << letters_in(mu.text) << "\n";
  std::cout << "  emoji     " << std::setw(13) << encode_utf8(emoji.text).size() << std::setw(11) << emoji.text.length() << std::setw(10)
            << letters_in(emoji.text) << "\n\n";

  RC_CHECK_EQ(plain.text.length(), 3);
  RC_CHECK_EQ(letters_in(plain.text), 3);
  RC_CHECK_EQ(encode_utf8(mu.text).size(), 2);
  RC_CHECK_EQ(mu.text.length(), 1);
  RC_CHECK_EQ(letters_in(mu.text), 1);
  RC_CHECK_EQ(encode_utf8(emoji.text).size(), 4);
  RC_CHECK_EQ(emoji.text.length(), 2);              // two 16 bit units for one character
  RC_CHECK_EQ(letters_in(emoji.text), 1);
}

RC_TEST("bytes read in the wrong encoding come out as other letters, and read in the right one they round trip") {
  const QByteArray wire = "\xC2\xB5s";                              // "micro s" in UTF-8
  const QString wrong = QString::fromLatin1(wire);                  // the same bytes read as Latin-1
  const Decoded right = decode_utf8(wire);
  std::cout << "  the bytes C2 B5 73: read as Latin-1 they are " << wrong.length() << " letters (U+" << std::hex << unit_at(wrong, 0) << ", U+"
            << unit_at(wrong, 1) << std::dec << ", ...), read as UTF-8 they are " << right.text.length() << " letters\n";
  RC_CHECK_EQ(wrong.length(), 3);
  RC_CHECK_EQ(right.text.length(), 2);
  RC_CHECK(right.clean);
  RC_CHECK(encode_utf8(right.text) == wire);

  // And text that does not fit in Latin-1 is lost by it, where UTF-8 keeps it.
  const QString euro = decode_utf8("\xE2\x82\xAC").text;
  const QByteArray latin = euro.toLatin1();
  std::cout << "  the euro sign as Latin-1 is the byte " << byte_at(latin, 0) << " (a question mark), as UTF-8 " << encode_utf8(euro).size()
            << " bytes\n\n";
  RC_CHECK_EQ(byte_at(latin, 0), 63);
  RC_CHECK_EQ(encode_utf8(euro).size(), 3);
}

RC_TEST("a byte that is not valid UTF-8 becomes a replacement character, and the caller is told") {
  const Decoded bad = decode_utf8(QByteArray("ab\xff" "c"));
  std::cout << "  the bytes 61 62 FF 63: " << bad.text.length() << " letters, the third is U+" << std::hex << unit_at(bad.text, 2) << std::dec
            << ", clean: " << (bad.clean ? "yes" : "no") << "\n";
  RC_CHECK(!bad.clean);
  RC_CHECK_EQ(bad.text.length(), 4);
  RC_CHECK_EQ(unit_at(bad.text, 2), 0xFFFD);

  const Decoded fine = decode_utf8(QByteArray("ab\xC2\xB5"));
  RC_CHECK(fine.clean);
  RC_CHECK(decode_utf8(QByteArray()).clean);

  // A cut off character at the end is also not clean: a frame boundary that split a multi byte letter.
  const Decoded cut = decode_utf8(QByteArray("ab\xE2\x82"));
  std::cout << "  a euro sign cut after two of its three bytes, as a whole frame: clean: " << (cut.clean ? "yes" : "no") << "\n";
  RC_CHECK(!cut.clean);

  // A decoder meant for a stream waits for the rest instead: the same two chunks, decoded one after the other, make the euro sign.
  QStringDecoder stream(QStringDecoder::Utf8);
  const QString first = stream(QByteArray("ab\xE2\x82"));
  const QString second = stream(QByteArray("\xAC"));
  std::cout << "  the same bytes through a decoder that keeps its state: the first chunk gives " << first.length() << " letters with no error, the second gives U+"
            << std::hex << second.at(0).unicode() << std::dec << "\n\n";
  RC_CHECK(!stream.hasError());
  RC_CHECK_EQ(first.length(), 2);
  RC_CHECK_EQ(int(second.at(0).unicode()), 0x20AC);
}

RC_TEST("a number is read and written the same way whatever the machine's locale says") {
  bool ok = false;
  RC_CHECK_NEAR(parse_number("1.5", &ok), 1.5, 1e-15);
  RC_CHECK(ok);
  RC_CHECK_NEAR(parse_number("-0.25", &ok), -0.25, 1e-15);
  RC_CHECK(ok);
  RC_CHECK_NEAR(parse_number("1e3", &ok), 1000.0, 1e-9);
  RC_CHECK(ok);
  parse_number("1,5", &ok);                       // a comma is not a decimal point here
  RC_CHECK(!ok);
  parse_number("1.5x", &ok);
  RC_CHECK(!ok);
  RC_CHECK_NEAR(parse_number("1,500.5", &ok), 1500.5, 1e-9);     // a grouping comma in the right place is accepted, which a strict parser might not want
  RC_CHECK(ok);
  parse_number("", &ok);
  RC_CHECK(!ok);

  RC_CHECK_EQ(format_number(1.5, 2).toStdString(), std::string("1.50"));
  RC_CHECK_EQ(format_number(1234567.0, 0).toStdString(), std::string("1234567"));

  // The same calls with a German machine's locale as the default: nothing changes.
  const LocaleScope german{QLocale(QLocale::German)};
  RC_CHECK_EQ(format_number(1.5, 2).toStdString(), std::string("1.50"));
  RC_CHECK_NEAR(parse_number("1.5", &ok), 1.5, 1e-15);
  RC_CHECK(ok);
}

RC_TEST("on a German machine the locale of the application writes 1,5 and reads 1.500 as fifteen hundred") {
  const LocaleScope german{QLocale(QLocale::German)};
  bool ok = false;
  const std::string written = QLocale().toString(1.5).toStdString();
  const double thousand = QLocale().toDouble(QString("1.500"), &ok);
  QLocale().toDouble(QString("1.5"), &ok);
  const bool dot_one_five_ok = ok;
  std::cout << "  with German as the application's locale: QLocale().toString(1.5) is \"" << written << "\", QLocale().toDouble(\"1.500\") is " << thousand
            << ", and \"1.5\" is " << (dot_one_five_ok ? "read" : "refused") << "\n";
  std::cout << "  the plain locale: format_number(1.5, 1) is \"" << format_number(1.5, 1).toStdString() << "\" and parse_number(\"1.500\") is "
            << parse_number("1.500", &ok) << "\n\n";
  RC_CHECK_EQ(written, std::string("1,5"));
  RC_CHECK_NEAR(thousand, 1500.0, 1e-9);
  RC_CHECK(!dot_one_five_ok);
  RC_CHECK_EQ(format_number(1.5, 1).toStdString(), std::string("1.5"));
  RC_CHECK_NEAR(parse_number("1.500", &ok), 1.5, 1e-15);
}

int main(int argc, char** argv) {
  qputenv("QT_QPA_PLATFORM", "offscreen");
  QCoreApplication app(argc, argv);
  return rc::test::run_all();
}
