#define RC_TEST_NO_MAIN
#include <rc/test/rc_test.hpp>

#include <QtCore/QBuffer>
#include <QtCore/QByteArray>
#include <QtCore/QCoreApplication>
#include <QtCore/QList>

#include <iostream>
#include <string>

#include "solution.hpp"

namespace {

std::string str(const QByteArray& b) { return std::string(b.constData(), static_cast<std::size_t>(b.size())); }

// All the lines the assembler gives for these chunks, in one list.
QList<QByteArray> run(LineAssembler& a, const QList<QByteArray>& chunks) {
  QList<QByteArray> all;
  for (const QByteArray& c : chunks) all += a.feed(c);
  return all;
}

bool same(const QList<QByteArray>& x, const QList<QByteArray>& y) {
  if (x.size() != y.size()) return false;
  for (int i = 0; i < x.size(); ++i) {
    if (x.at(i) != y.at(i)) return false;
  }
  return true;
}

}  // namespace

RC_TEST("a device hands over chunks, and treating each chunk as a line splits a line in two") {
  QByteArray message("temp 21.5\nvolt 12.1\n");
  QBuffer device(&message);
  device.open(QIODevice::ReadOnly);

  // Read it the way a device gives it: seven bytes at a time.
  QList<QByteArray> chunks;
  for (;;) {
    const QByteArray chunk = device.read(7);
    if (chunk.isEmpty()) break;
    chunks.append(chunk);
  }

  // The naive reader: each chunk is split at the newlines and every piece is taken as a line.
  int naive_pieces = 0;
  bool naive_has_whole_first_line = false;
  for (const QByteArray& chunk : chunks) {
    const QList<QByteArray> pieces = chunk.split('\n');
    for (const QByteArray& p : pieces) {
      if (p.isEmpty()) continue;
      ++naive_pieces;
      if (p == "temp 21.5") naive_has_whole_first_line = true;
    }
  }
  std::cout << "  " << chunks.size() << " chunks of at most 7 bytes: a reader that splits each chunk found " << naive_pieces
            << " pieces, for 2 lines, and the first line was " << (naive_has_whole_first_line ? "whole" : "never whole") << "\n";
  RC_CHECK_EQ(chunks.size(), 3);
  RC_CHECK_EQ(naive_pieces, 4);
  RC_CHECK(!naive_has_whole_first_line);

  LineAssembler assembler(64);
  const QList<QByteArray> lines = run(assembler, chunks);
  std::cout << "  the assembler found " << lines.size() << " lines\n\n";
  RC_CHECK_EQ(lines.size(), 2);
  if (lines.size() == 2) {
    RC_CHECK_EQ(str(lines.at(0)), std::string("temp 21.5"));
    RC_CHECK_EQ(str(lines.at(1)), std::string("volt 12.1"));
  }
}

RC_TEST("an unfinished line is held until its end arrives, and not lost or returned early") {
  LineAssembler a(64);
  RC_CHECK_EQ(a.feed(QByteArray("abc")).size(), 0);
  RC_CHECK_EQ(a.pending(), 3);
  const QList<QByteArray> out = a.feed(QByteArray("def\nxy"));
  RC_CHECK_EQ(out.size(), 1);
  if (out.size() == 1) RC_CHECK_EQ(str(out.at(0)), std::string("abcdef"));
  RC_CHECK_EQ(a.pending(), 2);
  const QList<QByteArray> more = a.feed(QByteArray("z\n"));
  RC_CHECK_EQ(more.size(), 1);
  if (more.size() == 1) RC_CHECK_EQ(str(more.at(0)), std::string("xyz"));
  RC_CHECK_EQ(a.pending(), 0);
}

RC_TEST("the same bytes give the same lines however they are cut into chunks") {
  const QByteArray message("temp 21.5\r\nvolt 12.1\n\nlast line\r\n");
  LineAssembler whole(64);
  const QList<QByteArray> expected = whole.feed(message);
  RC_CHECK_EQ(expected.size(), 4);

  int cuts = 0;
  int differ = 0;
  // Every way to cut it into two chunks, and into three.
  for (int i = 0; i <= message.size(); ++i) {
    LineAssembler a(64);
    if (!same(run(a, {message.left(i), message.mid(i)}), expected)) ++differ;
    ++cuts;
    for (int j = i; j <= message.size(); ++j) {
      LineAssembler b(64);
      if (!same(run(b, {message.left(i), message.mid(i, j - i), message.mid(j)}), expected)) ++differ;
      ++cuts;
    }
  }
  // And one byte at a time.
  LineAssembler one(64);
  QList<QByteArray> bytes;
  for (int i = 0; i < message.size(); ++i) bytes.append(message.mid(i, 1));
  const bool one_ok = same(run(one, bytes), expected);
  std::cout << "  " << cuts << " ways of cutting " << message.size() << " bytes into chunks, " << differ << " gave different lines; one byte at a time: "
            << (one_ok ? "the same" : "different") << "\n\n";
  RC_CHECK_EQ(differ, 0);
  RC_CHECK(one_ok);
  RC_CHECK_EQ(cuts, 629);                // 34 cuts into two, and 595 into three
}

RC_TEST("a carriage return before the newline is removed, an empty line is a line, and a carriage return inside a line stays") {
  LineAssembler a(64);
  const QList<QByteArray> lines = a.feed(QByteArray("a\r\nb\n\r\nc\rd\n\n"));
  std::cout << "  \"a\\r\\nb\\n\\r\\nc\\rd\\n\\n\" gave " << lines.size() << " lines; a reader that splits at \\n alone keeps the \\r, and its first line is "
            << QByteArray("a\r\nb").split('\n').at(0).size() << " bytes, not 1\n\n";
  RC_CHECK_EQ(lines.size(), 5);
  if (lines.size() == 5) {
    RC_CHECK_EQ(str(lines.at(0)), std::string("a"));          // not "a\r"
    RC_CHECK_EQ(str(lines.at(1)), std::string("b"));
    RC_CHECK_EQ(str(lines.at(2)), std::string(""));           // "\r\n" is an empty line
    RC_CHECK_EQ(str(lines.at(3)), std::string("c\rd"));       // a lone \r is data
    RC_CHECK_EQ(str(lines.at(4)), std::string(""));
  }
  // What splitting at \n alone gives for the same first line.
  RC_CHECK_EQ(QByteArray("a\r\nb").split('\n').at(0).size(), 2);
}

RC_TEST("a line with no end is dropped and not held, so noise cannot make the buffer grow") {
  LineAssembler a(8);
  // Exactly the limit is kept.
  const QList<QByteArray> ok = a.feed(QByteArray("12345678\n"));
  RC_CHECK_EQ(ok.size(), 1);
  RC_CHECK_EQ(a.dropped(), 0);

  QList<QByteArray> out = a.feed(QByteArray("fine\n"));
  RC_CHECK_EQ(out.size(), 1);

  // A hundred thousand bytes of noise with no newline, in chunks.
  int worst_pending = 0;
  QList<QByteArray> from_noise;
  for (int i = 0; i < 1000; ++i) {
    from_noise += a.feed(QByteArray(100, 'x'));
    if (a.pending() > worst_pending) worst_pending = a.pending();
  }
  std::cout << "  100000 bytes with no newline: at most " << worst_pending << " bytes held (limit 8), " << from_noise.size() << " lines returned, skipping "
            << a.skipping() << "\n";
  RC_CHECK(worst_pending <= 8);
  RC_CHECK_EQ(from_noise.size(), 0);
  RC_CHECK(a.skipping());

  // The line ends, it is counted, and the next one is read normally.
  const QList<QByteArray> after = a.feed(QByteArray("\nnext\n"));
  std::cout << "  after its newline: dropped " << a.dropped() << ", then " << after.size() << " line: " << (after.size() == 1 ? str(after.at(0)) : std::string("?"))
            << "\n\n";
  RC_CHECK_EQ(a.dropped(), 1);
  RC_CHECK_EQ(after.size(), 1);
  if (after.size() == 1) RC_CHECK_EQ(str(after.at(0)), std::string("next"));
  RC_CHECK(!a.skipping());
}

int main(int argc, char** argv) {
  qputenv("QT_QPA_PLATFORM", "offscreen");
  QCoreApplication app(argc, argv);
  return rc::test::run_all();
}
